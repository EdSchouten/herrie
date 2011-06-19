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
 * @file audio_file.h
 * @brief Generic access and decoding of audio file formats.
 */

struct audio_format;
struct vfsref;

/**
 * @brief The data needed about an audio file.
 */
struct audio_file {
	/**
	 * @brief The FILE pointer for the file.
	 */
	FILE *fp;

	/**
	 * @brief An audio format for opening, closing and reading
	 * 	the audio file.
	 */
	struct audio_format *drv;
	/**
	 * @brief Extra information that might be needed.
	 */
	void *drv_data;

	/**
	 * @brief The file's sample rate.
	 */
	unsigned int srate;

	/**
	 * @brief The number of audio channels.
	 */
	unsigned int channels;

	/**
	 * @brief The file's length in seconds.
	 */
	unsigned int time_len;
	/**
	 * @brief Position we are at in the file in seconds.
	 */
	unsigned int time_cur;
	/**
	 * @brief File is a stream (no seeking).
	 */
	int stream;

	/**
	 * @brief Name of the artist, stored in UTF-8.
	 */
	char *artist;
	/**
	 * @brief Name of the song, stored in UTF-8.
	 */
	char *title;

#ifdef BUILD_SCROBBLER
	/**
	 * @brief Name of the album, stored in UTF-8.
	 */
	char *album;
	/**
	 * @brief Indicator whether the scrobbler code is done with the
	 *        song.
	 */
	char _scrobbler_done;
#endif /* BUILD_SCROBBLER */
};

/**
 * @brief Fill an audio_file struct with the appropiate information
 *        and function calls, and open the file handle.
 */
struct audio_file *audio_file_open(const struct vfsref *vr);
/**
 * @brief Clean up the given audio_file struct and close the file handle.
 */
void audio_file_close(struct audio_file *fd);

/**
 * @brief Call the read function in the audio_file struct.
 */
size_t audio_file_read(struct audio_file *fd, int16_t *buf, size_t len);

/**
 * @brief Call the seek function in the audio_file struct.
 */
void audio_file_seek(struct audio_file *fd, int len, int rel);
