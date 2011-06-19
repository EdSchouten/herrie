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
 * @file gui_vfslist.h
 * @brief Generic directory/playlist display for textual user interface.
 */

#include CURSES_HEADER

struct vfslist;
struct vfsref;
struct vfsmatch;

/**
 * @brief Graphical presentation of a vfslist.
 */
struct gui_vfslist {
	/**
	 * @brief Curses window used to draw entries.
	 */
	WINDOW		*win;
	/**
	 * @brief Height of the Curses window.
	 */
	unsigned int	winheight;
	/**
	 * @brief Focus state of window to determine selection color.
	 */
	unsigned int	winfocused;
	/**
	 * @brief Show index numbers before each entry.
	 */
	int		shownumbers;

	/**
	 * @brief List that should be used for display.
	 */
	const struct vfslist *list;
	/**
	 * @brief Item currently at the top of the screen.
	 */
	struct vfsref	*vr_top;
	/**
	 * @brief Index number of vr_top.
	 */
	unsigned int	idx_top;
	/**
	 * @brief Item selected with the cursor.
	 */
	struct vfsref	*vr_selected;
	/**
	 * @brief Index number of vr_selected.
	 */
	unsigned int	idx_selected;

	/**
	 * @brief String containing a percentual notation of the
	 *        progress in the list.
	 */
	char		percent[8];

	/**
	 * @brief Callback function that should be run after a refresh.
	 */
	void		(*callback)(void);
};

/**
 * @brief Allocate and initialize a new gui_vfslist.
 */
struct gui_vfslist *gui_vfslist_new(int shownumbers);
/**
 * @brief Deallocate a gui_vfslist.
 */
void gui_vfslist_destroy(struct gui_vfslist *gv);
/**
 * @brief Set the list that should be shown in the dialog.
 */
void gui_vfslist_setlist(struct gui_vfslist *gv, const struct vfslist *vl);
/**
 * @brief Warn the user when the list is empty.
 */
int gui_vfslist_warn_isempty(struct gui_vfslist *gv);
/**
 * @brief Move the dialog to a specified position in the terminal.
 */
void gui_vfslist_move(struct gui_vfslist *gv, int x, int y,
    int width, int height);
/**
 * @brief Focus or unfocus the dialog. This causes the selected item to
 *        change its color.
 */
void gui_vfslist_setfocus(struct gui_vfslist *gv, int focus);
/**
 * @brief Change which item is currently selected.
 */
void gui_vfslist_setselected(struct gui_vfslist *gv, struct vfsref *vr,
    unsigned int index);

/**
 * @brief Read which item is currently selected.
 */
static inline struct vfsref *
gui_vfslist_getselected(struct gui_vfslist *gv)
{
	return (gv->vr_selected);
}

/**
 * @brief Read the index number of the currently selected item.
 */
static inline unsigned int
gui_vfslist_getselectedidx(struct gui_vfslist *gv)
{
	return (gv->idx_selected);
}

/**
 * @brief Move the cursor one item up.
 */
void gui_vfslist_cursor_up(struct gui_vfslist *gv);
/**
 * @brief Move the cursor one item down.
 */
void gui_vfslist_cursor_down(struct gui_vfslist *gv, int silent);
/**
 * @brief Move the cursor to the top of the list.
 */
void gui_vfslist_cursor_head(struct gui_vfslist *gv);
/**
 * @brief Move the cursor to the bottom of the list.
 */
void gui_vfslist_cursor_tail(struct gui_vfslist *gv);
/**
 * @brief Move the cursor one page up.
 */
void gui_vfslist_cursor_pageup(struct gui_vfslist *gv);
/**
 * @brief Move the cursor one page down.
 */
void gui_vfslist_cursor_pagedown(struct gui_vfslist *gv);

/**
 * @brief Return the percentage string.
 */
static inline const char *
gui_vfslist_getpercentage(struct gui_vfslist *gv)
{
	return (gv->percent);
}

/**
 * @brief Set a callback that is run each time the dialog is refreshed.
 */
static inline void
gui_vfslist_setcallback(struct gui_vfslist *gv, void (*func)(void))
{
	gv->callback = func;
}

/**
 * @brief Notify the gui_vfslist that an item is about to be removed
 *        from the list.
 */
void gui_vfslist_notify_pre_removal(struct gui_vfslist *gv,
    unsigned int index);
/**
 * @brief Notify the gui_vfslist that an item has been inserted to the
 *        list.
 */
void gui_vfslist_notify_post_insertion(struct gui_vfslist *gv,
    unsigned int index);
/**
 * @brief Notify the gui_vfslist that the list has been randomized.
 */
void gui_vfslist_notify_post_randomization(struct gui_vfslist *gv);
/**
 * @brief Notify the gui_vfslist that all changes to the list itself are
 *        finished. This causes the list to be redrawn.
 */
void gui_vfslist_notify_done(struct gui_vfslist *gv);
/**
 * @brief Change the selection to the first item that matches the
 *        globally defined search string gui_input_cursearch.
 */
int gui_vfslist_searchnext(struct gui_vfslist *gv, const struct vfsmatch *vm);
/**
 * @brief Show the full pathname of the selected entry in the message
 *        bar.
 */
void gui_vfslist_fullpath(struct gui_vfslist *gv);
