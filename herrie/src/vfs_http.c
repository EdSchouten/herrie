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
 * @file vfs_regular.c
 */

#include <curl/curl.h>
#include <curl/easy.h>

#include "vfs_modules.h"

#define HTTPBUFSIZ	2048

struct httpstream {
	/* Curl connection */
	char		*url;
	CURL		*con;
	CURLM		*conm;

	/* Data buffers */
	char		*bufptr;
	char		*buflen;
	char		buf[CURL_MAX_WRITE_SIZE];
};

/*
 * CURL routines
 */
static size_t
vfs_http_incoming(void *ptr, size_t size, size_t nmemb, void *cookie)
{
	struct httpstream *hs = cookie;
	size_t len;

	len = size * nmemb;
	memcpy(hs->buf, ptr, size * nmemb);
	hs->bufptr = hs->buf;
	hs->buflen = hs->buf + len;
	return (len);
}

/*
 * FILE * wrapper
 */

static int
vfs_http_readfn(void *cookie, char *buf, int len)
{
	struct httpstream *hs = cookie;
	int handles, left = len, copylen, maxfd;
	fd_set rfds, wfds, efds;
	CURLMcode ret = CURLM_CALL_MULTI_PERFORM;

	while (left > 0) {
		if (hs->bufptr == hs->buflen) {
			FD_ZERO(&rfds);
			FD_ZERO(&wfds);
			FD_ZERO(&efds);
			curl_multi_fdset(hs->conm, &rfds, &wfds, &efds, &maxfd);
			if (maxfd != -1)
				select(maxfd + 1, &rfds, &wfds, &efds, NULL);
			ret = curl_multi_perform(hs->conm, &handles);
			if (ret == CURLM_CALL_MULTI_PERFORM)
				continue;
			if (ret != CURLM_OK)
				return (0);
		}
		g_assert(hs->bufptr != hs->buflen);

		copylen = MIN(left, hs->buflen - hs->bufptr);
		memcpy(buf, hs->bufptr, copylen);
		left -= copylen;
		hs->bufptr += copylen;
		buf += copylen;
	}

	return (len);
}

static int
vfs_http_closefn(void *cookie)
{

	return (0);
}

/*
 * Public interface
 */

int
vfs_http_open(struct vfsent *ve, int isdir)
{
	return strncmp(ve->filename, "http://", 7);
}

FILE *
vfs_http_handle(struct vfsent *ve)
{
	struct httpstream *hs;
	//struct curl_slist *slist = NULL;
	FILE *ret;

	/* Allocate the datastructure */
	hs = g_slice_new(struct httpstream);
	hs->bufptr = hs->buflen = NULL;

	/* Curl connection */
	hs->con = curl_easy_init();
	hs->url = g_strdup(ve->filename);
	curl_easy_setopt(hs->con, CURLOPT_URL, hs->url);

	curl_easy_setopt(hs->con, CURLOPT_WRITEFUNCTION, vfs_http_incoming);
	curl_easy_setopt(hs->con, CURLOPT_WRITEDATA, hs);

	hs->conm = curl_multi_init();
	curl_multi_add_handle(hs->conm, hs->con);

	ret = funopen(hs, vfs_http_readfn, NULL, NULL, vfs_http_closefn);

	return (ret);
}
