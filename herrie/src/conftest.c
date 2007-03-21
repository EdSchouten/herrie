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

/*
 * Header files which aren't obtained from pkg-config and such. We need
 * to check whether they are available on our system.
 */

#if 0
#include <ao/ao.h>
#include OSS_HEADER
#include CURSES_HEADER
#endif
#ifdef BUILD_MODPLUG
#include <sys/mman.h>
#endif /* BUILD_MODPLUG */
#ifdef BUILD_MP3
#include <id3tag.h>
#include <mad.h>
#endif /* BUILD_MP3 */
#ifdef BUILD_SCROBBLER
#include <openssl/md5.h>
#endif /* BUILD_SCROBBLER */
#ifdef BUILD_SNDFILE
#include <sndfile.h>
#endif /* BUILD_SNDFILE */
#ifdef BUILD_VORBIS
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif /* BUILD_VORBIS */

int
main(int argc, char *argv[])
{
	return (0);
}
