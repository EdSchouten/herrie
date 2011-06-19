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
 * @file audio_format_vorbis.c
 * @brief Ogg Vorbis decompression routines.
 */

#include "stdinc.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include "audio_file.h"
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
	const char *tag;

	if ((cmt = ov_comment(vfp, -1)) == NULL)
		return;

	for (i = 0; i < cmt->comments; i++) {
		tag = cmt->user_comments[i];

		if (g_ascii_strncasecmp(tag, "artist=", 7) == 0)
			fd->artist = g_strdup(tag + 7);
		else if (g_ascii_strncasecmp(tag, "title=", 6) == 0)
			fd->title = g_strdup(tag + 6);
#ifdef BUILD_SCROBBLER
		else if (g_ascii_strncasecmp(tag, "album=", 6) == 0)
			fd->album = g_strdup(tag + 6);
#endif /* BUILD_SCROBBLER */
	}
}

int
vorbis_open(struct audio_file *fd, const char *ext)
{
	OggVorbis_File *vfp;
	vorbis_info *info;

	if (fd->stream) {
		/* Not yet */
		return (-1);
	}

	vfp = g_slice_new(OggVorbis_File);
	if (ov_open(fd->fp, vfp, NULL, 0) != 0) {
		g_slice_free(OggVorbis_File, vfp);
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
	g_slice_free(OggVorbis_File, vfp);

	/* libvorbisfile already closes the file for us */
	fd->fp = NULL;
}

size_t
vorbis_read(struct audio_file *fd, int16_t *buf, size_t len)
{
	OggVorbis_File *vfp = fd->drv_data;
	size_t ret = 0;
	long rlen;
	char *out = (char *)buf;

	len *= sizeof(int16_t);

	/* Return 16 bits signed native endian */
	while (ret < len) {
#if G_BYTE_ORDER == G_BIG_ENDIAN
		rlen = ov_read(vfp, out + ret, len - ret, 1, 2, 1, NULL);
#else /* G_BYTE_ORDER != G_BIG_ENDIAN */
		rlen = ov_read(vfp, out + ret, len - ret, 0, 2, 1, NULL);
#endif /* G_BYTE_ORDER == G_BIG_ENDIAN */
		if (rlen <= 0)
			break;
		ret += rlen;
	}
	fd->time_cur = ov_time_tell(vfp);

	return (ret / sizeof(int16_t));
}

void
vorbis_seek(struct audio_file *fd, int len, int rel)
{
	double npos;
	OggVorbis_File *vfp = fd->drv_data;

	npos = len;
	if (rel) {
		/* Perform a relative seek. */
		npos += ov_time_tell(vfp);
	}

	npos = CLAMP(npos, 0, ov_time_total(vfp, -1));

	ov_time_seek(vfp, npos);
	fd->time_cur = ov_time_tell(vfp);
}
