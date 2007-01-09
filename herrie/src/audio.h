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
 * @file audio.h
 */

#ifndef _AUDIO_H_
#define _AUDIO_H_

struct audio_format;

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
	long srate;

	/**
	 * @brief The number of audio channels.
	 */
	long channels;

	/**
	 * @brief The file's length in seconds.
	 */
	int time_len;
	/**
	 * @brief Position we are at in the file in seconds.
	 */
	int time_cur;

	/**
	 * @brief Information about the song acquired from the file.
	 */
	struct {
		/**
		 * @brief Name of the artist, stored in UTF-8.
		 */
		char *artist;
		/**
		 * @brief Name of the song, stored in UTF-8.
		 */
		char *title;
		/**
		 * @brief Name of the album, stored in UTF-8.
		 */
		char *album;
	} tag;

#ifdef BUILD_SCROBBLER
	/**
	 * @brief Indicator whether the scrobbler code is done with the
	 *        song.
	 */
	char _scrobbler_done;
#endif /* BUILD_SCROBBLER */
};

/**
 * @brief Fill an audio_file struct with the appropiate information
 * 	and function calls, and open the file handle.
 */
struct audio_file *audio_open(const char *filename);
/**
 * @brief Clean up the given audio_file struct and close the file handle.
 */
void audio_close(struct audio_file *fd);

/**
 * @brief Call the read function in the audio_file struct.
 */
size_t audio_read(struct audio_file *fd, void *buf);

/**
 * @brief Call the seek function in the audio_file struct if available.
 */
void audio_seek(struct audio_file *fd, int len);

#endif /* !_AUDIO_H_ */
