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
 * @file audio_format_modplug.c
 * @brief libmodplug decompression routines.
 */

#include "stdinc.h"

#include <sys/mman.h>
#include <modplug.h>

#include "audio_file.h"
#include "audio_format.h"
#include "audio_output.h"

/**
 * @brief Sample rate we want to use.
 */
#define SAMPLERATE	44100
/**
 * @brief The number of bytes used by a single frame.
 */
#define BYTESPERSAMPLE	4

/**
 * @brief Private data for libmodplug stored in the audio file structure.
 */
struct modplug_drv_data {
	/**
	 * @brief Base address of our memory map.
	 */
	void		*map_base;
	/**
	 * @brief Length of our memory map.
	 */
	size_t		map_len;
	/**
	 * @brief libmodplug handle.
	 */
	ModPlugFile	*modplug;
	/**
	 * @brief Decoded sample offset.
	 */
	off_t		sample;
};

/**
 * @brief Set proper parameters in the Modplug library.
 */
static void
modplug_init(void)
{
	ModPlug_Settings mset;

	ModPlug_GetSettings(&mset);
	mset.mChannels = 2;
	mset.mBits = 16;
	mset.mFrequency = SAMPLERATE;
	ModPlug_SetSettings(&mset);
}

int
modplug_open(struct audio_file *fd, const char *ext)
{
	struct modplug_drv_data *data;
	const char *title;

	/* libmodplug doesn't have good magic. Match by extension */
	if (ext == NULL)
		return (-1);
	if (g_ascii_strcasecmp(ext, "mod") != 0 &&
	    g_ascii_strcasecmp(ext, "s3m") != 0 &&
	    g_ascii_strcasecmp(ext, "it") != 0 &&
	    g_ascii_strcasecmp(ext, "xm") != 0)
		return (-1);

	/* If only we could memory map the internet... */
	if (fd->stream)
		return (-1);

	data = g_slice_new(struct modplug_drv_data);

	/* Calculate file length */
	fseek(fd->fp, 0, SEEK_END);
	data->map_len = ftello(fd->fp);

	/* Memory map the entire file */
	data->map_base = mmap(NULL, data->map_len, PROT_READ,
	    MAP_PRIVATE, fileno(fd->fp), 0);
	if (data->map_base == MAP_FAILED)
		goto free;

	/* Now feed it to modplug */
	modplug_init();
	data->modplug = ModPlug_Load(data->map_base, data->map_len);
	if (data->modplug != NULL) {
		data->sample = 0;
		fd->drv_data = data;
		fd->srate = SAMPLERATE;
		fd->channels = 2;

		fd->time_len = ModPlug_GetLength(data->modplug) / 1000;
		title = ModPlug_GetName(data->modplug);
		if (title != NULL && title[0] != '\0')
			fd->title = g_strdup(title);
		return (0);
	}

	munmap(data->map_base, data->map_len);
free:	g_slice_free(struct modplug_drv_data, data);
	return (1);
}

void
modplug_close(struct audio_file *fd)
{
	struct modplug_drv_data *data = fd->drv_data;

	ModPlug_Unload(data->modplug);
	munmap(data->map_base, data->map_len);
	g_slice_free(struct modplug_drv_data, data);
}

size_t
modplug_read(struct audio_file *fd, int16_t *buf, size_t len)
{
	struct modplug_drv_data *data = fd->drv_data;
	int rlen;

	rlen = ModPlug_Read(data->modplug, buf, len * sizeof(int16_t));
	data->sample += rlen / BYTESPERSAMPLE;
	fd->time_cur = data->sample / SAMPLERATE;

	return (rlen / sizeof(int16_t));
}

void
modplug_seek(struct audio_file *fd, int len, int rel)
{
	struct modplug_drv_data *data = fd->drv_data;
	size_t off;

	off = len * SAMPLERATE;
	if (rel)
		off += data->sample;
	/* Don't seek out of reach */
	off = CLAMP(off, 0, (fd->time_len - 1) * SAMPLERATE);

	ModPlug_Seek(data->modplug, (off * 10) / (SAMPLERATE / 100));
	data->sample = off;
	fd->time_cur = data->sample / SAMPLERATE;
}
