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
 * @file gui_browser.c
 * @brief File browser in textual user interface.
 */

#include "stdinc.h"

#include "config.h"
#include "gui.h"
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
static struct vfslist vl_flist = VFSLIST_INITIALIZER;
/**
 * @brief Reference to window used as the file browser.
 */
static struct gui_vfslist *win_browser;
/**
 * @brief The current filtering string that's being applied.
 */
static char *locatestr = NULL;

/**
 * @brief Refresh the bar above the filebrowser to contain the proper
 *        filename and percentage.
 */
static void
gui_browser_dirname_refresh(void)
{
	const char *percent;
	int plen;

	gui_lock();

	werase(win_dirname);
	if (vr_curdir != NULL) {
		mvwaddstr(win_dirname, 0, 1, vfs_filename(vr_curdir));
		if (locatestr != NULL) {
			waddstr(win_dirname, " (");
			waddstr(win_dirname, _("filter"));
			waddstr(win_dirname, ": ");
			waddstr(win_dirname, locatestr);
			waddstr(win_dirname, ")");
		}
	}

	percent = gui_vfslist_getpercentage(win_browser);
	plen = strlen(percent);
	mvwaddstr(win_dirname, 0, COLS - plen, percent);
	wnoutrefresh(win_dirname);

	gui_unlock();
}

/**
 * @brief Clean up our pseudo-directory data.
 */
static void
gui_browser_cleanup_flist(void)
{
	struct vfsref *vr;

	/* Close our fake view */
	while ((vr = vfs_list_first(&vl_flist)) != NULL) {
		vfs_list_remove(&vl_flist, vr);
		vfs_close(vr);
	}

	g_free(locatestr);
	locatestr = NULL;
}

void
gui_browser_init(void)
{
	const char *defdir;
	char *cwd;

	win_dirname = newwin(1, 0, GUI_SIZE_BROWSER_DIRNAME_TOP, 0);
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
		vr_curdir = vfs_lookup(defdir, NULL, NULL, 0);
	} else {
		/* Open current directory */
		cwd = g_get_current_dir();
		vr_curdir = vfs_lookup(cwd, NULL, NULL, 1);
		g_free(cwd);
	}

	if (vr_curdir != NULL) {
		vfs_populate(vr_curdir);
		gui_vfslist_setlist(win_browser, vfs_population(vr_curdir));
	} else {
		gui_msgbar_warn(_("Unable to open initial directory."));
	}
	gui_vfslist_move(win_browser, 0, GUI_SIZE_BROWSER_TOP,
	    COLS, GUI_SIZE_BROWSER_HEIGHT);
}

void
gui_browser_destroy(void)
{
	delwin(win_dirname);
	gui_vfslist_destroy(win_browser);

	/* Clean up our mess */
	gui_browser_cleanup_flist();
	if (vr_curdir != NULL)
		vfs_close(vr_curdir);
}

void
gui_browser_resize(void)
{
	gui_lock();
	wresize(win_dirname, 1, COLS);
	mvwin(win_dirname, GUI_SIZE_BROWSER_DIRNAME_TOP, 0);
	clearok(win_dirname, TRUE);
	gui_unlock();

	gui_vfslist_move(win_browser, 0, GUI_SIZE_BROWSER_TOP,
	    COLS, GUI_SIZE_BROWSER_HEIGHT);
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
gui_browser_cursor_head(void)
{
	gui_vfslist_cursor_head(win_browser);
}

void
gui_browser_cursor_tail(void)
{
	gui_vfslist_cursor_tail(win_browser);
}

/*
 * Change current directory
 */

void
gui_browser_gotofile(struct vfsref *vr)
{
	struct vfsref *vrp, *vrn;
	unsigned int idx;

	if ((vrp = vfs_lookup("..", NULL, vfs_filename(vr), 1)) == NULL)
		goto bad;
	if (vfs_populate(vrp) != 0) {
		/* Permission denied? */
		vfs_close(vrp);
		goto bad;
	}
	
	for (vrn = vfs_list_first(vfs_population(vrp)), idx = 1;
	    vrn != NULL; vrn = vfs_list_next(vrn), idx++) {
		/* Select the previous directory */
		if (strcmp(vfs_name(vr), vfs_name(vrn)) == 0)
			break;
	}

	/* Change the directory */
	gui_browser_cleanup_flist();
	vfs_close(vr_curdir);
	vr_curdir = vrp;
	gui_vfslist_setlist(win_browser, vfs_population(vr_curdir));

	if (vrn != NULL)
		gui_vfslist_setselected(win_browser, vrn, idx);

	return;
bad:
	gui_msgbar_warn(_("Unable to enter the parent directory."));
}

void
gui_browser_dir_parent(void)
{

	if (vr_curdir == NULL)
		return;
	
	if (locatestr != NULL) {
		/* First unset the filter if we have one */
		gui_browser_cleanup_flist();
		gui_vfslist_setlist(win_browser, vfs_population(vr_curdir));
		return;
	}

	gui_browser_gotofile(vr_curdir);
}

void
gui_browser_dir_enter(void)
{
	struct vfsref *vr;

	if (gui_vfslist_warn_isempty(win_browser))
		return;

	vr = vfs_dup(gui_vfslist_getselected(win_browser));
	if (vfs_populate(vr) != 0) {
		if (vfs_populatable(vr)) {
			/* Permission denied? */
			gui_msgbar_warn(_("Unable to enter the "
			    "selected directory."));
		}
		vfs_close(vr);
		return;
	}

	/* Change the directory */
	gui_browser_cleanup_flist();
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

int
gui_browser_searchnext(const struct vfsmatch *vm)
{
	return gui_vfslist_searchnext(win_browser, vm);
}

/**
 * @brief Change to a specified directory.
 */
static void
gui_browser_do_chdir(const char *path)
{
	int dir = 0;
	struct vfsref *vr;

	if (vr_curdir != NULL) {
		/* Relative to the current node */
		vr = vfs_lookup(path, NULL, vfs_filename(vr_curdir), 0);
	} else {
		/* Relative to the root */
		vr = vfs_lookup(path, NULL, NULL, 0);
	}

	if (vr != NULL) {
		if (vfs_populate(vr) == 0)
			dir = 1;
		else if (!vfs_playable(vr))
			goto bad;

		/* Replace old directory */
		gui_browser_cleanup_flist();
		if (vr_curdir != NULL)
			vfs_close(vr_curdir);
		vr_curdir = vr;

		if (dir) {
			/* Go inside the directory */
			gui_vfslist_setlist(win_browser,
			    vfs_population(vr));
		} else {
			/* Create a fake directory */
			vfs_list_insert_tail(&vl_flist, vfs_dup(vr));
			gui_vfslist_setlist(win_browser, &vl_flist);
		}

		return;
	}

bad:	gui_msgbar_warn(_("Unable to display the file or directory."));
	if (vr != NULL)
		vfs_close(vr);
}

void
gui_browser_chdir(void)
{
	char *path;
	const char *curwd = NULL;

	if (vr_curdir != NULL)
		curwd = vfs_filename(vr_curdir);

	path = gui_input_askstring(_("Change directory"), curwd, NULL);
	if (path == NULL)
		return;

	gui_browser_do_chdir(path);
	g_free(path);
}

void
gui_browser_setfocus(int focus)
{
	gui_vfslist_setfocus(win_browser, focus);
}

void
gui_browser_write_playlist(void)
{
	char *fn;
	struct vfsref *vr;

	fn = gui_input_askstring(_("Write playlist to file"), NULL, NULL);
	if (fn == NULL)
		return;
	playq_lock();
	vr = vfs_write_playlist(&playq_list, vr_curdir, fn);
	playq_unlock();
	g_free(fn);

	if (vr == NULL) {
		gui_msgbar_warn(_("Unable to write playlist."));
		return;
	}
	vfs_populate(vr);

	/* Replace old directory */
	gui_browser_cleanup_flist();
	if (vr_curdir != NULL)
		vfs_close(vr_curdir);
	vr_curdir = vr;

	/* Go inside the directory */
	gui_vfslist_setlist(win_browser, vfs_population(vr));
}

void
gui_browser_fullpath(void)
{
	gui_vfslist_fullpath(win_browser);
}

int
gui_browser_locate(const struct vfsmatch *vm)
{
	struct vfslist vl = VFSLIST_INITIALIZER;

	if (vr_curdir == NULL)
		return (-1);

	/* Perform a search on the query */
	vfs_locate(&vl, vr_curdir, vm);
	if (vfs_list_empty(&vl))
		return (-1);
	
	gui_browser_cleanup_flist();
	locatestr = g_strdup(vfs_match_value(vm));
	vfs_list_move(&vl_flist, &vl);
	gui_vfslist_setlist(win_browser, &vl_flist);

	return (0);
}

void
gui_browser_gotofolder(void)
{

	if (gui_vfslist_warn_isempty(win_browser))
		return;

	gui_browser_gotofile(gui_vfslist_getselected(win_browser));
}

void
gui_browser_gotohome(void)
{
	const char *defdir;

	defdir = config_getopt("gui.browser.defaultpath");
	if (defdir[0] == '\0')
		defdir = "~";
	gui_browser_do_chdir(defdir);
}
