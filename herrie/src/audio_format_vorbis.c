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
 */
/**
 * @file audio_format_vorbis.c
 */

#include <glib.h>

#include <string.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "audio_format.h"
#include "audio_output.h"

/**
 * @brief Read tags from Ogg Vorbis file and store them in the audio
 *        file handle.
 */
static void
vorbis_read_comments(struct audio_file *fd)
{
	OggVorbis_File *vfp = fd->drv_data;
	struct vorbis_comment *cmt;
	int i;
	char *tag, *value;

	if ((cmt = ov_comment(vfp, -1)) == NULL)
		return;
	
	for (i = 0; i < cmt->comments; i++) {
		tag = cmt->user_comments[i];
		value = strchr(tag, '=');
		if (value == NULL) {
			/* No foo=bar format */
			continue;
		} else {
			/* Split at the '=' character */
			*value++ = '\0';
		}

		if (strcasecmp(tag, "artist") == 0)
			fd->tag.artist = value;
		else if (strcasecmp(tag, "title") == 0)
			fd->tag.title = value;
		else if (strcasecmp(tag, "album") == 0)
			fd->tag.album = value;
	}
}

int
vorbis_open(struct audio_file *fd)
{
	OggVorbis_File *vfp;
	vorbis_info *info;

	vfp = g_malloc(sizeof(OggVorbis_File));
	if (ov_open(fd->fp, vfp, NULL, 0) != 0) {
		g_free(vfp);
		return (-1);
	}

	info = ov_info(vfp, -1);

	fd->drv_data = vfp;
	fd->srate = info->rate;
	fd->channels = 2; /* XXX */
	fd->time_len = ov_time_total(vfp, -1);

	vorbis_read_comments(fd);
	
	return (0);
}

void
vorbis_close(struct audio_file *fd)
{
	OggVorbis_File *vfp = fd->drv_data;

	ov_clear(vfp);
	g_free(vfp);

	/* libvorbisfile already closes the file for us */
	fd->fp = NULL;
}

size_t
vorbis_read(struct audio_file *fd, void *buf)
{
	OggVorbis_File *vfp = fd->drv_data;
	size_t ret;

	/* Return 16 bits signed little endian */
	ret = ov_read(vfp, buf, AUDIO_OUTPUT_BUFLEN, 0, 2, 1, NULL);
	fd->time_cur = ov_time_tell(vfp);

	return (ret);
}

void
vorbis_seek(struct audio_file *fd, int len)
{
	double npos;
	OggVorbis_File *vfp = fd->drv_data;

	npos = ov_time_tell(vfp) + len;
	npos = CLAMP(npos, 0, ov_time_total(vfp, -1));

	ov_time_seek(vfp, npos);
}
