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
 * @file scrobbler_send.c
 */

#include <glib.h>

#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include "config.h"
#include "scrobbler_internal.h"

/**
 *@ brief The URL format used for the initial handshake.
 */
#define SCROBBLER_URL "http://%s/?hs=true&p=1.1&c=her&v=0.1&u=%s"

/**
 * @brief Parse a handshake response and update the AudioScrobbler
 *        thread storage.
 */
static size_t
handshake_write(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct scrobbler_condata *scd = stream;

	size_t len = size * nmemb;
	char *cur = ptr, *next;

	/* First step: null terminate the buffer */
	cur[len - 1] = '\0';

	do {
		/* Terminate the line */
		next = strchr(cur, '\n');
		if (next != NULL)
			*next = '\0';

		if (strncmp("INTERVAL ", cur, 9) == 0) {
			/* Interval line */
			scd->interval = atoi(cur + 9);
		} else if (strncmp("http:// ", cur, 7) == 0) {
			/* URL */
			g_free(scd->url);
			scd->url = g_strdup(cur);
		} else if (strlen(cur) == 32) {
			/* Challenge */
			memcpy(scd->challenge, cur, 32);
		}

		/* Move on to the next line */
		cur = next + 1;
	} while (next != NULL);

	return (len);
}

int
scrobbler_send_handshake(struct scrobbler_condata *scd)
{
	int ret = -1;
	char *url;
	const char *host;
	CURL *con;

	con = curl_easy_init();
	if (con == NULL)
		return (-1);
	
	/* Set the connection URL */
	host = config_getopt("scrobbler.hostname");
	url = g_strdup_printf(SCROBBLER_URL, host, scd->username);
	curl_easy_setopt(con, CURLOPT_URL, url);

	/* Callback function */
	curl_easy_setopt(con, CURLOPT_WRITEFUNCTION, handshake_write);
	curl_easy_setopt(con, CURLOPT_WRITEDATA, scd);

	/* Free resources that will be acquired during fetch */
	scd->challenge[0] = '\0';
	g_free(scd->url); scd->url = NULL;

	if (curl_easy_perform(con) != 0)
		goto done;
	
	/* After running this function, we *MUST* have a challenge */
	if (scd->challenge[0] == '\0' || scd->url == NULL)
		goto done;
	
	/* Everything went well */
	ret = 0;
done:	curl_easy_cleanup(con);
	g_free(url);
	return (ret);
}

/**
 * @brief Parse a submission response and update the AudioScrobbler
 *        thread storage.
 */
static size_t
tracks_write(void *ptr, size_t size, size_t nmemb, void *stream)
{
	struct scrobbler_condata *scd = stream;

	size_t len = size * nmemb;
	char *cur = ptr, *next;

	/* First step: null terminate the buffer */
	cur[len - 1] = '\0';

	do {
		/* Terminate the line */
		next = strchr(cur, '\n');
		if (next != NULL)
			*next = '\0';

		if (strncmp("INTERVAL ", cur, 9) == 0) {
			/* Interval line */
			scd->interval = atoi(cur + 9);
		} else if (strcmp("OK", cur) == 0) {
			/* 'OK' string */
			scd->ok = 1;
		} else if (strcmp("BADAUTH", cur) == 0) {
			/* 'BADAUTH' string */
			scd->authorized = 0;
		}

		/* Move on to the next line */
		cur = next + 1;
	} while (next != NULL);

	return (len);
}

int
scrobbler_send_tracks(struct scrobbler_condata *scd, char *poststr)
{
	CURL *con;

	con = curl_easy_init();
	if (con == NULL)
		return (-1);
	
	scd->ok = 0;

	curl_easy_setopt(con, CURLOPT_URL, scd->url);
	curl_easy_setopt(con, CURLOPT_POSTFIELDS, poststr);

	curl_easy_setopt(con, CURLOPT_WRITEFUNCTION, tracks_write);
	curl_easy_setopt(con, CURLOPT_WRITEDATA, scd);

	curl_easy_perform(con);
	curl_easy_cleanup(con);

	return (scd->ok != 1);
}
