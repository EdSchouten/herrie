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
 * @file audio_format_aac.c
 * @brief AAC decompression routines.
 */

#include "stdinc.h"

#include <neaacdec.h>

#include "audio_format.h"
#include "audio_output.h"

/**
 * @brief Private AAC data stored in the audio file structure.
 */
struct aac_drv_data {
	NeAACDecHandle		decoder;
	NeAACDecFrameInfo	curframe;
	size_t			buflen;
	unsigned char		buf_input[65536];
};

int
aac_open(struct audio_file *fd, const char *ext)
{
	struct aac_drv_data *data;
	NeAACDecConfigurationPtr cfg;
	long srate;
	char channels;

	if (fd->stream) {
		/* Not yet */
		return (-1);
	}

	data = g_slice_new0(struct aac_drv_data);
	data->decoder = NeAACDecOpen();

	/* Set output correctly */
	cfg = NeAACDecGetCurrentConfiguration(data->decoder);
	cfg->outputFormat = FAAD_FMT_16BIT;
	NeAACDecSetConfiguration(data->decoder, cfg);

	data->buflen = fread(data->buf_input, 1,
	    sizeof data->buf_input, fd->fp);
	if (data->buflen == 0)
		goto bad;
	if (NeAACDecInit(data->decoder, data->buf_input, data->buflen,
	    &srate, &channels) < 0)
		goto bad;

	fd->srate = srate;
	fd->channels = channels;

	fd->drv_data = data;
	return (0);

bad:	NeAACDecClose(data->decoder);
	g_slice_free(struct aac_drv_data, data);
	return (-1);
}

void
aac_close(struct audio_file *fd)
{
}

#include "gui.h"

size_t
aac_read(struct audio_file *fd, void *buf)
{
	struct aac_drv_data *data = fd->drv_data;
	size_t left = AUDIO_OUTPUT_BUFLEN;
	void *rbuf;
	NeAACDecFrameInfo fr;
	GString *str;

	do {
		rbuf = NeAACDecDecode(data->decoder, &fr,
		    data->buf_input, sizeof data->buf_input);
		if (rbuf == NULL || fr.bytesconsumed == 0 || fr.error != 0) {
			/* Refill buffer */
			data->buflen = fread(data->buf_input, 1,
			    sizeof data->buf_input, fd->fp);
			if (data->buflen == 0)
				return (0);
			continue;
		}
		str = g_string_new("");
		g_string_printf(str, "Samples: %lu\n", fr.samples);
		gui_msgbar_warn(str->str);
		g_string_free(str, TRUE);
		return (0);
	} while (left > 0);

	return (1);
}

void
aac_seek(struct audio_file *fd, int len, int rel)
{
}
