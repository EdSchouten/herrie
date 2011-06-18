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
 * @file audio_output_oss.c
 * @brief OSS audio output driver.
 */

#include "stdinc.h"

#include <sys/ioctl.h>
#include OSS_HEADER

#include "audio_file.h"
#include "audio_output.h"
#include "config.h"
#include "gui.h"

#ifndef SNDCTL_DSP_GETPLAYVOL
#define BUILD_OSS3_MIXER
#endif /* !SNDCTL_DSP_GETPLAYVOL */

/**
 * @brief File descriptor of the audio device handle.
 */
static int dev_fd;
#if defined(BUILD_VOLUME) && defined(BUILD_OSS3_MIXER)
/**
 * @brief File descriptor of the mixer device.
 */
static int mix_fd;
#endif /* BUILD_VOLUME && BUILD_OSS3_MIXER */
/**
 * @brief Sample rate of the audio device handle.
 */
static unsigned int cur_srate = 0;
/**
 * @brief Amount of channels of the audio device handle.
 */
static unsigned int cur_channels = 0;

int
audio_output_open(void)
{
	const char *dev;

	/* Open the audio device */
	dev = config_getopt("audio.output.oss.device");
	if ((dev_fd = open(dev, O_WRONLY)) == -1) {
		g_printerr(_("Cannot open audio device \"%s\".\n"), dev);
		return (-1);
	}

#if defined(BUILD_VOLUME) && defined(BUILD_OSS3_MIXER)
	/* Open the mixer device */
	dev = config_getopt("audio.output.oss.mixer");
	mix_fd = open(dev, O_RDWR);
#endif /* BUILD_VOLUME && BUILD_OSS3_MIXER */

	return (0);
}

int
audio_output_play(struct audio_file *fd)
{
	int16_t buf[2048];
	int len;
	int fmt;

	if ((len = audio_file_read(fd, buf, sizeof buf / sizeof(int16_t))) == 0)
		return (-1);

	if (cur_srate != fd->srate || cur_channels != fd->channels) {
		/* Our settings have been altered */
		ioctl(dev_fd, SNDCTL_DSP_RESET, NULL);

		/* 16 bits native endian stereo */
		fmt = AFMT_S16_NE;
		if (ioctl(dev_fd, SNDCTL_DSP_SETFMT, &fmt) == -1)
			goto bad;

		/* Reset the sample rate */
		if (ioctl(dev_fd, SNDCTL_DSP_SPEED, &fd->srate) == -1)
			goto bad;

		/* Reset the number of channels rate */
		if (ioctl(dev_fd, SNDCTL_DSP_CHANNELS, &fd->channels) == -1)
			goto bad;

		/* Both succeeded */
		cur_srate = fd->srate;
		cur_channels = fd->channels;
	}

	len *= sizeof(int16_t);
	if (write(dev_fd, buf, len) != len)
		return (-1);

	return (0);
bad:
	gui_msgbar_warn(_("Sample rate or amount of channels not supported."));
	/* Invalidate the old sample rate setting to force reconfiguration */
	cur_srate = 0;
	return (-1);
}

void
audio_output_close(void)
{
	close(dev_fd);
}

#ifdef BUILD_VOLUME
/**
 * @brief Adjust the audio output by a certain percentage and return the
 *        new value.
 */
static int
audio_output_volume_adjust(int n)
{
	int vol, out;

#ifdef BUILD_OSS3_MIXER
	if (mix_fd < 0 ||
	    ioctl(mix_fd, MIXER_READ(SOUND_MIXER_VOLUME), &vol) == -1)
#else
	if (ioctl(dev_fd, SNDCTL_DSP_GETPLAYVOL, &vol) == -1)
#endif
		return (-1);

	/* XXX: Merge left and right */
	vol = ((vol & 0x7f) + ((vol >> 8) & 0x7f)) / 2;
	vol = CLAMP(vol + n, 0, 100);
	out = (vol << 8) | vol;

#ifdef BUILD_OSS3_MIXER
	if (ioctl(mix_fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &out) == -1)
#else
	if (ioctl(dev_fd, SNDCTL_DSP_SETPLAYVOL, &out) == -1)
#endif
		return (-1);

	return (vol);
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
