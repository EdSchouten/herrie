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
 * @file md5.c
 * @brief MD5 hashing.
 */

#include "stdinc.h"

#include "md5.h"

/**
 * @brief Encode data from native byte ordering.
 */
static inline void
md5_encode(uint32_t dst[4], const uint32_t src[4])
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	memcpy(dst, src, 16);
#else /* G_BYTE_ORDER != G_LITTLE_ENDIAN */
	size_t i;

	/* Convert data to little endian */
	for (i = 0; i < 4; i++)
		dst[i] = GUINT32_TO_LE(src[i]);
#endif /* G_BYTE_ORDER == G_LITTLE_ENDIAN */
}

/**
 * @brief Decode data to native byte ordering.
 */
static inline void
md5_decode(uint32_t buf[16])
{
#if G_BYTE_ORDER != G_LITTLE_ENDIAN
	size_t i;

	/* Convert data from little endian */
	for (i = 0; i < 16; i++)
		buf[i] = GUINT32_FROM_LE(buf[i]);
#endif /* G_BYTE_ORDER != G_LITTLE_ENDIAN */
}

/**
 * @brief First of the four MD5 calculation steps.
 */
#define md5_f(x, y, z)	(z ^ (x & (y ^ z)))
/**
 * @brief Second of the four MD5 calculation steps.
 */
#define md5_g(x, y, z)	(y ^ (z & (x ^ y)))
/**
 * @brief Third of the four MD5 calculation steps.
 */
#define md5_h(x, y, z)	(x ^ y ^ z)
/**
 * @brief Fourth of the four MD5 calculation steps.
 */
#define md5_i(x, y, z)	(y ^ (x | ~z))
/**
 * @brief Perform a single MD5 calculation step, including the rotation
 *        and addition steps.
 */
#define md5_step(f, w, x, y, z, d, s) do {	\
	w += f(x, y, z) + d;			\
	w = (w << s) | (w >> (32 - s));		\
	w += x;					\
} while(0)

/**
 * @brief Hash a 64 byte chunk and add it to the old hash value.
 */
static void
md5_transform(uint32_t state[4], const uint32_t buf[16])
{
	uint32_t a, b, c, d;

	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];

	md5_step(md5_f, a, b, c, d, buf[0]  + 0xd76aa478, 7);
	md5_step(md5_f, d, a, b, c, buf[1]  + 0xe8c7b756, 12);
	md5_step(md5_f, c, d, a, b, buf[2]  + 0x242070db, 17);
	md5_step(md5_f, b, c, d, a, buf[3]  + 0xc1bdceee, 22);
	md5_step(md5_f, a, b, c, d, buf[4]  + 0xf57c0faf, 7);
	md5_step(md5_f, d, a, b, c, buf[5]  + 0x4787c62a, 12);
	md5_step(md5_f, c, d, a, b, buf[6]  + 0xa8304613, 17);
	md5_step(md5_f, b, c, d, a, buf[7]  + 0xfd469501, 22);
	md5_step(md5_f, a, b, c, d, buf[8]  + 0x698098d8, 7);
	md5_step(md5_f, d, a, b, c, buf[9]  + 0x8b44f7af, 12);
	md5_step(md5_f, c, d, a, b, buf[10] + 0xffff5bb1, 17);
	md5_step(md5_f, b, c, d, a, buf[11] + 0x895cd7be, 22);
	md5_step(md5_f, a, b, c, d, buf[12] + 0x6b901122, 7);
	md5_step(md5_f, d, a, b, c, buf[13] + 0xfd987193, 12);
	md5_step(md5_f, c, d, a, b, buf[14] + 0xa679438e, 17);
	md5_step(md5_f, b, c, d, a, buf[15] + 0x49b40821, 22);

	md5_step(md5_g, a, b, c, d, buf[1]  + 0xf61e2562, 5);
	md5_step(md5_g, d, a, b, c, buf[6]  + 0xc040b340, 9);
	md5_step(md5_g, c, d, a, b, buf[11] + 0x265e5a51, 14);
	md5_step(md5_g, b, c, d, a, buf[0]  + 0xe9b6c7aa, 20);
	md5_step(md5_g, a, b, c, d, buf[5]  + 0xd62f105d, 5);
	md5_step(md5_g, d, a, b, c, buf[10] + 0x02441453, 9);
	md5_step(md5_g, c, d, a, b, buf[15] + 0xd8a1e681, 14);
	md5_step(md5_g, b, c, d, a, buf[4]  + 0xe7d3fbc8, 20);
	md5_step(md5_g, a, b, c, d, buf[9]  + 0x21e1cde6, 5);
	md5_step(md5_g, d, a, b, c, buf[14] + 0xc33707d6, 9);
	md5_step(md5_g, c, d, a, b, buf[3]  + 0xf4d50d87, 14);
	md5_step(md5_g, b, c, d, a, buf[8]  + 0x455a14ed, 20);
	md5_step(md5_g, a, b, c, d, buf[13] + 0xa9e3e905, 5);
	md5_step(md5_g, d, a, b, c, buf[2]  + 0xfcefa3f8, 9);
	md5_step(md5_g, c, d, a, b, buf[7]  + 0x676f02d9, 14);
	md5_step(md5_g, b, c, d, a, buf[12] + 0x8d2a4c8a, 20);

	md5_step(md5_h, a, b, c, d, buf[5]  + 0xfffa3942, 4);
	md5_step(md5_h, d, a, b, c, buf[8]  + 0x8771f681, 11);
	md5_step(md5_h, c, d, a, b, buf[11] + 0x6d9d6122, 16);
	md5_step(md5_h, b, c, d, a, buf[14] + 0xfde5380c, 23);
	md5_step(md5_h, a, b, c, d, buf[1]  + 0xa4beea44, 4);
	md5_step(md5_h, d, a, b, c, buf[4]  + 0x4bdecfa9, 11);
	md5_step(md5_h, c, d, a, b, buf[7]  + 0xf6bb4b60, 16);
	md5_step(md5_h, b, c, d, a, buf[10] + 0xbebfbc70, 23);
	md5_step(md5_h, a, b, c, d, buf[13] + 0x289b7ec6, 4);
	md5_step(md5_h, d, a, b, c, buf[0]  + 0xeaa127fa, 11);
	md5_step(md5_h, c, d, a, b, buf[3]  + 0xd4ef3085, 16);
	md5_step(md5_h, b, c, d, a, buf[6]  + 0x04881d05, 23);
	md5_step(md5_h, a, b, c, d, buf[9]  + 0xd9d4d039, 4);
	md5_step(md5_h, d, a, b, c, buf[12] + 0xe6db99e5, 11);
	md5_step(md5_h, c, d, a, b, buf[15] + 0x1fa27cf8, 16);
	md5_step(md5_h, b, c, d, a, buf[2]  + 0xc4ac5665, 23);

	md5_step(md5_i, a, b, c, d, buf[0]  + 0xf4292244, 6);
	md5_step(md5_i, d, a, b, c, buf[7]  + 0x432aff97, 10);
	md5_step(md5_i, c, d, a, b, buf[14] + 0xab9423a7, 15);
	md5_step(md5_i, b, c, d, a, buf[5]  + 0xfc93a039, 21);
	md5_step(md5_i, a, b, c, d, buf[12] + 0x655b59c3, 6);
	md5_step(md5_i, d, a, b, c, buf[3]  + 0x8f0ccc92, 10);
	md5_step(md5_i, c, d, a, b, buf[10] + 0xffeff47d, 15);
	md5_step(md5_i, b, c, d, a, buf[1]  + 0x85845dd1, 21);
	md5_step(md5_i, a, b, c, d, buf[8]  + 0x6fa87e4f, 6);
	md5_step(md5_i, d, a, b, c, buf[15] + 0xfe2ce6e0, 10);
	md5_step(md5_i, c, d, a, b, buf[6]  + 0xa3014314, 15);
	md5_step(md5_i, b, c, d, a, buf[13] + 0x4e0811a1, 21);
	md5_step(md5_i, a, b, c, d, buf[4]  + 0xf7537e82, 6);
	md5_step(md5_i, d, a, b, c, buf[11] + 0xbd3af235, 10);
	md5_step(md5_i, c, d, a, b, buf[2]  + 0x2ad7d2bb, 15);
	md5_step(md5_i, b, c, d, a, buf[9]  + 0xeb86d391, 21);

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
}

void
md5_update(struct md5_context *m, const void *buf, size_t len)
{
	unsigned char *b, *ib;
	size_t blen, left, clen;

	/* Store how many bytes we process and obtain the buffer length */
	b = (unsigned char *)m->buf;
	blen = m->count & 0x3f;
	m->count += len;
	ib = (unsigned char *)buf;

	/* Finish off an incomplete chunk first if we must */
	if (blen != 0) {
		/* How many bytes we still need */
		left = 64 - blen;
		/* How many bytes we're going to copy */
		clen = MIN(left, len);

		memcpy(b + blen, ib, clen);
		/* This still doesn't make up for a full block */
		if (len < left)
			return;

		md5_decode(m->buf);
		md5_transform(m->state, m->buf);
		ib += clen;
		len -= clen;
	}

	/* Handle the data in 64 byte chunks */
	while (len >= 64) {
		memcpy(m->buf, ib, 64);
		md5_decode(m->buf);
		md5_transform(m->state, m->buf);
		ib += 64;
		len -= 64;
	}

	/* Copy in the last partial frame */
	memcpy(m->buf, ib, len);
}

void
md5_final(struct md5_context *m, unsigned char out[16])
{
	unsigned char *b;
	size_t blen;

	/* Terminate the data with 0x80 */
	b = (unsigned char *)m->buf;
	blen = m->count & 0x3f;
	b[blen++] = 0x80;

	if (blen > 56) {
		/* We can't fit the length. Allocate an extra block. */
		memset(b + blen, 0, 64 - blen);
		md5_decode(m->buf);
		md5_transform(m->state, m->buf);

		/* Add a new zero-block */
		memset(b, 0, 56);
	} else {
		/* The length will fit */
		memset(b + blen, 0, 56 - blen);
		md5_decode(m->buf);
	}

	/* Store the length in bits in the hash as well */
	m->buf[14] = m->count << 3;
	m->buf[15] = m->count >> 29;
	md5_transform(m->state, m->buf);

	/* Copy out the hash and zero our internal storage */
	md5_encode((uint32_t *)out, m->state);
	memset(m, 0, sizeof(struct md5_context));
}
