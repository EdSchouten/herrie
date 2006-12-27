/*
 * Copyright (c) 2006 Ed Schouten <ed@fxq.nl>
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
 *
 * $Id: audio_output_oss.c 1040 2006-12-20 22:12:28Z ed $
 */
/**
 * @file audio_output_oss.c
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <glib.h>

#include <sys/ioctl.h>
#ifdef __OpenBSD__
#include <soundcard.h>
#else /* !__OpenBSD__ */
#include <sys/soundcard.h>
#endif /* __OpenBSD__ */

#include "audio_output.h"
#include "config.h"
#include "trans.h"

/**
 * @brief File descriptor of the audio device handle.
 */
static int dev_fd;
/**
 * @brief Sample rate of the audio device handle.
 */
static long cur_srate = -1;
/**
 * @brief Amount of channels of the audio device handle.
 */
static long cur_channels = -1;

int
audio_output_open(void)
{
	int dat;
	const char *dev;

	dev = config_getopt("audio.output.oss.device");
	if ((dev_fd = open(dev, O_WRONLY)) == -1) {
		g_printerr(_("Cannot open audio device \"%s\".\n"), dev);
		return (-1);
	}
	
	/* 16 bits little endian stereo */
	dat = AFMT_S16_LE;
	ioctl (dev_fd, SNDCTL_DSP_SETFMT, &dat);

	return (0);
}

int
audio_output_play(struct audio_file *fd)
{
	char buf[AUDIO_OUTPUT_BUFLEN];
	int len;

	if ((len = audio_read(fd, buf)) == 0)
		return (0);

	if (cur_srate != fd->srate || cur_channels != fd->channels) {
		/* Our settings have been altered */
		ioctl (dev_fd, SNDCTL_DSP_RESET, NULL);

		if (cur_srate != fd->srate) {
			/* Reset the sample rate */
			if (ioctl (dev_fd, SNDCTL_DSP_SPEED,
			    &fd->srate) != -1)
				cur_srate = fd->srate;
		}

		if (cur_channels != fd->channels) {
			/* Reset the number of channels rate */
			if (ioctl (dev_fd, SNDCTL_DSP_CHANNELS,
			    &fd->channels) != -1)
				cur_channels = fd->channels;
		}
	}

	return write(dev_fd, buf, len);
}

void
audio_output_close(void)
{
	close (dev_fd);
}
