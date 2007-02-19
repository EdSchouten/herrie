/*
 * Copyright (c) 2006-2007 Ed Schouten <ed@fxq.nl>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/**
 * @file audio_output_sdl.c
 */

#include <SDL.h>
#include <SDL_audio.h>

#include "audio_file.h"
#include "audio_output.h"
#include "gui.h"

/**
 * @brief Format currently used for playback.
 */
SDL_AudioSpec	curfmt;
/**
 * @brief Buffer with decoded audio data.
 */
Uint8		buf[AUDIO_OUTPUT_BUFLEN];
/**
 * @brief Amount of data still present in the audio buffer.
 */
size_t		buflen = 0;
/**
 * @brief Lock used for the buffer length.
 */
GMutex		*buflock;
/**
 * @brief Conditional used for notifying that new data can be written.
 */
GCond		*bufcond;

/**
 * @brief SDL Audio callback function which writes data to the audio
 *        device.
 */
static void
audio_output_write(void *data, Uint8 *stream, int len)
{
	if (buflen != 0) {
		SDL_MixAudio(stream, buf, buflen, SDL_MIX_MAXVOLUME);
		buflen = 0;
		g_cond_signal(bufcond);
	}
}

int
audio_output_open(void)
{
	buflock = g_mutex_new();
	bufcond = g_cond_new();

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		g_printerr(_("Cannot initialize SDL audio subsystem.\n"));
		return (1);
	}

	curfmt.format = AUDIO_S16LSB,
	curfmt.callback = audio_output_write;

	return (0);
}

int
audio_output_play(struct audio_file *fd)
{
	int rlen;

	g_mutex_lock(buflock);
	if (buflen != 0)
		g_cond_wait(bufcond, buflock);
	g_mutex_unlock(buflock);

	if ((rlen = buflen = audio_file_read(fd, buf)) == 0)
		return (0);

	if (curfmt.freq != fd->srate || curfmt.channels != fd->channels) {
		audio_output_close();

		curfmt.freq = fd->srate;
		curfmt.channels = fd->channels;
		curfmt.samples = AUDIO_OUTPUT_BUFLEN /
		    (2 * curfmt.channels);

		if (SDL_OpenAudio(&curfmt, NULL) != 0) {
			gui_msgbar_warn(_("Cannot open the audio device."));
			return (0);
		}

		g_assert(curfmt.size == AUDIO_OUTPUT_BUFLEN);
	}

	SDL_PauseAudio(0);

	return (rlen);
}

void
audio_output_close(void)
{
	SDL_CloseAudio();
}
