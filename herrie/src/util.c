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
 * @file util.c
 * @brief General utility functions
 */

#include "stdinc.h"

#include "util.h"

#ifdef BUILD_SCROBBLER
/**
 * @brief Convert a numerical value to a hexadecimal character.
 */
static inline char
toxdigit(char val)
{
	if (val < 10)
		return (val + '0');
	else
		return (val - 10 + 'a');
}

void
hex_encode(unsigned char *bin, char *hex, size_t len)
{
	while (len-- > 0) {
		*hex++ = toxdigit(*bin >> 4);
		*hex++ = toxdigit(*bin++ & 0x0f);
	}
}
#endif /* BUILD_SCROBBLER */

char *
http_escape(const char *str, const char *prepend)
{
	const char *c;
	const char allowed[] = "-_.!~*'()/";
	GString *ret;

	if (prepend == NULL)
		prepend = "";

	/* Argument may be empty */
	if (str == NULL)
		return g_strdup(prepend);

	ret = g_string_new(prepend);

	for (c = str; *c != '\0'; c++) {
		if (*c == ' ')
			g_string_append_c(ret, '+');
		else if (g_ascii_isalnum(*c) || strchr(allowed, *c) != NULL)
			/* Character is allowed */
			g_string_append_c(ret, *c);
		else
			/* Reserved or unwise character */
			g_string_append_printf(ret, "%%%02hhx",
			    (const unsigned char)*c);
	}

	return g_string_free(ret, FALSE);
}

#ifdef BUILD_XSPF
/**
 * @brief Unescape a string according to HTTP/1.1. The conversion will
 *        be performed in place. The arguments point to the read and
 *        write offsets. In almost all cases, they must point to the
 *        same string.
 */
static void
http_unescape(char *r, char *w)
{
	for (; *r != '\0'; r++, w++) {
		if (r[0] == '%' &&
		    g_ascii_isxdigit(r[1]) && g_ascii_isxdigit(r[2])) {
			/* Character needs to be unescaped */
			*w = g_ascii_xdigit_value(r[1]) << 4;
			*w |= g_ascii_xdigit_value(r[2]);
			/* Move two bytes more */
			r += 2;
		} else if (*r == '+') {
			/* Convert + back to space */
			*w = ' ';
		} else {
			/* Ordinary character */
			*w = *r;
		}
	}

	/* Null terminate the output */
	*w = '\0';
}

char *
url_escape(const char *str)
{
	if (strstr(str, "://") == NULL) {
		return http_escape(str, "file://");
	} else {
		return g_strdup(str);
	}
}

char *
url_unescape(char *str)
{
	if (strncmp(str, "file://", 7) == 0) {
		http_unescape(str + 7, str);
		/* XXX: slash conversion */
	}

	return (str);
}
#endif /* BUILD_XSPF */
