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
 * @file gui_playq.c
 * @brief Playlist management for textual user interface.
 */

#include "stdinc.h"

#include "audio_file.h"
#include "audio_output.h"
#include "gui.h"
#include "gui_internal.h"
#include "gui_vfslist.h"
#include "playq.h"
#include "vfs.h"

/**
 * @brief Buffer containing a string representation of the playback time
 *        of the current song in the form of " [x:xx/y:yy]".
 */
static GString *str_time;
/**
 * @brief Pointer to string containing playback status.
 */
static const char *str_status = NULL;
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
 * @brief Fills the str_song with the artist and song value of the audio
 *        file.
 */
static void
gui_playq_statbar_song(struct audio_file *fd)
{
	char *artist, *title;

	if (fd == NULL) {
		g_string_assign(str_song, "");
	} else {
		/* Smash strings down to the correct charset */
		g_assert(fd->title != NULL);
		title = g_convert_with_fallback(fd->title,
		    -1, "", "UTF-8", "?", NULL, NULL, NULL);
		if (title == NULL)
			/* Conversion error - don't convert charset */
			title = g_strdup(fd->title);

		if (fd->artist == NULL) {
			/* Only show the title */
			g_string_assign(str_song, title);
		} else {
			/* Print artist and title */
			artist = g_convert_with_fallback(fd->artist,
			    -1, "", "UTF-8", "?", NULL, NULL, NULL);
			if (artist == NULL)
				/* Conversion error */
				artist = g_strdup(fd->artist);
			g_string_printf(str_song, "%s - %s",
			    artist, title);
			g_free(artist);
		}
		g_free(title);
	}
}

/**
 * @brief Set str_status to the current playback status.
 */
static void
gui_playq_statbar_status(struct audio_file *fd, int paused)
{
	if (fd == NULL) {
		str_status = _("Idle");
	} else {
		if (paused)
			str_status = _("Paused");
		else
			str_status = _("Playing");
	}
}

/**
 * @brief Append a formatted string representation of a timestamp to a
 *        string.
 */
static void
gui_playq_statbar_time_calc(GString *str, unsigned int time)
{
	if (time < 3600)
		/* Only show minutes and seconds */
		g_string_append_printf(str, "%u:%02u",
		    time / 60, time % 60);
	else
		/* Show hours as well */
		g_string_append_printf(str, "%u:%02u:%02u",
		    time / 3600, (time / 60) % 60, time % 60);
}

/**
 * @brief Fills the str_time with the time value of the audio file.
 *        audio file. It also checks if the times are the same as
 *        before, useful to discard useless refreshes.
 */
static void
gui_playq_statbar_time(struct audio_file *fd)
{
	if (fd == NULL) {
		g_string_assign(str_time, "");
	} else {
		g_string_assign(str_time, " [");
		gui_playq_statbar_time_calc(str_time, fd->time_cur);
		if (!fd->stream) {
			g_string_append_c(str_time, '/');
			gui_playq_statbar_time_calc(str_time, fd->time_len);
		}
		g_string_append_c(str_time, ']');
	}
}

/**
 * @brief Set the name of the song and time showed in the status bar.
 */
static void
gui_playq_song_set(struct audio_file *fd, int paused, int timeonly)
{
	gui_lock();
	if (!timeonly)
		gui_playq_statbar_song(fd);
	gui_playq_statbar_time(fd);
	gui_playq_statbar_status(fd, paused);
	gui_unlock();
}

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

	gui_lock();
	/* Blank it */
	werase(win_statbar);

	/* Put the content back */
	mvwaddstr(win_statbar, 0, 1, str_status);
	waddstr(win_statbar, " | ");
	waddstr(win_statbar, str_song->str);

	percent = gui_vfslist_getpercentage(win_playq);
	plen = strlen(percent);
	mvwaddstr(win_statbar, 0, COLS - str_time->len - plen, str_time->str);
	waddstr(win_statbar, percent);

	/* And draw it */
	wnoutrefresh(win_statbar);
	gui_unlock();
}

void
gui_playq_init(void)
{
	win_statbar = newwin(1, 0, 0, 0);
	clearok(win_statbar, TRUE);
	if (gui_draw_colors)
		wbkgdset(win_statbar, COLOR_PAIR(GUI_COLOR_BAR));
	else
		wbkgdset(win_statbar, A_REVERSE);

	str_time = g_string_sized_new(24);
	str_song = g_string_sized_new(128);

	gui_playq_song_set(NULL, 0, 0);

	win_playq = gui_vfslist_new(1);
	gui_vfslist_setcallback(win_playq, gui_playq_statbar_refresh);
	gui_vfslist_setlist(win_playq, &playq_list);
	gui_vfslist_move(win_playq, 0, 1, COLS, GUI_SIZE_PLAYQ_HEIGHT);
}

void
gui_playq_destroy(void)
{
	delwin(win_statbar);
	gui_vfslist_destroy(win_playq);

	g_string_free(str_time, TRUE);
	g_string_free(str_song, TRUE);
}

void
gui_playq_song_update(struct audio_file *fd, int paused, int timeonly)
{
	gui_playq_song_set(fd, paused, timeonly);

#ifdef BUILD_SETPROCTITLE
	/* Print same message in ps(1) */
	setproctitle("%s %s%s", str_status, str_song->str, str_time->str);
#endif /* BUILD_SETPROCTITLE */

	gui_playq_statbar_refresh();
	gui_draw_done();
}

void
gui_playq_resize(void)
{
	gui_lock();
	wresize(win_statbar, 1, COLS);
	clearok(win_statbar, TRUE);
	gui_unlock();

	playq_lock();
	gui_vfslist_move(win_playq, 0, 1, COLS, GUI_SIZE_PLAYQ_HEIGHT);
	playq_unlock();
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
	gui_draw_done();
}

void
gui_playq_cursor_up(void)
{
	playq_lock();
	gui_vfslist_cursor_up(win_playq);
	playq_unlock();
}

void
gui_playq_cursor_down(void)
{
	playq_lock();
	gui_vfslist_cursor_down(win_playq, 0);
	playq_unlock();
}

void
gui_playq_cursor_pageup(void)
{
	playq_lock();
	gui_vfslist_cursor_pageup(win_playq);
	playq_unlock();
}

void
gui_playq_cursor_pagedown(void)
{
	playq_lock();
	gui_vfslist_cursor_pagedown(win_playq);
	playq_unlock();
}

void
gui_playq_cursor_head(void)
{
	playq_lock();
	gui_vfslist_cursor_head(win_playq);
	playq_unlock();
}

void
gui_playq_cursor_tail(void)
{
	playq_lock();
	gui_vfslist_cursor_tail(win_playq);
	playq_unlock();
}

void
gui_playq_song_remove(void)
{
	struct vfsref *vr;

	playq_lock();
	if (!gui_vfslist_warn_isempty(win_playq)) {
		vr = gui_vfslist_getselected(win_playq);
		playq_song_fast_remove(vr,
		    gui_vfslist_getselectedidx(win_playq));
	}
	playq_unlock();
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

	playq_lock();
	vr_selected = gui_vfslist_getselected(win_playq);
	if (vr_selected == vfs_list_first(&playq_list)) {
		playq_unlock();
		/* List is empty or adding before the first node */
		playq_song_add_head(vr);
	} else {
		playq_song_fast_add_before(vr, vr_selected,
		    gui_vfslist_getselectedidx(win_playq));
		playq_unlock();
	}
}

void
gui_playq_song_add_after(struct vfsref *vr)
{
	struct vfsref *vr_selected;

	playq_lock();
	vr_selected = gui_vfslist_getselected(win_playq);
	if (vr_selected == vfs_list_last(&playq_list)) {
		playq_unlock();
		/* List is empty or appending after the last node */
		playq_song_add_tail(vr);
	} else {
		playq_song_fast_add_after(vr, vr_selected,
		    gui_vfslist_getselectedidx(win_playq));
		playq_unlock();
	}
}

void
gui_playq_song_move_up(void)
{
	struct vfsref *vr_selected;

	playq_lock();
	if (!gui_vfslist_warn_isempty(win_playq)) {
		vr_selected = gui_vfslist_getselected(win_playq);
		if (vr_selected == vfs_list_first(&playq_list)) {
			gui_msgbar_warn(_("The song is already at the "
			    "top of the playlist."));
		} else {
			playq_song_fast_move_up(vr_selected,
			    gui_vfslist_getselectedidx(win_playq));
		}
	}
	playq_unlock();
}

void
gui_playq_song_move_down(void)
{
	struct vfsref *vr_selected;

	playq_lock();
	if (!gui_vfslist_warn_isempty(win_playq)) {
		vr_selected = gui_vfslist_getselected(win_playq);
		if (vr_selected == vfs_list_last(&playq_list)) {
			gui_msgbar_warn(_("The song is already at the "
			    "bottom of the playlist."));
		} else {
			playq_song_fast_move_down(vr_selected,
			    gui_vfslist_getselectedidx(win_playq));
		}
	}
	playq_unlock();
}

void
gui_playq_song_move_head(void)
{
	struct vfsref *vr_selected;

	playq_lock();
	if (!gui_vfslist_warn_isempty(win_playq)) {
		vr_selected = gui_vfslist_getselected(win_playq);
		if (vr_selected == vfs_list_first(&playq_list)) {
			gui_msgbar_warn(_("The song is already at the "
			    "top of the playlist."));
		} else {
			playq_song_fast_move_head(vr_selected,
			    gui_vfslist_getselectedidx(win_playq));
		}
	}
	playq_unlock();
}

void
gui_playq_song_move_tail(void)
{
	struct vfsref *vr_selected;

	playq_lock();
	if (!gui_vfslist_warn_isempty(win_playq)) {
		vr_selected = gui_vfslist_getselected(win_playq);
		if (vr_selected == vfs_list_last(&playq_list)) {
			gui_msgbar_warn(_("The song is already at the "
			    "bottom of the playlist."));
		} else {
			playq_song_fast_move_tail(vr_selected,
			    gui_vfslist_getselectedidx(win_playq));
		}
	}
	playq_unlock();
}

void
gui_playq_song_select(void)
{
	playq_lock();
	if (!gui_vfslist_warn_isempty(win_playq))
		playq_song_fast_select(gui_vfslist_getselected(win_playq));
	playq_unlock();
}

int
gui_playq_searchnext(const struct vfsmatch *vm)
{
	int ret;

	playq_lock();
	ret = gui_vfslist_searchnext(win_playq, vm);
	playq_unlock();

	return (ret);
}

void
gui_playq_setfocus(int focus)
{
	playq_lock();
	gui_vfslist_setfocus(win_playq, focus);
	playq_unlock();
}

void
gui_playq_fullpath(void)
{
	playq_lock();
	gui_vfslist_fullpath(win_playq);
	playq_unlock();
}

#ifdef BUILD_VOLUME
/**
 * @brief Show the result of the volume setting routines.
 */
static void
gui_playq_volume_show(int nval)
{
	char *str;

	if (nval < 0) {
		gui_msgbar_warn(_("Failed to adjust the volume."));
	} else {
		str = g_strdup_printf(_("Volume: %d%%"), nval);
		gui_msgbar_warn(str);
		g_free(str);
	}
}

void
gui_playq_volume_up(void)
{
	int nval;

	nval = audio_output_volume_up();
	gui_playq_volume_show(nval);
}

void
gui_playq_volume_down(void)
{
	int nval;

	nval = audio_output_volume_down();
	gui_playq_volume_show(nval);
}
#endif /* BUILD_VOLUME */

void
gui_playq_gotofolder(void)
{
	struct vfsref *vr;

	playq_lock();
	if (gui_vfslist_warn_isempty(win_playq)) {
		playq_unlock();
	} else {
		vr = gui_vfslist_getselected(win_playq);
		playq_unlock();

		gui_browser_gotofile(vr);
	}
}
