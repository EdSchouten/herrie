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
 * @file audio_output_ao.c
 * @brief libao audio output driver.
 */

#include "stdinc.h"

#include <ao/ao.h>

#include "audio_file.h"
#include "audio_output.h"
#include "config.h"
#include "gui.h"

/**
 * @brief Handle to an audio device handle if one has already been opened.
 */
static ao_device*	devptr = NULL;
/**
 * @brief Format of the current open audio device handle.
 */
static ao_sample_format	devfmt;
/**
 * @brief Default options that should be prepended when opening the
 *        audio device.
 */
static ao_option	*devopt = NULL;

int
audio_output_open(void)
{
	const char *host;
	char *henv, *hend;

	ao_initialize();

	/* We always expect 16 bits native endian PCM */
	devfmt.bits = 16;
	devfmt.byte_format = AO_FMT_NATIVE;

	host = config_getopt("audio.output.ao.host");
	if (strcmp(host, "env_ssh") == 0) {
		/* Fetch "host" option from SSH_CLIENT variable */
		henv = g_strdup(getenv("SSH_CLIENT"));
		if (henv != NULL) {
			/* Remove the trailing port number */
			if ((hend = strchr(henv, ' ')) != NULL)
				*hend = '\0';
			ao_append_option(&devopt, "host", henv);
			g_free(henv);
		}
	} else if (host[0] != '\0') {
		/* Use the value as the hostname */
		ao_append_option(&devopt, "host", host);
	}

	return (0);
}

int
audio_output_play(struct audio_file *fd)
{
	const char *drvname;
	int16_t buf[2048];
	int len, drvnum;

	if ((len = audio_file_read(fd, buf, sizeof buf / sizeof(int16_t))) == 0)
		return (-1);

	if ((unsigned int)devfmt.rate != fd->srate ||
	    (unsigned int)devfmt.channels != fd->channels) {
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

		devptr = ao_open_live(drvnum, &devfmt, devopt);
		if (devptr == NULL) {
			gui_msgbar_warn(_("Cannot open the audio device."));
			return (-1);
		}
	}

	if (ao_play(devptr, (char *)buf, len * sizeof(int16_t)) == 0) {
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
		ao_close(devptr);
		devptr = NULL;
	}
}
