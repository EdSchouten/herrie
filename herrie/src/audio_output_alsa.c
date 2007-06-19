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
 * @file audio_output_alsa.c
 * @brief ALSA audio output driver.
 */

#include "stdinc.h"

#include <alsa/asoundlib.h>

#include "audio_file.h"
#include "audio_output.h"
#include "config.h"
#include "gui.h"

/**
 * @brief Handle to the audio device obtained from ALSA.
 */
static snd_pcm_t		*devhnd;
/**
 * @brief Hardware parameters from the audio device as read obtained the
 *        ALSA.
 */
static snd_pcm_hw_params_t	*devparam;
static unsigned int		channels = 0;
static unsigned int		srate = 0;

static int
audio_output_apply_hwparams(void)
{
	/* Acquire the standard parameters */
	if (snd_pcm_hw_params_any(devhnd, devparam) != 0)
		return (-1);

	/* Set the access method - XXX: mmap */
	if (snd_pcm_hw_params_set_access(devhnd, devparam,
	    SND_PCM_ACCESS_RW_INTERLEAVED) != 0)
		return (-1);

	/* Output format */
	if (snd_pcm_hw_params_set_format(devhnd, devparam,
	    SND_PCM_FORMAT_S16_LE) != 0)
		return (-1);
	/* Sample rate */
	if (snd_pcm_hw_params_set_rate_near(devhnd, devparam, &srate, NULL) != 0)
		return (-1);
	/* Channels */
	if (snd_pcm_hw_params_set_channels(devhnd, devparam, channels) != 0)
		return (-1);

	/* Apply values */
	if (snd_pcm_hw_params(devhnd, devparam) != 0)
		return (-1);
	/* And we're off! */
	if (snd_pcm_prepare(devhnd) != 0)
		return (-1);
	
	return (0);
}

int
audio_output_open(void)
{
	/* Open the device */
	if (snd_pcm_open(&devhnd,
	    config_getopt("audio.output.alsa.device"),
	    SND_PCM_STREAM_PLAYBACK, 0) != 0)
		goto error;

	/* Retreive the hardware parameters */
	if (snd_pcm_hw_params_malloc(&devparam) != 0)
		goto error;

	return (0);
error:
	g_printerr(_("Cannot open the audio device.\n"));
	return (-1);
}

int
audio_output_play(struct audio_file *fd)
{
	char buf[16384];
	size_t bps;
	snd_pcm_sframes_t ret, len, done = 0;

	if ((len = audio_file_read(fd, buf, sizeof buf)) == 0)
		return (-1);
	
	if (fd->channels != channels || fd->srate != srate) {
		/* Reset the stream */
		snd_pcm_drain(devhnd);

		/* Apply the new values */
		channels = fd->channels;
		srate = fd->srate;
		if (audio_output_apply_hwparams() != 0) {
			gui_msgbar_warn(_("Sample rate or amount of channels not supported."));
			return (-1);
		}
	}

	/* ALSA measures in sample lengths */
	bps = fd->channels * sizeof(short);
	len /= bps;

	/* Our complex error handling for snd_pcm_writei() */
	while (done < len) {
		ret = snd_pcm_writei(devhnd, buf + (done * bps), len - done);
		if (ret == -EPIPE) {
			/* Buffer underrun. */
			if (snd_pcm_prepare(devhnd) != 0)
				return (-1);
			continue;
		} else if (ret <= 0) {
			/* Some other strange error. */
			return (-1);
		} else {
			done += ret;
		}
	}

	return (0);
}

void
audio_output_close(void)
{
	snd_pcm_drain(devhnd);
	snd_pcm_close(devhnd);
	snd_pcm_hw_params_free(devparam);
}
