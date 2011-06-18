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
 * @file audio_format_mp3.c
 * @brief MP3 decompression routines.
 */

#include "stdinc.h"

#include <mad.h>
#include <id3tag.h>

#include "audio_file.h"
#include "audio_format.h"
#include "audio_output.h"

/**
 * @brief Private MP3 data stored in the audio file structure.
 */
struct mp3_drv_data {
	/**
	 * @brief Stream data.
	 */
	struct mad_stream	mstream;
	/**
	 * @brief Frame data.
	 */
	struct mad_frame	mframe;
	/**
	 * @brief Frame header.
	 */
	struct mad_header	mheader;
	/**
	 * @brief Synth data.
	 */
	struct mad_synth	msynth;
	/**
	 * @brief Progress timer.
	 */
	mad_timer_t		mtimer;
	/**
	 * @brief Sample offset.
	 */
	int			cursample;
	/**
	 * @brief Length of the file.
	 */
	off_t			flen;

	/**
	 * @brief Read buffer.
	 */
	unsigned char buf_input[65536];
};

/*
 * File and tag matching
 */

/**
 * @brief Test if an opened file is an MP3 file.
 */
static int
mp3_match(FILE *fp, const char *ext)
{
	unsigned char buf[3];
	int ret = 0;

	/* Also match (broken) *.mp3 files */
	if (ext != NULL && strcmp(ext, "mp3") == 0)
		goto done;

	if (fread(buf, sizeof buf, 1, fp) != 1) {
		ret = -1;
		goto done;
	}
	
	/* Match 1: first twelve bits high */
	if (buf[0] == 0xff && (buf[1] & 0xf0) == 0xf0)
		goto done;
	
	/* Match 2: ID3 header */
	if (buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3')
		goto done;

	ret = -1;

done:	rewind(fp);
	return (ret);
}

/**
 * @brief Read the ID3 tag from an MP3 file.
 */
static void
mp3_readtags(struct audio_file *fd)
{
	int tmpfd;
	unsigned int i;
	off_t orig_off;
	struct id3_file *id3f;
	struct id3_tag *tag;
	id3_ucs4_t const *str;
	char **dst = NULL;

	/* Duplicate the current filehandle */
	tmpfd = dup(fileno(fd->fp));
	if (tmpfd < 0)
		return;
	
	/* Backing up the original offset */
	orig_off = lseek(tmpfd, 0, SEEK_CUR);

	id3f = id3_file_fdopen(tmpfd, ID3_FILE_MODE_READONLY);
	if (id3f == NULL)
		goto bad;
	
	/* Here we go */
	tag = id3_file_tag(id3f);
	if (tag == NULL)
		goto done;

	for (i = 0; i < tag->nframes; i++) {
		if (strncmp("TPE1", tag->frames[i]->id, 4) == 0) {
			dst = &fd->artist;
		} else if (strncmp("TIT2", tag->frames[i]->id, 4) == 0) {
			dst = &fd->title;
#ifdef BUILD_SCROBBLER
		} else if (strncmp("TALB", tag->frames[i]->id, 4) == 0) {
			dst = &fd->album;
#endif /* BUILD_SCROBBLER */
		} else {
			continue;
		}
		if (id3_field_getnstrings(&tag->frames[i]->fields[1]) != 0) {
			str = id3_field_getstrings(&tag->frames[i]->fields[1], 0);
			if (str != NULL)
				*dst = (char*)id3_ucs4_utf8duplicate(str);
		}
	}
	
done:
	id3_file_close(id3f);
bad:
	lseek(tmpfd, orig_off, SEEK_SET);
	close(tmpfd);
}

/*
 * MP3 frame decoding routines
 */

/**
 * @brief Refill the databuffer when it's drained, copying the last
 *        partial frame to the beginning.
 */
static size_t
mp3_fill_buffer(struct audio_file *fd)
{
	struct mp3_drv_data *data = fd->drv_data;
	size_t offset, filledlen, readlen;

	if (data->mstream.next_frame == NULL) {
		/* Place contents at the beginning */
		offset = 0;
	} else {
		/*
		 * We still need to save the fragmented frame. Copy it
		 * to the beginning and load as much as possible.
		 */
		offset = data->mstream.bufend - data->mstream.next_frame;
		memmove(data->buf_input, data->mstream.next_frame, offset);
	}

	readlen = sizeof data->buf_input - offset;

	filledlen = fread(data->buf_input + offset, 1, readlen, fd->fp);

	if (filledlen <= 0)
		return (0);

	if (filledlen < readlen) {
		/*
		 * Place some null bytes at the end
		 * because we may overrun it
		 */
		memset(data->buf_input + offset + filledlen, 0x00000000,
		    MAD_BUFFER_GUARD);
		filledlen += MAD_BUFFER_GUARD;
	}

	/* Return the length of the entire buffer */
	return (offset + filledlen);
}

/**
 * @brief Try to decode a frame, refilling the buffer if needed.
 */
static int
mp3_read_frame(struct audio_file *fd)
{
	struct mp3_drv_data *data = fd->drv_data;
	size_t buflen;

	/*  Get the next frame */
	for (;;) {
		/* We've run out of data. Read from file */
		if ((data->mstream.buffer == NULL) ||
		    (data->mstream.error == MAD_ERROR_BUFLEN)) {
			if ((buflen = mp3_fill_buffer(fd)) == 0)
				/* Read error */
				return (1);

			/* Load it back in */
			mad_stream_buffer(&data->mstream, data->buf_input,
			    buflen);
			data->mstream.error = MAD_ERROR_NONE;
		}

		if (mad_header_decode(&data->mheader, &data->mstream) == 0) {
			/* Update the audio timer */
			mad_timer_add(&data->mtimer,
			    data->mframe.header.duration);
			fd->time_cur = data->mtimer.seconds;
			data->mframe.header = data->mheader;
			return (0);
		}
	
		if (!MAD_RECOVERABLE(data->mstream.error) &&
		    (data->mstream.error != MAD_ERROR_BUFLEN)) {
			/* Unrecoverable error - bail out */
			return (1);
		}
	}

}

/**
 * @brief Convert a fixed point sample to a short.
 */
static inline int16_t
mp3_fixed_to_short(mad_fixed_t fixed)
{
	if (fixed >= MAD_F_ONE)
		return (SHRT_MAX);
	else if (fixed <= -MAD_F_ONE)
		return (-SHRT_MAX);

	return (fixed >> (MAD_F_FRACBITS - 15));
}

/**
 * @brief Rewind the current audio file handle to the beginning.
 */
static void
mp3_rewind(struct audio_file *fd)
{
	struct mp3_drv_data *data = fd->drv_data;

	rewind(fd->fp);

	data->cursample = 0;
	fd->time_cur = 0;

	mad_stream_init(&data->mstream);
	mad_frame_init(&data->mframe);
	mad_header_init(&data->mheader);
	mad_synth_init(&data->msynth);
	mad_timer_reset(&data->mtimer);
}

/**
 * @brief Calculate the length of the current audio file.
 */
static void
mp3_calc_length(struct audio_file *fd)
{
	struct mp3_drv_data *data = fd->drv_data;
	off_t curpos;

	/* Go to the end of the file */
	do {
		if (mp3_read_frame(fd) != 0) {
			/* Short track */
			fd->time_len = data->mtimer.seconds;
			data->flen = ftello(fd->fp);
			goto done;
		}
	} while (data->mtimer.seconds < 100);

	/* Extrapolate the time. Not really accurate, but good enough */
	curpos = ftello(fd->fp);
	fseek(fd->fp, 0, SEEK_END);
	data->flen = ftello(fd->fp);

	fd->time_len = ((double)data->flen / curpos) *
	    data->mtimer.seconds;
done:
	/* Go back to the start */
	mp3_rewind(fd);
}

/*
 * Public API
 */

int
mp3_open(struct audio_file *fd, const char *ext)
{
	struct mp3_drv_data *data;

	/* Don't match other files */
	if (!fd->stream) {
		if (mp3_match(fd->fp, ext) != 0)
			return (-1);
		mp3_readtags(fd);
	}

	data = g_slice_new(struct mp3_drv_data);
	fd->drv_data = (void *)data;

	mp3_rewind(fd);
	if (!fd->stream)
		mp3_calc_length(fd);
	
	return (0);
}

void
mp3_close(struct audio_file *fd)
{
	struct mp3_drv_data *data = fd->drv_data;
	
	mad_frame_finish(&data->mframe);
	mad_stream_finish(&data->mstream);
	mad_synth_finish(&data->msynth);

	g_slice_free(struct mp3_drv_data, data);
}

size_t
mp3_read(struct audio_file *fd, int16_t *buf, size_t len)
{
	struct mp3_drv_data *data = fd->drv_data;
	size_t written = 0;
	int i;

	do {
		/* Get a new frame when we haven't go one */
		if (data->cursample == 0) {
			if (mp3_read_frame(fd) != 0)
				goto done;
			if (mad_frame_decode(&data->mframe, &data->mstream) == -1)
				continue;

			/* We can now set the sample rate */
			fd->srate = data->mframe.header.samplerate;
			fd->channels = MAD_NCHANNELS(&data->mframe.header);

			mad_synth_frame(&data->msynth, &data->mframe);
		}

		while ((data->cursample < data->msynth.pcm.length) &&
		    (written < len)) {
			/* Write out all channels */
			for (i = 0; i < MAD_NCHANNELS(&data->mframe.header); i++) {
				buf[written++] = mp3_fixed_to_short(
				    data->msynth.pcm.samples[i][data->cursample]);
			}

			data->cursample++;
		}

		/* Move on to the next frame */
		if (data->cursample == data->msynth.pcm.length)
			data->cursample = 0;

	} while (written < len);

done:
	return (written);
}

void
mp3_seek(struct audio_file *fd, int len, int rel)
{
	struct mp3_drv_data *data = fd->drv_data;
	off_t newpos;

	if (rel) {
		/* Relative seek */
		len += fd->time_cur;
	}

	/* Calculate the new relative position */
	len = CLAMP(len, 0, (int)fd->time_len);
	newpos = ((double)len / fd->time_len) * data->flen;
	
	/* Seek to the new position */
	mp3_rewind(fd);
	fseek(fd->fp, newpos, SEEK_SET);
	data->mtimer.seconds = len;
	data->mtimer.fraction = 0;
	fd->time_cur = data->mtimer.seconds;
}
