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
 * @file audio_format_modplug.c
 */

#include <sys/mman.h>
#include <modplug.h>

#include "audio_format.h"
#include "audio_output.h"

struct modplug_drv_data {
	void		*map_base;
	size_t		map_len;
	ModPlugFile	*modplug;
};

int
modplug_open(struct audio_file *fd)
{
	struct modplug_drv_data *data;

	if (fd->stream) {
		/* If only we could memory map the internet... */
		return (-1);
	}

	fd->drv_data = data = g_slice_new(struct modplug_drv_data);

	/* Calculate file length */
	fseek(fd->fp, 0, SEEK_END);
	data->map_len = ftello(fd->fp);

	/* Memory map the entire file */
	data->map_base = mmap(NULL, data->map_len, PROT_READ, 0,
	    fileno(fd->fp), 0);
	if (data->map_base == MAP_FAILED)
		goto free;

	/* Now feed it to modplug */
	data->modplug = ModPlug_Load(data->map_base, data->map_len);
	if (data->modplug != NULL) {
		fd->time_len = ModPlug_GetLength(data->modplug) / 1000;
		fd->tag.title = g_strdup(ModPlug_GetName(data->modplug));
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
modplug_read(struct audio_file *fd, void *buf)
{
	struct modplug_drv_data *data = fd->drv_data;

	return ModPlug_Read(data->modplug, buf, AUDIO_OUTPUT_BUFLEN);
}

void
modplug_seek(struct audio_file *fd, int len, int rel)
{
}
