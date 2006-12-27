/*
 * Copyright (c) 2006 Ed Schouten <ed@fxq.nl>
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
 * @file scrobbler_hash.c
 */

#include <sys/types.h>
#include <openssl/md5.h>

#include "scrobbler_internal.h"

/**
 * @brief Convert a numerical value to a hexadecimal character.
 */
static char
tohexadecimal(char val)
{
	if (val < 10)
		return (val + '0');
	else
		return (val - 10 + 'a');
}

void
scrobbler_hash(struct scrobbler_condata *scd)
{
	int i;
	unsigned char bin_res[16];
	MD5_CTX ctx;

	/*
	 * Generate the new MD5 value
	 */
	MD5_Init(&ctx);
	MD5_Update(&ctx, scd->password, 32);
	MD5_Update(&ctx, scd->challenge, 32);
	MD5_Final(bin_res, &ctx);

	/*
	 * Convert the result back to hexadecimal string
	 */
	for (i = 0; i < sizeof bin_res; i++) {
		scd->response[i * 2] = tohexadecimal(bin_res[i] >> 4);
		scd->response[i * 2 + 1] = tohexadecimal(bin_res[i] & 0x0f);
	}
}
