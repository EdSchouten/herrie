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
 * @file scrobbler.c
 */

#include <time.h>

#include "config.h"
#include "gui.h"
#include "scrobbler.h"
#include "scrobbler_internal.h"

/**
 * @brief Flag indicating if the AudioScrobbler thread has already been
 *        launched, causing the submission queue to be filled.
 */
static char scrobbler_enabled = 0;

/**
 * @brief Lock used to provide safe access to the AudioScrobbler queue.
 */
static GMutex 			*scrobbler_lock;
/**
 * @brief Conditional variable used to notify the avaiability of new
 *        tracks ready for submission to AudioScrobbler.
 */
static GCond			*scrobbler_avail;
/**
 * @brief Reference to AudioScrobbler submission thread.
 */
static GThread			*scrobbler_runner;

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
	int	length;
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
#define scrobbler_queue_next(se)	((se)->next)
/**
 * @brief Place a new item in our Scrobbler queue.
 */
#define scrobbler_queue_insert_tail(se)					\
do {									\
	scrobbler_queue_next(se) = NULL;				\
	if (scrobbler_queue_last != NULL)				\
		scrobbler_queue_last->next = (se);			\
	else								\
		scrobbler_queue_first = (se);				\
	scrobbler_queue_last = (se);					\
} while (0)
/**
 * @brief Remove the first item from our Scrobbler queue.
 */
#define scrobbler_queue_remove_head()					\
do {									\
	if (scrobbler_queue_first != NULL)				\
		if (scrobbler_queue_first == scrobbler_queue_last)	\
			scrobbler_queue_last = NULL;			\
		scrobbler_queue_first = scrobbler_queue_first->next;	\
} while (0)

/**
 * @brief Escape a string according to HTTP/1.1.
 */
static char *
scrobbler_http_escape(const char *str)
{
	const char *c;
	const char allowed[] = "-_.!~*'()";
	GString *ret;

	/* Argument may be empty */
	if (str == NULL)
		return g_strdup("");

	ret = g_string_sized_new(32);

	for (c = str; *c != '\0'; c++) {
		if (g_ascii_isalnum(*c) || (strchr(allowed, *c) != NULL))
			/* Character is allowed */
			g_string_append_c(ret, *c);
		else
			/* Reserved or unwise character */
			g_string_append_printf(ret, "%%%02hhx", *c);
	}

	return g_string_free(ret, FALSE);
}

void
scrobbler_notify_read(struct audio_file *fd, int eof)
{
	struct scrobbler_entry *new;
	int len;

	if (!scrobbler_enabled)
		return;

	/* We already processed the song */
	if (fd->_scrobbler_done)
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
	if (fd->tag.title == NULL ||
	    (fd->tag.artist == NULL && fd->tag.album == NULL))
		return;

	/* Place the track in our queue */
	new = g_slice_new(struct scrobbler_entry);
	new->artist = scrobbler_http_escape(fd->tag.artist);
	new->title = scrobbler_http_escape(fd->tag.title);
	new->album = scrobbler_http_escape(fd->tag.album);
	new->length = len;
	new->time = time(NULL);

	g_mutex_lock(scrobbler_lock);
	scrobbler_queue_insert_tail(new);
	g_cond_signal(scrobbler_avail);
	g_mutex_unlock(scrobbler_lock);
}

/**
 * @brief Fetch as much tracks from the AudioScrobbler submission queue
 *        (at most 10), generate a HTTP/1.1 POST string for submission
 *        and return the amount of tracks described in the POST string.
 */
static int
scrobbler_queue_fetch(struct scrobbler_condata *scd, char **poststr)
{
	char timestr[20];
	struct scrobbler_entry *ent;
	int len = 0;
	GString *str;

	g_mutex_lock(scrobbler_lock);
	while ((ent = scrobbler_queue_first) == NULL)
		g_cond_wait(scrobbler_avail, scrobbler_lock);

	str = g_string_sized_new(256);
	g_string_printf(str, "u=%s&s=", scd->username);
	g_string_append_len(str, scd->response, 32);
	
	for (len = 0; (len < 10) && (ent != NULL); len++) {
		strftime(timestr, sizeof timestr, "%G-%m-%d %H:%M:%S",
		    gmtime(&ent->time));

		g_string_append_printf(str,
		    "&a[%d]=%s&t[%d]=%s&b[%d]=%s&m[%d]=&l[%d]=%d&i[%d]=%s",
		    len, ent->artist,
		    len, ent->title,
		    len, ent->album,
		    len, /* No MusicBrainz ID */
		    len, ent->length,
		    len, timestr);

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
 * @brief Remove a specified amount of tracks from the AudioScrobbler
 *        submission queue.
 */
static void
scrobbler_queue_remove(int amount)
{
	int i;
	struct scrobbler_entry *ent;

	g_mutex_lock(scrobbler_lock);
	for (i = amount; i > 0; i--) {
		ent = scrobbler_queue_first;
		g_assert(ent != NULL);
		scrobbler_queue_remove_head();

		g_free(ent->artist);
		g_free(ent->title);
		g_free(ent->album);
		g_slice_free(struct scrobbler_entry, ent);
	}
	g_mutex_unlock(scrobbler_lock);
}

/**
 * @brief The actual thread that gets spawned to process AudioScrobbler
 *        submission.
 */
static void *
scrobbler_runner_thread(void *arg)
{
	struct scrobbler_condata *scd = arg;
	int amount;
	char *poststr, *msg;

	gui_input_sigmask();

	for(;;) {
		/* Wait a little longer when we can't connect */
		scd->interval = 60;

		if (!scd->authorized) {
			/*
			 * We are not authorized yet. Try to
			 * authenticate first.
			 */
			if (scrobbler_send_handshake(scd) == 0) {
				gui_msgbar_warn(_("Successfully "
				    "authorized at AudioScrobbler."));

				/* Hash the challenge */
				scrobbler_hash(scd);
				scd->authorized = 1;
			} else {
				gui_msgbar_warn(_("Failed to authorize "
				    "at AudioScrobbler."));
			}
		} else {
			/*
			 * We are authorized. Send some tracks.
			 */
			amount = scrobbler_queue_fetch(scd, &poststr);

			if (scrobbler_send_tracks(scd, poststr) == 0) {
				/* Done. Remove them from the queue. */
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
			} else if (scd->authorized == 0) {
				gui_msgbar_warn(_("Invalid AudioScrobbler "
				    "username/password."));
			} else {
				gui_msgbar_warn(_("Failed to submit songs "
				    "to AudioScrobbler."));
			}

			g_free(poststr);
		}

		/* Make sure we don't transmit too fast */
		g_usleep(MAX(scd->interval, 1) * 1000000);
	}
	
	return (NULL);
}

void
scrobbler_init(void)
{
	scrobbler_lock = g_mutex_new();
	scrobbler_avail = g_cond_new();
}

void
scrobbler_spawn(void)
{
	struct scrobbler_condata *scd;
	const char *su, *sp;

	su = config_getopt("scrobbler.username");
	sp = config_getopt("scrobbler.password");

	/* Bail out if the username or password is not filled in */
	if (su[0] == '\0' || sp[0] == '\0')
		return;
	
	/* Connection local storage */
	scd = g_slice_new0(struct scrobbler_condata);
	scd->username = scrobbler_http_escape(su);
	scd->password = sp;
	
	scrobbler_runner = g_thread_create(scrobbler_runner_thread, scd, 0, NULL);
	scrobbler_enabled = 1;
}
