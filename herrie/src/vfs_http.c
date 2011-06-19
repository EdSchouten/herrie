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
 * @file vfs_http.c
 * @brief HTTP file access.
 */

#include "stdinc.h"

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/multi.h>

#include "gui.h"
#include "vfs.h"
#include "vfs_modules.h"

/**
 * @brief Datastructure with HTTP stream buffer.
 */
struct httpstream {
	/**
	 * @brief URL of current connection.
	 */
	char	*url;
	/**
	 * @brief CURL 'easy' connection.
	 */
	CURL	*con;
	/**
	 * @brief CURL 'multi' connection.
	 */
	CURLM	*conm;

	/**
	 * @brief Pointer to current read position in the buffer.
	 */
	char	*bufptr;
	/**
	 * @brief Pointer to where the buffer's contents end.
	 */
	char	*buflen;
	/**
	 * @brief Temporary buffer to store fetched data.
	 */
	char	buf[CURL_MAX_WRITE_SIZE];
};

/**
 * @brief Callback function that copies packet contents to httpstream
 *        structure.
 */
static size_t
vfs_http_incoming(void *ptr, size_t size, size_t nmemb, void *cookie)
{
	struct httpstream *hs = cookie;
	size_t len;

	/* We just assume the buffer is completely drained */
	len = size * nmemb;
	memcpy(hs->buf, ptr, size * nmemb);
	hs->bufptr = hs->buf;
	hs->buflen = hs->buf + len;

	return (len);
}

/**
 * @brief fread() routine for HTTP streams.
 */
#ifdef __GLIBC__
static ssize_t
vfs_http_readfn(void *cookie, char *buf, size_t len)
#else /* !__GLIBC__ */
static int
vfs_http_readfn(void *cookie, char *buf, int len)
#endif /* __GLIBC__ */
{
	struct httpstream *hs = cookie;
	struct timeval timeout = { 5, 0 };
	char *errmsg;
	int handles, left = len, copylen, maxfd, sret;
	fd_set rfds, wfds, efds;
	CURLMcode cret;

	while (left > 0) {
		if (hs->bufptr == hs->buflen) {
			FD_ZERO(&rfds);
			FD_ZERO(&wfds);
			FD_ZERO(&efds);
			curl_multi_fdset(hs->conm, &rfds, &wfds, &efds,
			    &maxfd);
			if (maxfd != -1) {
				/* Wait for data to arrive */
				sret = select(maxfd + 1, &rfds, &wfds,
				    &efds, &timeout);
				if (sret <= 0)
					goto bad;
			}
			/* Fetch data from socket */
			cret = curl_multi_perform(hs->conm, &handles);
			if (handles == 0)
				goto bad;
			if (cret == CURLM_CALL_MULTI_PERFORM ||
			    hs->bufptr == hs->buflen)
				continue;
			if (cret != CURLM_OK)
				goto bad;
		}

		/* Perform a copyout of the buffer */
		copylen = MIN(left, hs->buflen - hs->bufptr);
		memcpy(buf, hs->bufptr, copylen);
		left -= copylen;
		hs->bufptr += copylen;
		buf += copylen;
	}

	return (len);

bad:	/* Bail out with an error message */
	errmsg = g_strdup_printf(
	    _("Connection with \"%s\" lost."), hs->url);
	gui_msgbar_warn(errmsg);
	g_free(errmsg);
	return (0);
}

/**
 * @brief fclose() routine for HTTP streams.
 */
static int
vfs_http_closefn(void *cookie)
{
	struct httpstream *hs = cookie;

	curl_multi_cleanup(hs->conm);
	curl_easy_cleanup(hs->con);
	g_free(hs->url);
	g_slice_free(struct httpstream, hs);

	return (0);
}

/*
 * Public interface
 */

int
vfs_http_match(struct vfsent *ve, int isdir)
{
	return strncmp(ve->filename, "http://", 7);
}

FILE *
vfs_http_open(struct vfsent *ve)
{
	struct httpstream *hs;
	FILE *ret;
#ifdef __GLIBC__
	cookie_io_functions_t iofn = {
	    vfs_http_readfn, NULL, NULL, vfs_http_closefn };
#endif /* __GLIBC__ */

	/* Allocate the datastructure */
	hs = g_slice_new(struct httpstream);
	hs->bufptr = hs->buflen = NULL;

	/* Curl connection */
	hs->con = curl_easy_init();
	hs->url = g_strdup(ve->filename);
	curl_easy_setopt(hs->con, CURLOPT_URL, hs->url);
	curl_easy_setopt(hs->con, CURLOPT_CONNECTTIMEOUT, 5);
	curl_easy_setopt(hs->con, CURLOPT_USERAGENT, APP_NAME "/" APP_VERSION);

	curl_easy_setopt(hs->con, CURLOPT_WRITEFUNCTION, vfs_http_incoming);
	curl_easy_setopt(hs->con, CURLOPT_WRITEDATA, hs);

	hs->conm = curl_multi_init();
	curl_multi_add_handle(hs->conm, hs->con);

#ifdef __GLIBC__
	/* glibc systems should have fopencookie() */
	ret = fopencookie(hs, "rb", iofn);
#else /* !__GLIBC__ */
	/* BSD's */
	ret = funopen(hs, vfs_http_readfn, NULL, NULL, vfs_http_closefn);
#endif /* __GLIBC__ */
	if (ret == NULL)
		vfs_http_closefn(hs);

	return (ret);
}
