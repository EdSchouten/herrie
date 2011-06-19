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
 * @file audio_format.h
 * @brief Audio file format specific decompression routines.
 */

struct audio_file;

#ifdef BUILD_MODPLUG
/**
 * @brief Open a modplug file.
 */
int modplug_open(struct audio_file *fd, const char *ext);
/**
 * @brief Close and clean up the modplug file.
 */
void modplug_close(struct audio_file *fd);
/**
 * @brief Read data from the modplug file and place it in buf.
 */
size_t modplug_read(struct audio_file *fd, int16_t *buf, size_t len);
/**
 * @brief Seek the modplug file a relatime amount of seconds.
 */
void modplug_seek(struct audio_file *fd, int len, int rel);
#endif /* BUILD_MODPLUG */

#ifdef BUILD_MP3
/**
 * @brief Open an mp3 file.
 */
int mp3_open(struct audio_file *fd, const char *ext);
/**
 * @brief Close and clean up the mp3 file.
 */
void mp3_close(struct audio_file *fd);
/**
 * @brief Read data from the mp3 file and place it in buf.
 */
size_t mp3_read(struct audio_file *fd, int16_t *buf, size_t len);
/**
 * @brief Seek the mp3 file a relatime amount of seconds.
 */
void mp3_seek(struct audio_file *fd, int len, int rel);
#endif /* BUILD_MP3 */

#ifdef BUILD_GST
/**
 * @brief Open an GST file.
 */
int gst_open(struct audio_file *fd, const char *ext);
/**
 * @brief Close and clean up the GST file.
 */
void gst_close(struct audio_file *fd);
/**
 * @brief Read data from the GST file and place it in buf.
 */
size_t gst_read(struct audio_file *fd, int16_t *buf, size_t len);
/**
 * @brief Seek the GST file a relatime amount of seconds.
 */
void gst_seek(struct audio_file *fd, int len, int rel);
#endif /* BUILD_GST */

#ifdef BUILD_SNDFILE
/**
 * @brief Open a soundfile.
 */
int sndfile_open(struct audio_file *fd, const char *ext);
/**
 * @brief Close the soundfile.
 */
void sndfile_close(struct audio_file *fd);
/**
 * @brief Read data from the soundfile and place it in buf.
 */
size_t sndfile_read(struct audio_file *fd, int16_t *buf, size_t len);
/**
 * @brief Seek a relative amount in seconds in the current file handle.
 */
void sndfile_seek(struct audio_file *fd, int len, int rel);
#endif /* BUILD_SNDFILE */

#ifdef BUILD_VORBIS
/**
 * @brief Open an Ogg Vorbis file.
 */
int vorbis_open(struct audio_file *fd, const char *ext);
/**
 * @brief Close and clean up the Ogg Vorbis file.
 */
void vorbis_close(struct audio_file *fd);
/**
 * @brief Read data from the Ogg Vorbis file and place it in buf.
 */
size_t vorbis_read(struct audio_file *fd, int16_t *buf, size_t len);
/**
 * @brief Seek a relative amount in seconds in the current file handle.
 */
void vorbis_seek(struct audio_file *fd, int len, int rel);
#endif /* BUILD_VORBIS */
