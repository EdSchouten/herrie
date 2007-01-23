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
 * @file gui_browser.c
 */

#include "config.h"
#include "gui_internal.h"
#include "gui_vfslist.h"
#include "playq.h"
#include "vfs.h"

/**
 * @brief Reference to window with pathname of current folder.
 */
static WINDOW *win_dirname;
/**
 * @brief Reference to VFS object that is currently shown.
 */
static struct vfsref *vr_curdir = NULL;
/**
 * @brief List we use when displaying a file instead of a directory,
 *        containing only the file itself.
 */
static struct vfslist vl_flist;
/**
 * @brief Reference to window used as the file browser.
 */
static struct gui_vfslist *win_browser;

/**
 * @brief Refresh the bar above the filebrowser to contain the proper
 *        filename and percentage.
 */
static void
gui_browser_dirname_refresh(void)
{
	const char *percent;
	int plen;

	GUI_LOCK;

	werase(win_dirname);
	if (vr_curdir != NULL) {
		mvwaddstr(win_dirname, 0, 1, vfs_filename(vr_curdir));
	}

	percent = gui_vfslist_getpercentage(win_browser);
	plen = strlen(percent);
	mvwaddstr(win_dirname, 0, COLS - plen, percent);
	wnoutrefresh(win_dirname);

	GUI_UNLOCK;
}

void
gui_browser_init(void)
{
	const char *defdir;
	char *cwd;

	win_dirname = newwin(1, 0, gui_size_browser_dirname_top, 0);
	clearok(win_dirname, TRUE);
	if (gui_draw_colors)
		wbkgdset(win_dirname, COLOR_PAIR(GUI_COLOR_BAR));
	else
		wbkgdset(win_dirname, A_REVERSE);

	win_browser = gui_vfslist_new(0);
	gui_vfslist_setfocus(win_browser, 1);
	gui_vfslist_setcallback(win_browser, gui_browser_dirname_refresh);
	gui_browser_dirname_refresh();

	defdir = config_getopt("gui.browser.defaultpath");
	if (defdir[0] != '\0') {
		/* Open predefined directory */
		vr_curdir = vfs_open(defdir, NULL, NULL);
	} else {
		/* Open current directory */
		cwd = g_get_current_dir();
		vr_curdir = vfs_open(cwd, NULL, NULL);
		g_free(cwd);
	}

	if (vr_curdir != NULL) {
		vfs_populate(vr_curdir);
		gui_vfslist_setlist(win_browser, vfs_population(vr_curdir));
	} else {
		gui_msgbar_warn(_("Unable to open initial directory."));
	}
	gui_vfslist_move(win_browser, 0, gui_size_browser_top,
	    COLS, gui_size_browser_height);
}

void
gui_browser_destroy(void)
{
	delwin(win_dirname);
	gui_vfslist_destroy(win_browser);
	if (vr_curdir != NULL)
		vfs_close(vr_curdir);
}

void
gui_browser_resize(void)
{
	GUI_LOCK;
	wresize(win_dirname, 1, COLS);
	mvwin(win_dirname, gui_size_browser_dirname_top, 0);
	clearok(win_dirname, TRUE);
	GUI_UNLOCK;

	gui_vfslist_move(win_browser, 0, gui_size_browser_top,
	    COLS, gui_size_browser_height);
}

/*
 * Cursor manipulation
 */

void
gui_browser_cursor_up(void)
{
	gui_vfslist_cursor_up(win_browser);
}

void
gui_browser_cursor_down(void)
{
	gui_vfslist_cursor_down(win_browser, 0);
}

void
gui_browser_cursor_pageup(void)
{
	gui_vfslist_cursor_pageup(win_browser);
}

void
gui_browser_cursor_pagedown(void)
{
	gui_vfslist_cursor_pagedown(win_browser);
}

void
gui_browser_cursor_top(void)
{
	gui_vfslist_cursor_top(win_browser);
}

void
gui_browser_cursor_bottom(void)
{
	gui_vfslist_cursor_bottom(win_browser);
}

/*
 * Change current directory
 */

void
gui_browser_dir_parent(void)
{
	struct vfsref *vr, *vrn;
	unsigned int idx;

	if (vr_curdir == NULL)
		return;

	if ((vr = vfs_open("..", NULL, vfs_filename(vr_curdir))) == NULL)
		goto bad;
	if (vfs_populate(vr) != 0) {
		/* Permission denied? */
		vfs_close(vr);
		goto bad;
	}
	
	for (vrn = vfs_list_first(vfs_population(vr)), idx = 1;
	    vrn != NULL; vrn = vfs_list_next(vrn), idx++) {
		/* Select the previous directory */
		if (strcmp(vfs_name(vr_curdir), vfs_name(vrn)) == 0)
			break;
	}

	vfs_close(vr_curdir);
	vr_curdir = vr;
	gui_vfslist_setlist(win_browser, vfs_population(vr_curdir));

	if (vrn != NULL)
		gui_vfslist_setselected(win_browser, vrn, idx);

	return;
bad:
	gui_msgbar_warn(_("Unable to enter the parent directory."));
}

void
gui_browser_dir_enter(void)
{
	struct vfsref *vr;

	if ((vr = gui_vfslist_getselected(win_browser)) == NULL)
		return;

	vr = vfs_dup(vr);
	if (vfs_populate(vr) != 0) {
		if (vfs_populatable(vr)) {
			/* Permission denied? */
			gui_msgbar_warn(_("Unable to enter the "
			    "selected directory."));
		}
		vfs_close(vr);
		return;
	}

	vfs_close(vr_curdir);
	vr_curdir = vr;
	gui_vfslist_setlist(win_browser, vfs_population(vr_curdir));
}

/*
 * Playlist manipulation
 */

void
gui_browser_playq_add_tail(void)
{
	struct vfsref *vr;

	if (gui_vfslist_warn_isempty(win_browser))
		return;

	vr = gui_vfslist_getselected(win_browser);
	playq_song_add_tail(vr);
	gui_vfslist_cursor_down(win_browser, 1);
}

void
gui_browser_playq_add_head(void)
{
	struct vfsref *vr;

	if (gui_vfslist_warn_isempty(win_browser))
		return;

	vr = gui_vfslist_getselected(win_browser);
	playq_song_add_head(vr);
	gui_vfslist_cursor_down(win_browser, 1);
}

void
gui_browser_playq_add_after(void)
{
	struct vfsref *vr;

	if (gui_vfslist_warn_isempty(win_browser))
		return;

	vr = gui_vfslist_getselected(win_browser);
	gui_playq_song_add_after(vr);
	gui_vfslist_cursor_down(win_browser, 1);
}

void
gui_browser_playq_add_before(void)
{
	struct vfsref *vr;

	if (gui_vfslist_warn_isempty(win_browser))
		return;

	vr = gui_vfslist_getselected(win_browser);
	gui_playq_song_add_before(vr);
	gui_vfslist_cursor_down(win_browser, 1);
}

void
gui_browser_searchnext(void)
{
	gui_vfslist_searchnext(win_browser);
}

void
gui_browser_chdir(void)
{
	char *path;
	int dir = 0;
	struct vfsref *vr;

	path = gui_input_askstring(_("Change directory"), NULL, NULL);
	if (path == NULL)
		return;

	if (vr_curdir != NULL) {
		/* Relative to the current node */
		vr = vfs_open(path, NULL, vfs_filename(vr_curdir));
	} else {
		/* Relative to the root */
		vr = vfs_open(path, NULL, NULL);
	}
	g_free(path);

	if (vr != NULL) {
		if (vfs_populate(vr) == 0)
			dir = 1;
		else if (!vfs_playable(vr))
			goto bad;

		/* Replace old directory */
		if (vr_curdir != NULL)
			vfs_close(vr_curdir);
		vr_curdir = vr;

		if (dir) {
			/* Go inside the directory */
			gui_vfslist_setlist(win_browser,
			    vfs_population(vr));
		} else {
			/* Create a fake directory */
			vfs_list_init(&vl_flist);
			vfs_list_insert_tail(&vl_flist, vr);
			gui_vfslist_setlist(win_browser, &vl_flist);
		}

		return;
	}

bad:	gui_msgbar_warn(_("Unable to display the file or directory."));
	if (vr != NULL)
		vfs_close(vr);
}

void
gui_browser_setfocus(int focus)
{
	gui_vfslist_setfocus(win_browser, focus);
}
