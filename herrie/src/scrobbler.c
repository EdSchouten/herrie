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
 * @file scrobbler.c
 * @brief AudioScrobbler submission.
 */

#include "stdinc.h"

#include <curl/curl.h>
#include <curl/easy.h>

#include "audio_file.h"
#include "config.h"
#include "gui.h"
#include "md5.h"
#include "scrobbler.h"
#include "util.h"
#include "vfs.h"

/**
 *@ brief The URL format used for the initial handshake.
 */
#define SCROBBLER_URL "http://%s/?hs=true&p=1.2&c=her&v=0.1&u=%s&t=%u&a=%s"

/**
 * @brief Flag indicating if the AudioScrobbler thread has already been
 *        launched, causing the submission queue to be filled.
 */
static char	scrobbler_enabled = 0;

/**
 * @brief Lock used to provide safe access to the AudioScrobbler queue.
 */
static GMutex 	*scrobbler_lock;
/**
 * @brief Conditional variable used to notify the avaiability of new
 *        tracks ready for submission to AudioScrobbler.
 */
static GCond	*scrobbler_avail;
/**
 * @brief Reference to AudioScrobbler submission thread.
 */
static GThread	*scrobbler_runner;

/**
 * @brief An entry in the AudioScrobbler queue, ready to be submitted to
 *        AudioScrobbler.
 */
struct scrobbler_entry {
	/**
	 * @brief The artist of the song, escaped to conform with
	 *        HTTP/1.1.
	 */
	char	*artist;
	/**
	 * @brief The title of the song, escaped to conform with
	 *        HTTP/1.1.
	 */
	char	*title;
	/**
	 * @brief The album of the song, escaped to conform with
	 *        HTTP/1.1.
	 */
	char	*album;
	/**
	 * @brief The length in seconds of the song.
	 */
	unsigned int length;
	/**
	 * @brief The time the song was added to the queue.
	 */
	time_t	time;

	/**
	 * @brief A reference to the next entry in the queue.
	 */
	struct scrobbler_entry	*next;
};
/**
 * @brief First item in the Scrobbler queue.
 */
static struct scrobbler_entry *scrobbler_queue_first = NULL;
/**
 * @brief Last item in the Scrobbler queue.
 */
static struct scrobbler_entry *scrobbler_queue_last = NULL;

/**
 * @brief Next item in the Scrobbler queue.
 */
static inline struct scrobbler_entry *
scrobbler_queue_next(struct scrobbler_entry *se)
{
	return (se->next);
}

/**
 * @brief Place a new item in our Scrobbler queue.
 */
static inline void
scrobbler_queue_insert_tail(struct scrobbler_entry *se)
{
	se->next = NULL;
	if (scrobbler_queue_last != NULL)
		scrobbler_queue_last->next = se;
	else
		scrobbler_queue_first = se;
	scrobbler_queue_last = se;
}

/**
 * @brief Iterate through all entries in the Scrobbler queue.
 */
#define SCROBBLER_QUEUE_FOREACH(se) \
	for (se = scrobbler_queue_first; se != NULL; se = se->next)

/**
 * @brief Remove the first item from our Scrobbler queue.
 */
static inline void
scrobbler_queue_remove_head(void)
{
	g_assert(scrobbler_queue_first != NULL);

	if (scrobbler_queue_first == scrobbler_queue_last)
		scrobbler_queue_last = NULL;
	scrobbler_queue_first = scrobbler_queue_first->next;
}

void
scrobbler_notify_read(struct audio_file *fd, int eof)
{
	struct scrobbler_entry *nse;
	unsigned int len;

	/* Don't accept streams or submit songs twice */
	if (!scrobbler_enabled || fd->stream || fd->_scrobbler_done)
		return;
	
	if (eof) {
		/* Just take the position - catches formats without seeking */
		len = fd->time_cur;
	} else {
		/* We may only submit if we're past four minutes or past 50% */
		if ((fd->time_cur < 240) &&
		    (fd->time_cur < (fd->time_len / 2)))
			return;

		len = fd->time_len;
	}

	/* Track was too short */
	if (len < 30)
		return;
	
	/* Mark it as processed */
	fd->_scrobbler_done = 1;

	/* We must have a title and an artist or an album */
	if (fd->title == NULL || (fd->artist == NULL && fd->album == NULL))
		return;

	/* Place the track in our queue */
	nse = g_slice_new(struct scrobbler_entry);
	nse->artist = http_escape(fd->artist, NULL);
	nse->title = http_escape(fd->title, NULL);
	nse->album = http_escape(fd->album, NULL);
	nse->length = len;
	nse->time = time(NULL);

	g_mutex_lock(scrobbler_lock);
	scrobbler_queue_insert_tail(nse);
	g_cond_signal(scrobbler_avail);
	g_mutex_unlock(scrobbler_lock);
}

void
scrobbler_notify_seek(struct audio_file *fd)
{
	fd->_scrobbler_done = 1;
}

/**
 * @brief Fetch as much tracks from the AudioScrobbler submission queue
 *        (at most 10), generate a HTTP/1.1 POST string for submission
 *        and return the amount of tracks described in the POST string.
 */
static unsigned int
scrobbler_queue_fetch(const char key[32], char **poststr)
{
	struct scrobbler_entry *ent;
	unsigned int len;
	GString *str;

	g_mutex_lock(scrobbler_lock);
	while ((ent = scrobbler_queue_first) == NULL)
		g_cond_wait(scrobbler_avail, scrobbler_lock);

	str = g_string_new("s=");
	g_string_append_len(str, key, 32);
	
	/* We can submit 50 tracks at a time */
	for (len = 0; (len < 50) && (ent != NULL); len++) {
		g_string_append_printf(str,
		    "&a[%u]=%s&t[%u]=%s&i[%u]=%u&o[%u]=P&r[%u]=&l[%u]=%u&b[%u]=%s&n[%u]=&m[%u]=",
		    len, ent->artist,
		    len, ent->title,
		    len, (unsigned int)ent->time,
		    len, /* No source */
		    len, /* No rating */
		    len, ent->length,
		    len, ent->album,
		    len, /* No track number */
		    len); /* No MusicBrainz ID */

		/* Go on to the next item */
		ent = scrobbler_queue_next(ent);
	}

	g_mutex_unlock(scrobbler_lock);

	*poststr = g_string_free(str, FALSE);

	/* Return the amount of entries in the poststring */
	g_assert(len >= 1);
	return (len);
}

/**
 * @brief Deallocate an AudioScrobbler queue entry.
 */
static void
scrobbler_queue_item_free(struct scrobbler_entry *ent)
{
	g_free(ent->artist);
	g_free(ent->title);
	g_free(ent->album);
	g_slice_free(struct scrobbler_entry, ent);
}

/**
 * @brief Remove a specified amount of tracks from the AudioScrobbler
 *        submission queue.
 */
static void
scrobbler_queue_remove(unsigned int amount)
{
	int i;
	struct scrobbler_entry *ent;

	g_mutex_lock(scrobbler_lock);
	for (i = amount; i > 0; i--) {
		ent = scrobbler_queue_first;
		scrobbler_queue_remove_head();
		scrobbler_queue_item_free(ent);
	}
	g_mutex_unlock(scrobbler_lock);
}

/**
 * @brief Generate a response, based on the password and challenge.
 */
static void
scrobbler_hash(time_t t, char out[32])
{
	char tstr[16];
	unsigned char bin_res[16];
	struct md5_context ctx = MD5CONTEXT_INITIALIZER;

	/*
	 * Generate the new MD5 value
	 */
	md5_update(&ctx, config_getopt("scrobbler.password"), 32);
	sprintf(tstr, "%u", (unsigned int)t);
	md5_update(&ctx, tstr, strlen(tstr));
	md5_final(&ctx, bin_res);

	/*
	 * Convert the result back to hexadecimal string
	 */
	hex_encode(bin_res, out, sizeof bin_res);
}

/**
 * @brief Concatenate cURL data to a string.
 */
static size_t
scrobbler_curl_concat(void *ptr, size_t size, size_t nmemb, void *stream)
{
	GString *response = stream;
	size_t len;

	len = size * nmemb;
	g_string_append_len(response, ptr, len);

	return (len);
}

/**
 * @brief Split a string into multiple nul-terminated lines.
 */
static void
scrobbler_split_lines(char *str, char *lines[], unsigned int nlines)
{
	unsigned int i;

	for (i = 0; i < nlines; i++) {
		/* Keep track of the line */
		lines[i] = str;

		/*
		 * If we haven't finished processing the string, move it
		 * on to the next line.
		 */
		if (str != NULL) {
			str = strchr(str, '\n');
			if (str != NULL)
				*str++ = '\0';
		}
	}
}

/**
 * @brief Send a handshake to the AudioScrobbler server. Also catch
 *         whether the configured password is correct.
 */
static int
scrobbler_send_handshake(char *key, char **url)
{
	char *hsurl;
	char hskey[33];
	time_t hstime;
	CURL *con;
	GString *response;
	char *lines[4];
	int ret;

	con = curl_easy_init();
	if (con == NULL)
		return (-1);
	
	/* Generate the connection URL, including the password key */
	hstime = time(NULL);
	scrobbler_hash(hstime, hskey);
	hskey[32] = '\0';
	hsurl = g_strdup_printf(SCROBBLER_URL,
	    config_getopt("scrobbler.hostname"),
	    config_getopt("scrobbler.username"),
	    (unsigned int)hstime, hskey);
	curl_easy_setopt(con, CURLOPT_URL, hsurl);
	curl_easy_setopt(con, CURLOPT_USERAGENT, APP_NAME "/" APP_VERSION);

	/* Callback function */
	response = g_string_sized_new(128);
	curl_easy_setopt(con, CURLOPT_WRITEFUNCTION, scrobbler_curl_concat);
	curl_easy_setopt(con, CURLOPT_WRITEDATA, response);

	/* Here we go */
	ret = curl_easy_perform(con);
	curl_easy_cleanup(con);
	g_free(hsurl);

	/* Connection error */
	if (ret != 0)
		goto done;

	ret = -1;
	scrobbler_split_lines(response->str, lines, 4);

	/* First line must be "OK" */
	if (lines[0] == NULL || strcmp(lines[0], "OK") != 0) {
		if (lines[0] != NULL && strcmp(lines[0], "BADAUTH") == 0)
			ret = 0;
		goto done;
	}
	/* Make sure the checksum is there */
	if (lines[1] == NULL || strlen(lines[1]) != 32)
		goto done;
	memcpy(key, lines[1], 32);

	/* Copy the submission URL */
	if (lines[3] == NULL)
		goto done;
	g_free(*url);
	*url = g_strdup(lines[3]);

	ret = 0;
done:
	g_string_free(response, TRUE);
	return (ret);
}

/**
 * @brief Submit an amount of tracks to the AudioScrobbler server. Also
 *        make sure whether the tracks are submitted properly.
 */
static int
scrobbler_send_tracks(char *key, const char *url, const char *poststr)
{
	CURL *con;
	GString *response;
	char *lines[1];
	int ret;

	con = curl_easy_init();
	if (con == NULL)
		return (-1);
	
	curl_easy_setopt(con, CURLOPT_URL, url);
	curl_easy_setopt(con, CURLOPT_POSTFIELDS, poststr);
	curl_easy_setopt(con, CURLOPT_USERAGENT, APP_NAME "/" APP_VERSION);

	/* Callback function */
	response = g_string_sized_new(128);
	curl_easy_setopt(con, CURLOPT_WRITEFUNCTION, scrobbler_curl_concat);
	curl_easy_setopt(con, CURLOPT_WRITEDATA, response);

	ret = curl_easy_perform(con);
	curl_easy_cleanup(con);

	/* Connection error */
	if (ret != 0)
		goto done;

	ret = -1;
	scrobbler_split_lines(response->str, lines, 1);

	/* Response string handling */
	if (lines[0] != NULL) {
		if (strcmp(lines[0], "OK") == 0)
			ret = 0;
		else if (strcmp(lines[0], "BADSESSION") == 0)
			/* Invalidate the session */
			key[0] = '\0';
	}
done:
	g_string_free(response, TRUE);
	return (ret);
}

/**
 * @brief The actual thread that gets spawned to process AudioScrobbler
 *        submission.
 */
static void *
scrobbler_runner_thread(void *unused)
{
	unsigned int amount, interval;
	char key[32] = "";
	char *poststr, *msg, *url = NULL;

	gui_input_sigmask();

	for(;;) {
		interval = 60;

		if (key[0] == '\0') {
			/* No key yet */
			if (scrobbler_send_handshake(key, &url) != 0) {
				/* Connection problems */
				gui_msgbar_warn(_("Failed to authorize "
				    "at AudioScrobbler."));
			} else if (key[0] == '\0') {
				/* We got BADAUTH */
				gui_msgbar_warn(_("Invalid AudioScrobbler "
				    "username/password."));
			} else {
				/* We've got a key */
				interval = 1;
				gui_msgbar_warn(_("Successfully "
				    "authorized at AudioScrobbler."));
			}
		} else {
			/*
			 * We are authorized. Send some tracks.
			 */
			amount = scrobbler_queue_fetch(key, &poststr);

			if (scrobbler_send_tracks(key, url, poststr) == 0) {
				/* Done. Remove them from the queue. */
				interval = 1;
				scrobbler_queue_remove(amount);

				/* Print a message on the GUI */
				if (amount == 1) {
					msg = g_strdup_printf(
					    _("Successfully sent 1 song "
					    "to AudioScrobbler."));
				} else {
					msg = g_strdup_printf(
					    _("Successfully sent %d songs "
					    "to AudioScrobbler."), amount);
				}
				gui_msgbar_warn(msg);
				g_free(msg);
			} else {
				gui_msgbar_warn(_("Failed to submit songs "
				    "to AudioScrobbler."));
			}

			g_free(poststr);
		}

		/* Make sure we don't transmit too fast */
		g_usleep(interval * 1000000);
	}
	
	g_assert_not_reached();
	return (NULL);
}

void
scrobbler_init(void)
{
	scrobbler_lock = g_mutex_new();
	scrobbler_avail = g_cond_new();
}

/**
 * @brief Dump the songs that are still present in the queue to disk.
 */
static void
scrobbler_queue_dump(void)
{
	const char *filename;
	FILE *fp;
	struct scrobbler_entry *ent;

	filename = config_getopt("scrobbler.dumpfile");
	if (filename[0] == '\0')
		return;
	
	/* Nothing to be stored - remove queue */
	if (scrobbler_queue_first == NULL) {
		vfs_delete(filename);
		return;
	}
	
	/* Write list to queue file */
	fp = vfs_fopen(filename, "w");
	if (fp == NULL)
		return;
	SCROBBLER_QUEUE_FOREACH(ent) {
		fprintf(fp, "%s %s %s %u %d\n",
		    ent->artist, ent->title, ent->album,
		    ent->length, (int)ent->time);
	}
	fclose(fp);
}

/**
 * @brief Restore the AudioScrobbler queue from a file.
 */
static void
scrobbler_queue_restore(void)
{
	const char *filename;
	FILE *fio;
	char fbuf[1024], *s1, *s2;
	struct scrobbler_entry **nse;

	filename = config_getopt("scrobbler.dumpfile");
	if (filename[0] == '\0')
		return;
	
	if ((fio = vfs_fopen(filename, "r")) == NULL)
		return;

	nse = &scrobbler_queue_first;
	while (vfs_fgets(fbuf, sizeof fbuf, fio) == 0) {
		/* String parsing sucks */
		*nse = g_slice_new(struct scrobbler_entry);

		/* Artist */
		if ((s1 = strchr(fbuf, ' ')) == NULL)
			goto bad;
		(*nse)->artist = g_strndup(fbuf, s1 - fbuf);
		/* Title */
		if ((s2 = strchr(++s1, ' ')) == NULL)
			goto bad;
		(*nse)->title = g_strndup(s1, s2 - s1);
		/* Album */
		if ((s1 = strchr(++s2, ' ')) == NULL)
			goto bad;
		(*nse)->album = g_strndup(s2, s1 - s2);
		/* Length and time */
		if ((s2 = strchr(++s1, ' ')) == NULL)
			goto bad;
		(*nse)->length = strtoul(s1, NULL, 10);
		(*nse)->time = strtol(++s2, NULL, 10);
		
		/* Properly fix up the list */
		scrobbler_queue_last = *nse;
		nse = &(*nse)->next;
		continue;

bad:		scrobbler_queue_item_free(*nse);
	}
	fclose(fio);

	/* Terminate our entry list */
	*nse = NULL;
}

void
scrobbler_spawn(void)
{
	/* Bail out if the username or password is not filled in */
	if (config_getopt("scrobbler.username")[0] == '\0' ||
	    config_getopt("scrobbler.password")[0] == '\0')
		return;

	/* Restore unsubmitted tracks */
	scrobbler_queue_restore();
	
	scrobbler_runner = g_thread_create(scrobbler_runner_thread,
	    NULL, 0, NULL);
	scrobbler_enabled = 1;
}

void
scrobbler_shutdown(void)
{
	/* XXX: bring down the AudioScrobbler thread */
	g_mutex_lock(scrobbler_lock);
	scrobbler_queue_dump();
	g_mutex_unlock(scrobbler_lock);
}
