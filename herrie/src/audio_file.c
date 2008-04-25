/*
 * Copyright (c) 2006-2008 Ed Schouten <ed@80386.nl>
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
 * @file audio_file.c
 * @brief Generic access and decoding of audio file formats.
 */

#include "stdinc.h"

#include "audio_file.h"
#include "audio_format.h"
#include "scrobbler.h"
#include "vfs.h"

/**
 * @brief Audio format containg its functions to open, close, read and
 *        seek the format.
 */
struct audio_format {
	/**
	 * @brief The format's open call.
	 */
	int	(*open)(struct audio_file *fd, const char *ext);
	/**
	 * @brief The format's close call.
	 */
	void	(*close)(struct audio_file *fd);
	/**
	 * @brief The format's read call.
	 */
	size_t	(*read)(struct audio_file *fd, int16_t *buf, size_t len);
	/**
	 * @brief The format's seek call.
	 */
	void	(*seek)(struct audio_file *fd, int len, int rel);
};

/*
 * Make sure you add the most strict matching formats at the top. Raw or
 * headerless file formats should be listed at the bottom, as the
 * matching code just runs through this list.
 */
/**
 * @brief List of audio formats.
 */
static struct audio_format formats[] = {
#ifdef BUILD_VORBIS
	{ vorbis_open, vorbis_close, vorbis_read, vorbis_seek },
#endif /* !BUILD_VORBIS */
#ifdef BUILD_MP3
	{ mp3_open, mp3_close, mp3_read, mp3_seek },
#endif /* !BUILD_MP3 */
#ifdef BUILD_MODPLUG
	{ modplug_open, modplug_close, modplug_read, modplug_seek },
#endif /* !BUILD_MODPLUG */
#ifdef BUILD_SNDFILE
	/*
	 * Keep this entry at the bottom - it does evil stuff with raw
	 * file descriptors. It could also catch some raw formats.
	 */
	{ sndfile_open, sndfile_close, sndfile_read, sndfile_seek },
#endif /* !BUILD_SNDFILE */
};
/**
 * @brief Amount of audio formats.
 */
#define NUM_FORMATS (sizeof formats / sizeof(struct audio_format))

struct audio_file *
audio_file_open(const struct vfsref *vr)
{
	const char *ext;
	struct audio_file *out = NULL;
	unsigned int i;

	out = g_slice_new0(struct audio_file);
	
	out->fp = vfs_open(vr);
	if (out->fp == NULL)
		goto bad;

	/* Store file extension */
	if ((ext = strrchr(vfs_filename(vr), '.')) != NULL)
		ext++;

	for (i = 0; i < NUM_FORMATS; i++) {
		if (fseek(out->fp, 0, SEEK_SET) != 0)
			out->stream = 1;

		if (formats[i].open(out, ext) == 0) {
			/* Assign the format to the file */
			out->drv = &formats[i];
			break;
		}
	}

	if (out->drv == NULL) {
		/* No matched format */
		fclose(out->fp);
		goto bad;
	}

	/* No tag - just use the display name then */
	if (out->title == NULL)
		out->title = g_strdup(vfs_name(vr));

	return (out);

bad:
	g_slice_free(struct audio_file, out);
	return (NULL);
}

void
audio_file_close(struct audio_file *fd)
{
	fd->drv->close(fd);
	if (fd->fp != NULL)
		fclose(fd->fp);

	g_free(fd->artist);
	g_free(fd->title);
#ifdef BUILD_SCROBBLER
	g_free(fd->album);
#endif /* BUILD_SCROBBLER */
	g_slice_free(struct audio_file, fd);
}

size_t
audio_file_read(struct audio_file *fd, int16_t *buf, size_t len)
{
	size_t ret;

	ret = fd->drv->read(fd, buf, len);
#ifdef BUILD_SCROBBLER
	scrobbler_notify_read(fd, (ret == 0));
#endif /* BUILD_SCROBBLER */

	return (ret);
}

void
audio_file_seek(struct audio_file *fd, int len, int rel)
{
	g_assert(len != 0 || rel == 0);

	if (!fd->stream) {
		fd->drv->seek(fd, len, rel);
#ifdef BUILD_SCROBBLER
		scrobbler_notify_seek(fd);
#endif /* BUILD_SCROBBLER */
	}
}
