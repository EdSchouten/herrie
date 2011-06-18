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
 * @file audio_format_sndfile.c
 * @brief libsndfile decompression routines.
 */

#include "stdinc.h"

#include <sndfile.h>

#include "audio_file.h"
#include "audio_format.h"
#include "audio_output.h"

int
sndfile_open(struct audio_file *fd, const char *ext)
{
	SNDFILE *hnd;
	SF_INFO info;
	int fno;

	if (fd->stream) {
		/* Not yet */
		return (-1);
	}

	/* Rewind the file to the beginning */
	fno = fileno(fd->fp);
	lseek(fno, 0, SEEK_SET);

	if ((hnd = sf_open_fd(fno, SFM_READ, &info, 0)) == NULL)
		return (-1);
	fd->drv_data = (void *)hnd;

	fd->srate = info.samplerate;
	fd->channels = info.channels;
	fd->time_len = info.frames / fd->srate;

	/* Metadata - libsndfile only has artist + title */
	fd->artist = g_strdup(sf_get_string(hnd, SF_STR_ARTIST));
	fd->title = g_strdup(sf_get_string(hnd, SF_STR_TITLE));
	fd->album = g_strdup(sf_get_string(hnd, SF_STR_ALBUM));

	return (0);
}

void
sndfile_close(struct audio_file *fd)
{
	SNDFILE *hnd = fd->drv_data;
	sf_close(hnd);
}

size_t
sndfile_read(struct audio_file *fd, int16_t *buf, size_t len)
{
	SNDFILE *hnd = fd->drv_data;
	sf_count_t ret, frame;
	
	ret = sf_read_short(hnd, buf, len);

	/* Seek zero frames to obtain the current position */
	frame = sf_seek(hnd, 0, SEEK_CUR);
	fd->time_cur = frame / fd->srate;

	return (ret);
}

void
sndfile_seek(struct audio_file *fd, int len, int rel)
{
	SNDFILE *hnd = fd->drv_data;
	sf_count_t frame;

	/* Make sure we always align the seek to seconds */
	if (rel)
		len += fd->time_cur;
	len = CLAMP(len, 0, (int)fd->time_len);

	frame = sf_seek(hnd, len * fd->srate, SEEK_SET);
	fd->time_cur = frame / fd->srate;
}
