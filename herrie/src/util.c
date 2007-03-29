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
 * @file util.c
 * @brief General utility functions
 */

#include "util.h"

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
hex_encode(char *bin, char *hex, size_t len)
{
	while (len-- > 0) {
		*hex++ = toxdigit((unsigned char)*bin >> 4);
		*hex++ = toxdigit((unsigned char)*bin++ & 0x0f);
	}
}

inline void
hex_decode(char *hex, char *bin, size_t len)
{
	while (len-- > 0) {
		*bin = g_ascii_xdigit_value(*hex++) << 4;
		*bin++ |= g_ascii_xdigit_value(*hex++);
	}
}

char *
http_escape(const char *str)
{
	const char *c;
	const char allowed[] = "-_.!~*'()";
	GString *ret;

	/* Argument may be empty */
	if (str == NULL)
		return g_strdup("");

	ret = g_string_sized_new(32);

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

void
http_unescape(char *str)
{
	char *r, *w; /* Read and write offsets */

	for (r = w = str; *r != '\0'; r++, w++) {
		if (r[0] == '%' &&
		    g_ascii_isxdigit(r[1]) && g_ascii_isxdigit(r[2])) {
			/* Character needs to be unescaped */
			hex_decode(&r[1], w, 1);
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
