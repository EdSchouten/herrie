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
 * @file gui_playq.c
 */

#include "audio_file.h"
#include "gui_internal.h"
#include "gui_vfslist.h"
#include "playq.h"

/**
 * @brief Playback time currently displayed, used to compare and discard
 *        redundant screen refreshes.
 */
static int time_cur = 0;
/**
 * @brief Track length currently displayed, used to compare and discard
 *        redundant screen refreshes.
 */
static int time_len = 0;
/**
 * @brief Buffer containing a string representation of the playback time
 *        of the current song in the form of "  [x:xx/y:yy]".
 */
static GString *str_time;
/**
 * @brief Buffer containing a string representation of the artist and
 *        title of the current song.
 */
static GString *str_song;
/**
 * @brief Window object of the status bar at the top of the screen.
 */
static WINDOW *win_statbar;
/**
 * @brief Reference to the window used to display the playlist.
 */
static struct gui_vfslist *win_playq;

/**
 * @brief Refresh the bar above the playlist to show the correct title
 *        and time of the current track and the percentage of the
 *        playlist.
 */
static void
gui_playq_statbar_refresh(void)
{
	const char *percent;
	int plen;

	GUI_LOCK;
	/* Blank it */
	werase(win_statbar);
	
	/* Put the content back */
	mvwaddstr(win_statbar, 0, 1, str_song->str);
	percent = gui_vfslist_getpercentage(win_playq);
	plen = strlen(percent);
	mvwaddstr(win_statbar, 0, COLS - str_time->len - plen, str_time->str);
	mvwaddstr(win_statbar, 0, COLS - plen, percent);

	/* And draw it */
	wrefresh(win_statbar);
	GUI_UNLOCK;
}

void
gui_playq_init(void)
{
	win_statbar = newwin(1, 0, 0, 0);
	if (gui_draw_colors)
		wbkgdset(win_statbar, COLOR_PAIR(GUI_COLOR_BAR));
	else
		wbkgdset(win_statbar, A_REVERSE);

	str_time = g_string_sized_new(24);
	str_song = g_string_sized_new(128);

	win_playq = gui_vfslist_new(1);
	gui_vfslist_setcallback(win_playq, gui_playq_statbar_refresh);
	gui_vfslist_setlist(win_playq, &playq_list);
	gui_vfslist_move(win_playq, 0, 1, COLS, gui_size_playq_height);

	gui_playq_song_update(NULL, 0);
}

void
gui_playq_destroy(void)
{
	delwin(win_statbar);
	gui_vfslist_destroy(win_playq);

	g_string_free(str_time, TRUE);
	g_string_free(str_song, TRUE);
}

/**
 * @brief Fills the str_song with the artist and song value of the audio
 *        file.
 */
static void
gui_playq_statbar_song(struct audio_file *fd)
{
#ifdef BUILD_UTF8
	const char *artist, *title;
#else /* !BUILD_UTF8 */
	char *artist, *title;
#endif /* BUILD_UTF8 */

	if (fd == NULL) {
		g_string_assign(str_song, _("Idle"));
	} else {
#ifdef BUILD_UTF8
		/* Display strings as UTF-8 */
		artist = fd->tag.artist ? fd->tag.artist :
		    _("Unknown artist");
		title = fd->tag.title ? fd->tag.title :
		    _("Unknown song");
		g_string_printf(str_song, _("Playing %s - %s"),
		    artist, title);
#else /* !BUILD_UTF8 */
		/* Smash strings down to ISO-8859-1 */
		if (fd->tag.artist != NULL)
			artist = g_convert(fd->tag.artist, -1,
			    "ISO-8859-1", "UTF-8", NULL, NULL, NULL);
		else
			artist = g_strdup(_("Unknown artist"));
		if (fd->tag.title != NULL)
			title = g_convert(fd->tag.title, -1,
			    "ISO-8859-1", "UTF-8", NULL, NULL, NULL);
		else
			title = g_strdup(_("Unknown song"));
		g_string_printf(str_song, _("Playing %s - %s"),
		    artist, title);
		g_free(artist);
		g_free(title);
#endif /* BUILD_UTF8 */
	}
}

/**
 * @brief Fills the str_time with the time value of the audio file.
 *        audio file. It also checks if the times are the same as
 *        before, useful to discard useless refreshes.
 */
static int
gui_playq_statbar_time(struct audio_file *fd)
{
	int ntc, ntl;

	if (fd == NULL) {
		ntc = ntl = -1;
	} else {
		ntc = fd->time_cur;
		ntl = fd->time_len;
	}
	if (ntc == time_cur && ntl == time_len)
		return (1);
	
	time_cur = ntc;
	time_len = ntl;

	if (time_cur == -1 && time_len == -1) {
		g_string_assign(str_time, "");
	} else {
		if (fd->time_len < 3600 && fd->time_cur < 3600) {
			/* Only show minutes and seconds */
			g_string_printf(str_time,
			    "  [%u:%02u/%u:%02u]",
			    time_cur / 60, time_cur % 60,
			    time_len / 60, time_len % 60);
		} else {
			/* Show hours as well */
			g_string_printf(str_time,
			    "  [%u:%02u:%02u/%u:%02u:%02u]",
			    time_cur / 3600, (time_cur / 60) % 60,
			    time_cur % 60, time_len / 3600,
			    (time_len / 60) % 60, time_len % 60);
		}
	}

	return (0);
}

void
gui_playq_song_update(struct audio_file *fd, int timeonly)
{
	int changed = 0;

	GUI_LOCK;
	if (!timeonly) {
		gui_playq_statbar_song(fd);
		changed = 1;
	}
	if (gui_playq_statbar_time(fd) == 0)
		changed = 1;
	GUI_UNLOCK;

	if (changed) {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		/* Print same message in ps(1) */
		setproctitle("%s%s", str_song->str, str_time->str);
#endif /* __FreeBSD__ || __NetBSD__ || __OpenBSD__ */
	
		gui_playq_statbar_refresh();
		gui_draw_async_done();
	}
}

void
gui_playq_resize(void)
{
	GUI_LOCK;
	wresize(win_statbar, 1, COLS);
	clearok(win_statbar, TRUE);
	GUI_UNLOCK;
	
	PLAYQ_LOCK;
	gui_vfslist_move(win_playq, 0, 1, COLS, gui_size_playq_height);
	PLAYQ_UNLOCK;
}

void
gui_playq_notify_pre_removal(unsigned int index)
{
	gui_vfslist_notify_pre_removal(win_playq, index);
}

void
gui_playq_notify_post_insertion(unsigned int index)
{
	gui_vfslist_notify_post_insertion(win_playq, index);
}

void
gui_playq_notify_post_randomization(void)
{
	gui_vfslist_notify_post_randomization(win_playq);
}

void
gui_playq_notify_done(void)
{
	gui_vfslist_notify_done(win_playq);
	gui_draw_async_done();
}

void
gui_playq_cursor_up(void)
{
	PLAYQ_LOCK;
	gui_vfslist_cursor_up(win_playq);
	PLAYQ_UNLOCK;
}

void
gui_playq_cursor_down(void)
{
	PLAYQ_LOCK;
	gui_vfslist_cursor_down(win_playq, 0);
	PLAYQ_UNLOCK;
}

void
gui_playq_cursor_pageup(void)
{
	PLAYQ_LOCK;
	gui_vfslist_cursor_pageup(win_playq);
	PLAYQ_UNLOCK;
}

void
gui_playq_cursor_pagedown(void)
{
	PLAYQ_LOCK;
	gui_vfslist_cursor_pagedown(win_playq);
	PLAYQ_UNLOCK;
}

void
gui_playq_cursor_top(void)
{
	PLAYQ_LOCK;
	gui_vfslist_cursor_top(win_playq);
	PLAYQ_UNLOCK;
}

void
gui_playq_cursor_bottom(void)
{
	PLAYQ_LOCK;
	gui_vfslist_cursor_bottom(win_playq);
	PLAYQ_UNLOCK;
}

void
gui_playq_song_remove(void)
{
	struct vfsref *vr;

	PLAYQ_LOCK;
	if (!gui_vfslist_warn_isempty(win_playq)) {
		vr = gui_vfslist_getselected(win_playq);
		playq_song_fast_remove(vr,
		    gui_vfslist_getselectedidx(win_playq));
	}
	PLAYQ_UNLOCK;
}

void
gui_playq_song_remove_all(void)
{
	/* Don't care about locking here */
	if (gui_vfslist_warn_isempty(win_playq))
		return;

	if (gui_input_askyesno(_("Remove all songs from the playlist?")) == 0)
		playq_song_remove_all();
}

void
gui_playq_song_randomize(void)
{
	/* Don't care about locking here */
	if (gui_vfslist_warn_isempty(win_playq))
		return;

	if (gui_input_askyesno(_("Randomize the playlist?")) == 0)
		playq_song_randomize();
}

void
gui_playq_song_add_before(struct vfsref *vr)
{
	struct vfsref *vr_selected;

	PLAYQ_LOCK;
	vr_selected = gui_vfslist_getselected(win_playq);
	if (vr_selected == vfs_list_first(&playq_list)) {
		PLAYQ_UNLOCK;
		/* List is empty or adding before the first node */
		playq_song_add_head(vr);
	} else {
		playq_song_fast_add_before(vr, vr_selected,
		    gui_vfslist_getselectedidx(win_playq));
		PLAYQ_UNLOCK;
	}
}

void
gui_playq_song_add_after(struct vfsref *vr)
{
	struct vfsref *vr_selected;

	PLAYQ_LOCK;
	vr_selected = gui_vfslist_getselected(win_playq);
	if (vr_selected == vfs_list_last(&playq_list)) {
		PLAYQ_UNLOCK;
		/* List is empty or appending after the last node */
		playq_song_add_tail(vr);
	} else {
		playq_song_fast_add_after(vr, vr_selected,
		    gui_vfslist_getselectedidx(win_playq));
		PLAYQ_UNLOCK;
	}
}

void
gui_playq_song_moveup(void)
{
	struct vfsref *vr_selected;

	PLAYQ_LOCK;
	if (!gui_vfslist_warn_isempty(win_playq)) {
		vr_selected = gui_vfslist_getselected(win_playq);
		if (vr_selected == vfs_list_first(&playq_list)) {
			gui_msgbar_warn(_("The song is already at the "
			    "top of the playlist."));
		} else {
			playq_song_fast_moveup(vr_selected,
			    gui_vfslist_getselectedidx(win_playq));
		}
	}
	PLAYQ_UNLOCK;
}

void
gui_playq_song_movedown(void)
{
	struct vfsref *vr_selected;

	PLAYQ_LOCK;
	if (!gui_vfslist_warn_isempty(win_playq)) {
		vr_selected = gui_vfslist_getselected(win_playq);
		if (vr_selected == vfs_list_last(&playq_list)) {
			gui_msgbar_warn(_("The song is already at the "
			    "bottom of the playlist."));
		} else {
			playq_song_fast_movedown(vr_selected,
			    gui_vfslist_getselectedidx(win_playq));
		}
	}
	PLAYQ_UNLOCK;
}

void
gui_playq_searchnext(void)
{
	PLAYQ_LOCK;
	gui_vfslist_searchnext(win_playq);
	PLAYQ_UNLOCK;
}

void
gui_playq_setfocus(int focus)
{
	PLAYQ_LOCK;
	gui_vfslist_setfocus(win_playq, focus);
	PLAYQ_UNLOCK;
}
