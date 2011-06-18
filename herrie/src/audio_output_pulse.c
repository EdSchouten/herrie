/*
 * Copyright (c) 2006-2011 Ed Schouten <ed@80386.nl>
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
 * @file audio_output_pulse.c
 * @brief PulseAudio audio output driver.
 */

#include "stdinc.h"

#include <pulse/simple.h>

#include "audio_file.h"
#include "audio_output.h"
#include "gui.h"

/**
 * @brief Handle to an audio device handle if one has already been opened.
 */
static pa_simple*	devptr = NULL;
/**
 * @brief Format of the current open audio device handle.
 */
static pa_sample_spec	devfmt = { PA_SAMPLE_S16LE, 0, 0 };

int
audio_output_open(void)
{
	return (0);
}

int
audio_output_play(struct audio_file *fd)
{
	int16_t buf[2048];
	size_t len;

	if ((len = audio_file_read(fd, buf, sizeof buf / sizeof(int16_t))) == 0)
		return (-1);

	if (devfmt.rate != fd->srate || devfmt.channels != fd->channels) {
		/* Sample rate or amount of channels has changed */
		audio_output_close();

		devfmt.rate = fd->srate;
		devfmt.channels = fd->channels;
	}

	if (devptr == NULL) {
		/* Open the device */
		devptr = pa_simple_new(NULL, APP_NAME,
		    PA_STREAM_PLAYBACK, NULL, "Audio output", &devfmt,
		    NULL, NULL, NULL);
		if (devptr == NULL) {
			gui_msgbar_warn(_("Cannot open the audio device."));
			return (-1);
		}
	}

	if (pa_simple_write(devptr, buf, len * sizeof(int16_t), NULL) != 0) {
		/* No success - device must be closed */
		audio_output_close();
		return (-1);
	}

	return (0);
}

void
audio_output_close(void)
{
	if (devptr != NULL) {
		/* Close device */
		pa_simple_free(devptr);
		devptr = NULL;
	}
}
