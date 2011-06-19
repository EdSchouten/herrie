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
 * @file md5.h
 * @brief MD5 hashing.
 */

/**
 * @brief Internal state of the MD5 hash calculation.
 */
struct md5_context {
	/**
	 * @brief Buffer to store partial 64 byte frames.
	 */
	uint32_t buf[16];
	/**
	 * @brief MD5 hash of the partially hashed stream.
	 */
	uint32_t state[4];
	/**
	 * @brief Amount of bytes that have been hashed.
	 */
	uint64_t count;
};

/**
 * @brief Contents of an initialized MD5 context.
 */
#define MD5CONTEXT_INITIALIZER \
	{ { 0 }, { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 }, 0 }
/**
 * @brief Update the hash value by appending a buffer.
 */
void md5_update(struct md5_context *m, const void *buf, size_t len);
/**
 * @brief Finalize the hash value and return its hash value.
 */
void md5_final(struct md5_context *m, unsigned char out[16]);
