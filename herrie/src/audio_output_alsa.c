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
 * @brief the amount of channels that is currently used for playback.
 */
static unsigned int		channels = 0;
/**
 * @brief the sample rate that is currently used for playback.
 */
static unsigned int		srate = 0;
#ifdef BUILD_VOLUME
/**
 * @brief Handle to the ALSA mixer.
 */
static snd_mixer_t *mixhnd = NULL;
/**
 * @brief ALSA mixer element for the audio device.
 */
static snd_mixer_elem_t *elem;
#endif /* BUILD_VOLUME */

/**
 * @brief Alter the audio output parameters of the audio output device.
 */
static int
audio_output_apply_hwparams(void)
{
	snd_pcm_hw_params_t *devparam;

	snd_pcm_hw_params_alloca(&devparam);

	/* Acquire the standard parameters */
	if (snd_pcm_hw_params_any(devhnd, devparam) != 0)
		return (-1);

	/* Set the access method - XXX: mmap */
	if (snd_pcm_hw_params_set_access(devhnd, devparam,
	    SND_PCM_ACCESS_RW_INTERLEAVED) != 0)
		return (-1);

	/* Output format */
	if (snd_pcm_hw_params_set_format(devhnd, devparam,
	    SND_PCM_FORMAT_S16) != 0)
		return (-1);
	/* Sample rate */
	if (snd_pcm_hw_params_set_rate_near(devhnd, devparam, &srate, NULL) != 0)
		return (-1);
	/* Channels */
	if (snd_pcm_hw_params_set_channels(devhnd, devparam, channels) != 0)
		return (-1);

	/* Drain current data and make sure we aren't underrun */
	snd_pcm_drain(devhnd);

	/* Apply values */
	if (snd_pcm_hw_params(devhnd, devparam) != 0)
		return (-1);
	
	return (0);
}

#ifdef BUILD_VOLUME
/**
 * @brief Open the ALSA mixer device, making it possible to adjust the
 *        volume.
 */
static void
audio_output_volume_open(void)
{
	snd_mixer_selem_id_t *mixer;

	/* Open mixer device */
	if (snd_mixer_open(&mixhnd, 0) != 0)
		return;
	if (snd_mixer_attach(mixhnd,
	    config_getopt("audio.output.alsa.device")) != 0)
		goto bad;
	if (snd_mixer_selem_register(mixhnd, NULL, NULL) < 0)
		goto bad;
	if (snd_mixer_load(mixhnd) < 0)
		goto bad;

	/* Obtain mixer element */
	snd_mixer_selem_id_alloca(&mixer);
	snd_mixer_selem_id_set_name(mixer, config_getopt("audio.output.alsa.mixer"));

	elem = snd_mixer_find_selem(mixhnd, mixer);
	if (elem != NULL)
		return;
bad:
	snd_mixer_close(mixhnd);
	mixhnd = NULL;
}
#endif /* BUILD_VOLUME */

int
audio_output_open(void)
{
	/* Open the device */
	if (snd_pcm_open(&devhnd, config_getopt("audio.output.alsa.device"),
	    SND_PCM_STREAM_PLAYBACK, 0) != 0)
		goto error;

#ifdef BUILD_VOLUME
	audio_output_volume_open();
#endif /* BUILD_VOLUME */

	return (0);
error:
	g_printerr("%s\n", _("Cannot open the audio device."));
	return (-1);
}

int
audio_output_play(struct audio_file *fd)
{
	int16_t buf[8192];
	snd_pcm_sframes_t ret, len, done = 0;

	if ((len = audio_file_read(fd, buf, sizeof buf / sizeof(int16_t))) == 0)
		return (-1);
	
	if (fd->channels != channels || fd->srate != srate) {
		/* Apply the new values */
		channels = fd->channels;
		srate = fd->srate;
		if (audio_output_apply_hwparams() != 0) {
			gui_msgbar_warn(_("Sample rate or amount of channels not supported."));
			/* Invalidate the old settings */
			srate = 0;
			return (-1);
		}
	}

	/* ALSA measures in sample lengths */
	len /= fd->channels;

	/* Our complex error handling for snd_pcm_writei() */
	while (done < len) {
		ret = snd_pcm_writei(devhnd, buf + (done * fd->channels), len - done);
		if (ret == -EPIPE) {
			/* Buffer underrun. Try again. */
			if (snd_pcm_prepare(devhnd) != 0)
				return (-1);
			continue;
		} else if (ret <= 0) {
			/* Some other strange error. */
			return (-1);
		}

		done += ret;
	}

	return (0);
}

void
audio_output_close(void)
{
	snd_pcm_close(devhnd);
#ifdef BUILD_VOLUME
	if (mixhnd != NULL)
		snd_mixer_close(mixhnd);
#endif /* BUILD_VOLUME */
}

#ifdef BUILD_VOLUME
/**
 * @brief Adjust the audio output by a certain percentage and return the
 *        new value.
 */
static int
audio_output_volume_adjust(int n)
{
	int delta;
	long min, max;
	long right, left;

	if (mixhnd == NULL)
		return (-1);

	/* Allow other applications to change mixer */
	snd_mixer_handle_events(mixhnd);

	/* Obtain volume range boundaries */
	if (snd_mixer_selem_get_playback_volume_range(elem, &min, &max) != 0)
		return (-1);
	if (min >= max)
		return (-1);

	/* Convert delta from percent to steps */
	delta = (n * (max - min)) / 100;
	if (delta == 0)
		delta = n > 0 ? 1 : -1;

	/* Obtain current volume */
	if (snd_mixer_selem_get_playback_volume(elem,
	    SND_MIXER_SCHN_FRONT_LEFT, &left) != 0)
		return (-1);
	if (snd_mixer_selem_get_playback_volume(elem,
	    SND_MIXER_SCHN_FRONT_RIGHT, &right) != 0)
		return (-1);

	left = left + delta;
	left = CLAMP(left, min, max);
	right = right + delta;
	right = CLAMP(right, min, max);

	/* Set new volume */
	if (snd_mixer_selem_set_playback_volume(elem,
	    SND_MIXER_SCHN_FRONT_LEFT, left) != 0)
		return (-1);
	if (snd_mixer_selem_set_playback_volume(elem,
	    SND_MIXER_SCHN_FRONT_RIGHT, right) != 0)
		return (-1);

	/* Convert volume back to a percentage */
	return (((left + right) / 2 - min) * 100) / (max - min);
}

int
audio_output_volume_up(void)
{
	return audio_output_volume_adjust(4);
}

int
audio_output_volume_down(void)
{
	return audio_output_volume_adjust(-4);
}
#endif /* BUILD_VOLUME */
