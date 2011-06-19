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
 * @file util.h
 * @brief General utility functions
 */

#ifdef BUILD_SCROBBLER
/**
 * @brief Convert a binary buffer to a hexadecimal string. The len
 *        parameter is the length of the binary buffer. The string will
 *        not be null terminated.
 */
void hex_encode(unsigned char *bin, char *hex, size_t len);
#endif /* BUILD_SCROBBLER */

/**
 * @brief Escape a string according to HTTP/1.1. A string can be
 *        prepended as well, which won't be escaped.
 */
char *http_escape(const char *str, const char *prepend);

#ifdef BUILD_XSPF
/**
 * @brief Escape an URL when needed. If it's a local filename, file://
 *        will be prepended.
 */
char *url_escape(const char *str);
/**
 * @brief Unescape an URL to a local filename where possible
 *        (file://foo -> foo).
 */
char *url_unescape(char *str);
#endif /* BUILD_XSPF */
