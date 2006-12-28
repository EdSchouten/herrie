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
 * @file audio_output_ao.c
 */

#include <ao/ao.h>

#include "audio_output.h"
#include "config.h"

/**
 * @brief Handle to an audio device handle if one has already been opened.
 */
static ao_device*	devptr = NULL;
/**
 * @brief Format of the current open audio device handle.
 */
static ao_sample_format	devfmt;

int
audio_output_open(void)
{
	ao_initialize();

	/* We always expect 16 bits little endian PCM */
	devfmt.bits = 16;
	devfmt.byte_format = AO_FMT_LITTLE;

	return (0);
}

int
audio_output_play(struct audio_file *fd)
{
	const char *drvname;
	char buf[AUDIO_OUTPUT_BUFLEN];
	int len, drvnum;

	if ((len = audio_read(fd, buf)) == 0)
		return (0);

	if (devfmt.rate != fd->srate || devfmt.channels != fd->channels) {
		/* Sample rate or amount of channels has changed */
		audio_output_close();

		devfmt.rate = fd->srate;
		devfmt.channels = fd->channels;
	}

	if (devptr == NULL) {
		drvname = config_getopt("audio.output.ao.driver");
		if (drvname[0] != '\0') {
			/* User specified driver */
			drvnum = ao_driver_id(drvname);
		} else {
			/* Default driver */
			drvnum = ao_default_driver_id();
		}

		devptr = ao_open_live(drvnum, &devfmt, NULL);
		if (devptr == NULL)
			return (0);
	}

	if (ao_play(devptr, buf, len) == 0) {
		/* No success - device must be closed */
		audio_output_close();
		return (0);
	} else {
		return (len);
	}
}

void
audio_output_close(void)
{
	if (devptr != NULL) {
		ao_close(devptr);
		devptr = NULL;
	}
}
