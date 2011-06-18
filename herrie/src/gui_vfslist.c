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
 * @file gui_vfslist.c
 * @brief Generic directory/playlist display for textual user interface.
 */

#include "stdinc.h"

#include "config.h"
#include "gui.h"
#include "gui_internal.h"
#include "gui_vfslist.h"
#include "vfs.h"

/**
 * @brief Whether or not we want to scroll entire pages instead of
 * single lines when the selection gets out of sight.
 */
static int scrollpages;

/**
 * @brief Make sure the cursor resides inside the viewport. Move the
 *        viewport if not.
 */
static void
gui_vfslist_cursor_adjust(struct gui_vfslist *gv)
{
	struct vfsref *vr;

	/* Shortcut - nothing to adjust when the list is empty */
	if (gv->vr_selected == NULL)
		return;

	/* Three possible cases here */
	if (gv->idx_top > gv->idx_selected) {
		/*
		 * The entry is above the viewport. Move the viewport up
		 * so we can see it.
		 */
		gv->vr_top = gv->vr_selected;
		gv->idx_top = gv->idx_selected;
	} else if (gv->idx_top + gv->winheight <= gv->idx_selected) {
		/*
		 * The entry is below the viewport. Start counting
		 * backward from the selected entry, so we can keep it
		 * just in screen. This is faster than counting forward,
		 * because there could be a lot of items in between.
		 */
		gv->vr_top = gv->vr_selected;
		gv->idx_top = gv->idx_selected;
		do {
			vr = vfs_list_prev(gv->vr_top);
			if (vr == NULL)
				break;
			gv->vr_top = vr;
			gv->idx_top--;
		} while (gv->idx_top + gv->winheight - 1 > gv->idx_selected);
	} else {
		/*
		 * The item is in reach, but it could be possible that
		 * there is some blank space at the bottom. Move the
		 * viewport up, so we fill the terminal as much as
		 * possible.
		 */
		while (gv->idx_top + gv->winheight - 1 > vfs_list_items(gv->list)) {
			vr = vfs_list_prev(gv->vr_top);
			if (vr == NULL)
				break;
			gv->vr_top = vr;
			gv->idx_top--;
		}
	}
}

/**
 * @brief Calculate what width the column with the index numbers should
 *        have, based on the amount of songs in the playlist.
 */
static unsigned int
gui_vfslist_idxcol_width(struct gui_vfslist *gv)
{
	unsigned int len, idx;

	if (gv->list == NULL)
		return (0);

	/*
	 * Calculate the number of digits needed to print the number of
	 * entries
	 */
	for (len = 1, idx = vfs_list_items(gv->list);idx >= 10;
	    len++, idx /= 10);

	return (len);
}

/**
 * @brief Recalculate the percentage string.
 */
static void
gui_vfslist_percent(struct gui_vfslist *gv)
{
	unsigned int bottom, length;

	length = gv->list != NULL ? vfs_list_items(gv->list) : 0;
	bottom = gv->idx_top + gv->winheight - 1;

	if (bottom >= length) {
		/* We can see the end */
		if (gv->idx_top <= 1)
			strcpy(gv->percent, " (all) ");
		else
			strcpy(gv->percent, " (end) ");
	} else {
		/* Write back a percentage */
		sprintf(gv->percent, " (%d%%) ", (bottom * 100) / length);
	}
}

/**
 * @brief Redraw the contents of the dialog.
 */
static void
gui_vfslist_refresh(struct gui_vfslist *gv)
{
	unsigned int i, idx, idxw, idxmaxw = 0;
	char num[16], mark;
	struct vfsref *vr;

	if (gv->win == NULL)
		return;

	/* Make sure we have everything in sight */
	gui_vfslist_cursor_adjust(gv);

	gui_lock();
	werase(gv->win);

	vr = gv->vr_top;
	idx = gv->idx_top;
	if (gv->shownumbers)
		idxmaxw = gui_vfslist_idxcol_width(gv);
	for (i = 0; i < gv->winheight; i++) {
		if (vr == NULL) {
			/* We've reached the bottom of the list */
			mvwaddch(gv->win, i, 0, ' ');
			wbkgdset(gv->win, COLOR_PAIR(GUI_COLOR_BLOCK));
			wclrtobot(gv->win);
			break;
		}

		/* Sanity check */
		g_assert((vr == gv->vr_selected) == (idx == gv->idx_selected));

		if (vr == gv->vr_selected && gv->winfocused)
			/* Selected */
			wbkgdset(gv->win, COLOR_PAIR(GUI_COLOR_SELECT));
		else if (vfs_marked(vr))
			/* Marked */
			wbkgdset(gv->win, COLOR_PAIR(GUI_COLOR_MARKED));
		else if (vr == gv->vr_selected)
			/* Selected, but inactive */
			wbkgdset(gv->win, COLOR_PAIR(GUI_COLOR_DESELECT));

		/* Small whitespace on the left, or > when black & white */
		if ((vr == gv->vr_selected) && !gui_draw_colors) {
			if (gv->winfocused)
				wattron(gv->win, A_BOLD);
			mvwaddch(gv->win, i, 0, '>');
		} else {
			mvwaddch(gv->win, i, 0, ' ');
		}
		wclrtoeol(gv->win);

		if (gv->shownumbers) {
			idxw = snprintf(num, sizeof num, "%d", idx);
			mvwaddstr(gv->win, i, 1 + idxmaxw - idxw, num);
			waddstr(gv->win, ". ");
			waddstr(gv->win, vfs_name(vr));
		} else {
			waddstr(gv->win, vfs_name(vr));
		}

                /* Marking character for dirs and such */
		mark = vfs_marking(vr);
		if (mark != '\0')
			waddch(gv->win, mark);

		wbkgdset(gv->win, COLOR_PAIR(GUI_COLOR_BLOCK));
		wattroff(gv->win, A_BOLD);

		vr = vfs_list_next(vr);
		idx++;
	}

	wnoutrefresh(gv->win);
	gui_unlock();

	gui_vfslist_percent(gv);

	if (gv->callback != NULL)
		gv->callback();
}

struct gui_vfslist *
gui_vfslist_new(int shownumbers)
{
	struct gui_vfslist *ret;

	scrollpages = config_getopt_bool("gui.vfslist.scrollpages");
	
	ret = g_slice_new0(struct gui_vfslist);
	ret->shownumbers = shownumbers;

	return (ret);
}

void
gui_vfslist_destroy(struct gui_vfslist *gv)
{
	if (gv->win != NULL)
		delwin(gv->win);
	g_slice_free(struct gui_vfslist, gv);
}

void
gui_vfslist_setlist(struct gui_vfslist *gv, const struct vfslist *vl)
{
	gv->list = vl;
	gv->vr_top = gv->vr_selected = vfs_list_first(gv->list);
	gv->idx_top = gv->idx_selected = (gv->vr_selected != NULL) ? 1 : 0;

	gui_vfslist_refresh(gv);
}

int
gui_vfslist_warn_isempty(struct gui_vfslist *gv)
{
	if (gv->vr_selected == NULL) {
		gui_msgbar_warn(_("There are no songs."));
		return (1);
	} else {
		return (0);
	}
}

void
gui_vfslist_move(struct gui_vfslist *gv,
    int x, int y, int width, int height)
{
	gui_lock();
	if (gv->win == NULL) {
		gv->win = newwin(height, width, y, x);
	} else {
		wresize(gv->win, height, width);
		mvwin(gv->win, y, x);
	}
	clearok(gv->win, TRUE);
	gui_unlock();
	gv->winheight = height;

	gui_vfslist_refresh(gv);
}

void
gui_vfslist_setfocus(struct gui_vfslist *gv, int focus)
{
	gv->winfocused = focus;
	gui_vfslist_refresh(gv);
}

void
gui_vfslist_setselected(struct gui_vfslist *gv, struct vfsref *vr,
    unsigned int index)
{
	unsigned int i;

	gv->vr_selected = gv->vr_top = vr;
	gv->idx_selected = gv->idx_top =  index;

	/* Put the selected item in the center */
	for (i = 0; i < (gv->winheight - 1) / 2 &&
	    vfs_list_prev(gv->vr_top) != NULL; i++) {
		gv->vr_top = vfs_list_prev(gv->vr_top);
		gv->idx_top--;
	}

	gui_vfslist_refresh(gv);
}

void
gui_vfslist_cursor_up(struct gui_vfslist *gv)
{
	struct vfsref *vr;

	if (gui_vfslist_warn_isempty(gv))
		return;

	vr = vfs_list_prev(gv->vr_selected);
	if (vr != NULL) {
		gv->vr_selected = vr;
		gv->idx_selected--;

		if (scrollpages && gv->idx_top > gv->idx_selected) {
			gv->vr_top = vfs_list_first(gv->list);
			gv->idx_top = 1;
		}

		gui_vfslist_refresh(gv);
	} else {
		gui_msgbar_warn(_("You are at the first song."));
	}
}

void
gui_vfslist_cursor_down(struct gui_vfslist *gv, int silent)
{
	struct vfsref *vr;
	
	if (gui_vfslist_warn_isempty(gv))
		return;

	vr = vfs_list_next(gv->vr_selected);
	if (vr != NULL) {
		gv->vr_selected = vr;
		gv->idx_selected++;

		g_assert(gv->idx_top + gv->winheight >= gv->idx_selected);
		if (scrollpages && gv->idx_top + gv->winheight == gv->idx_selected) {
			gv->vr_top = gv->vr_selected;
			gv->idx_top = gv->idx_selected;
		}

		gui_vfslist_refresh(gv);
	} else if (!silent) {
		gui_msgbar_warn(_("You are at the last song."));
	}
}

void
gui_vfslist_cursor_head(struct gui_vfslist *gv)
{
	if (gui_vfslist_warn_isempty(gv))
		return;

	gv->vr_top = gv->vr_selected = vfs_list_first(gv->list);
	gv->idx_top = gv->idx_selected = (gv->vr_selected != NULL) ? 1 : 0;

	gui_vfslist_refresh(gv);
}

void
gui_vfslist_cursor_tail(struct gui_vfslist *gv)
{
	if (gui_vfslist_warn_isempty(gv))
		return;

	gv->vr_selected = vfs_list_last(gv->list);
	gv->idx_selected = vfs_list_items(gv->list);

	gui_vfslist_refresh(gv);
}

void
gui_vfslist_cursor_pageup(struct gui_vfslist *gv)
{
	unsigned int i;

	if (gui_vfslist_warn_isempty(gv))
		return;

	/* Cursor = bottom of the screen */
	gv->vr_selected = vfs_list_next(gv->vr_top);
	gv->idx_selected = gv->idx_top + 1;

	/* Adjust the viewport */
	for (i = 2; i < gv->winheight && gv->vr_top != NULL; i++) {
		gv->vr_top = vfs_list_prev(gv->vr_top);
		gv->idx_top--;
	}
	if (gv->vr_top == NULL) {
		/* Don't scroll out of reach */
		gv->vr_selected = gv->vr_top = vfs_list_first(gv->list);
		gv->idx_selected = gv->idx_top = 1;
	} else if (gv->vr_selected == NULL) {
		/* Original screen may have had one item */
		gv->vr_selected = gv->vr_top;
		gv->idx_selected = gv->idx_top;
	}
	
	gui_vfslist_refresh(gv);
}

void
gui_vfslist_cursor_pagedown(struct gui_vfslist *gv)
{
	unsigned int i;
	struct vfsref *vr_toptmp = gv->vr_top;

	if (gui_vfslist_warn_isempty(gv))
		return;

	/* A la vi: page down minus two */
	for (i = 2; i < gv->winheight && gv->vr_top != NULL; i++) {
		gv->vr_top = vfs_list_next(gv->vr_top);
		gv->idx_top++;
	}
	
	if (gv->vr_top == NULL) {
		gv->vr_top = vfs_list_last(gv->list);
		gv->idx_top = vfs_list_items(gv->list);
	}
	gv->vr_selected = gv->vr_top;
	gv->idx_selected = gv->idx_top;

	gui_vfslist_cursor_adjust(gv);
	if (vr_toptmp == gv->vr_top) {
		/*
		 * This action had no effect. We've reached the bottom,
		 * so move the cursor to the bottom then.
		 */
		gui_vfslist_cursor_tail(gv);
	} else {
		gui_vfslist_refresh(gv);
	}
}

/**
 * @brief Adjust our pointer to a VFS reference so it doesn't point to
 *        an object that is about to get removed.
 */
static void
gui_vfslist_adjust_pre_removal(struct vfsref **vr, unsigned int *curidx,
    unsigned int newidx, int follow)
{
	struct vfsref *vr_new;
	g_assert((*vr != NULL) && (*curidx > 0));

	vr_new = vfs_list_next(*vr);
	if (*curidx > newidx) {
		/* Item got removed above us */
		if (follow || vr_new == NULL) {
			/* Follow our selection */
			(*curidx)--;
		} else {
			/* Keep the current offset */
			*vr = vr_new;
		}
	} else if (*curidx == newidx) {
		/* Current item gets removed */
		if (vr_new != NULL) {
			/* Stay at the same position */
			*vr = vr_new;
		} else {
			/* Cannot go down - go up */
			*vr = vfs_list_prev(*vr);
			(*curidx)--;
		}
	}

	g_assert((*vr == NULL) == (*curidx == 0));
}

void
gui_vfslist_notify_pre_removal(struct gui_vfslist *gv, unsigned int index)
{
	gui_vfslist_adjust_pre_removal(&gv->vr_top, &gv->idx_top, index, 0);
	gui_vfslist_adjust_pre_removal(&gv->vr_selected, &gv->idx_selected,
	    index, 1);
}

/**
 * @brief Adjust a reference with index to an entry in the playlist,
 *        right after a new entry in the playlist gets inserted.
 */
static void
gui_vfslist_adjust_post_insertion(struct gui_vfslist *gv, struct vfsref **vr,
    unsigned int *curidx, unsigned int newidx, int follow)
{
	g_assert((*vr == NULL) == (*curidx == 0));

	if (*curidx == 0) {
		/* List was empty - we take the first slot */
		*vr = vfs_list_first(gv->list);
		*curidx = 1;
	} else if (*curidx >= newidx) {
		/* It happened above us */
		if (follow) {
			/* Follow our selection */
			(*curidx)++;
		} else {
			/* Keep the current offset */
			*vr = vfs_list_prev(*vr);
		}
	}

	g_assert((*vr != NULL) && (*curidx > 0));
}

void
gui_vfslist_notify_post_insertion(struct gui_vfslist *gv, unsigned int index)
{
	gui_vfslist_adjust_post_insertion(gv, &gv->vr_top, &gv->idx_top,
	    index, 0);
	gui_vfslist_adjust_post_insertion(gv, &gv->vr_selected,
	    &gv->idx_selected, index, 1);
}

void
gui_vfslist_notify_post_randomization(struct gui_vfslist *gv)
{
	unsigned int idx;

	/*
	 * After a randomization the indices do not match the
	 * corresponding objects. We could trace the objects and update
	 * the indices, but we might as well better keep the current
	 * indices and update the objects. This is more convenient for
	 * the user.
	 */
	if (gv->idx_top > 0) {
		/* Top position */
		for (idx = 1, gv->vr_top = vfs_list_first(gv->list);
		    idx < gv->idx_top;
		    idx++, gv->vr_top = vfs_list_next(gv->vr_top));
		/* Selection - just continue previous traversal */
		for (gv->vr_selected = gv->vr_top;
		    idx < gv->idx_selected;
		    idx++, gv->vr_selected = vfs_list_next(gv->vr_selected));
	}
}

void
gui_vfslist_notify_done(struct gui_vfslist *gv)
{
	gui_vfslist_refresh(gv);
}

int
gui_vfslist_searchnext(struct gui_vfslist *gv, const struct vfsmatch *vm)
{
	struct vfsref *vr;
	unsigned int idx;

	g_assert(vm != NULL);

	if (gv->vr_selected == NULL)
		return (-1);

	/* Step 1: search from selection to end */
	for (vr = vfs_list_next(gv->vr_selected), idx = gv->idx_selected + 1;
	    vr != NULL; vr = vfs_list_next(vr), idx++) {
		if (vfs_match_compare(vm, vfs_name(vr))) {
			gui_msgbar_flush();
			goto found;
		}
	}

	/* Step 2: search from beginning to selection */
	for (vr = vfs_list_first(gv->list), idx = 1;
	    vr != vfs_list_next(gv->vr_selected);
	    vr = vfs_list_next(vr), idx++) {
		if (vfs_match_compare(vm, vfs_name(vr))) {
			gui_msgbar_warn(_("Search wrapped to top."));
			goto found;
		}
	}

	return (-1);

found:	/* Select our found item */
	gv->vr_selected = vr;
	gv->idx_selected = idx;

	/* No need to redraw, as window switching is performed */
	return (0);
}

void
gui_vfslist_fullpath(struct gui_vfslist *gv)
{
	char *msg;

	if (gui_vfslist_warn_isempty(gv))
		return;
	
	msg = g_strdup_printf("%s: %s",
	    _("Full pathname"), vfs_filename(gv->vr_selected));
	gui_msgbar_warn(msg);
	g_free(msg);
}
