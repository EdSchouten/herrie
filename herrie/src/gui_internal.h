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
 * @file gui_internal.h
 */

#ifndef _GUI_INTERNAL_H_
#define _GUI_INTERNAL_H_

#ifdef BUILD_XCURSES
#include <xcurses.h>
#else /* !BUILD_XCURSES */
#include <curses.h>
#endif /* BUILD_XCURSES */

#include "audio.h"
#include "gui.h"
#include "vfs.h"

/**
 * @brief Refresh the curses GUI after a terminal resize.
 */
void gui_draw_resize(void);
/**
 * @brief The mutex that locks the GUI down.
 */
extern GMutex *gui_lock;
/**
 * @brief Acquire a lock on the GUI.
 */
#define GUI_LOCK g_mutex_lock(gui_lock)
/**
 * @brief Release a lock on the GUI.
 */
#define GUI_UNLOCK g_mutex_unlock(gui_lock)

/* Display size ratios */
/**
 * @brief Height of playlist window.
 */
#define gui_size_playq_height		((LINES / 2) - 1)
/**
 * @brief Offset of the window containing the filebrowser's directory
 *        name.
 */
#define gui_size_browser_dirname_top	(gui_size_playq_height + 1)
/**
 * @brief Offset of the filebrowser window.
 */
#define gui_size_browser_top		(gui_size_browser_dirname_top + 1)
/**
 * @brief Height of the filebrowser window.
 */
#define gui_size_browser_height		(LINES - gui_size_browser_top - 1)
/**
 * @brief Offset of the message bar window.
 */
#define gui_size_msgbar_top		(LINES - 1)

/**
 * @brief Curses colour code used to draw bars (status, dirname).
 */
#define GUI_COLOR_BAR			2
/**
 * @brief Curses colour code used to draw blocks (playlist, browser).
 */
#define GUI_COLOR_BLOCK			3
/**
 * @brief Curses colour code used to draw focused selections.
 */
#define GUI_COLOR_SELECT		4
/**
 * @brief Curses colour code used to draw unfocused selections.
 */
#define GUI_COLOR_DESELECT		5

/**
 * @brief Display a standard Yes/No question at the bottom of the screen
 *        and return the user response.
 */
int gui_input_askyesno(const char *question);
/**
 * @brief Display a string input question at the bottom of the screen
 *        and return the user response.
 */
char *gui_input_askstring(char *question, char *defstr);
/**
 * @brief Indicator of the current search string.
 */
extern char *gui_input_cursearch;

/*
 *  gui_msgbar
 */
/**
 * @brief Create a bar at the bottom of the terminal displaying
 * 	  messages and questions.
 */
void gui_msgbar_init(void);
/**
 * @brief Destroy the message bar at the bottom of the screen.
 */
void gui_msgbar_destroy(void);
/**
 * @brief Set the proper size of the message bar after a terminal
 *        resize.
 */
void gui_msgbar_resize(void);
/**
 * @brief Redraw the message bar after its contents have been altered.
 */
void gui_msgbar_refresh(void);
/**
 * @brief Restore the cursor position after another thread updated the GUI.
 */
#define gui_draw_async_done() gui_msgbar_refresh()
/**
 * @brief Flush the text in the message bar.
 */
void gui_msgbar_flush(void);

/*
 * gui_playq
 */
/**
 * @brief Initialize the playlist window.
 */
void gui_playq_init(void);
/**
 * @brief Destroy the playlist window.
 */
void gui_playq_destroy(void);
/**
 * @brief Redraw the playlist window, because of a terminal resize.
 */
void gui_playq_resize(void);

/**
 * @brief Move the cursor of the playlist one item up.
 */
void gui_playq_cursor_up(void);
/**
 * @brief Move the cursor of the playlist one item down.
 */
void gui_playq_cursor_down(void);
/**
 * @brief Move the cursor and the viewport of the playlist one page up.
 */
void gui_playq_cursor_pageup(void);
/**
 * @brief Move the cursor and the viewport of the playlist one page down.
 */
void gui_playq_cursor_pagedown(void);
/**
 * @brief Move the cursor and the viewport of the playlist to the top of
 *        the playlist.
 */
void gui_playq_cursor_top(void);
/**
 * @brief Move the cursor and the viewport of the playlist to the bottom
 *        of the playlist.
 */
void gui_playq_cursor_bottom(void);
/**
 * @brief Remove the currently selected song from the playlist.
 */
void gui_playq_song_remove(void);
/**
 * @brief Remove all songs from the playlist after presenting the user
 *        with a question.
 */
void gui_playq_song_remove_all(void);
/**
 * @brief Randomize all songs in the playlist after presenting the user
 *        with a question.
 */
void gui_playq_song_randomize(void);

/**
 * @brief Add a song before the selected item in the playlist.
 */
void gui_playq_song_add_before(struct vfsref *ve);
/**
 * @brief Add a song after the selected item in the playlist.
 */
void gui_playq_song_add_after(struct vfsref *ve);
/**
 * @brief Move the currently selected song one up.
 */
void gui_playq_song_moveup(void);
/**
 * @brief Move the currently selected song one down.
 */
void gui_playq_song_movedown(void);
/**
 * @brief Search for the next item matching gui_input_cursearch in the
 *        playlist.
 */
void gui_playq_searchnext(void);
/**
 * @brief Focus or unfocus the playlist.
 */
void gui_playq_setfocus(int focus);

/*
 * gui_browser
 */
/**
 * @brief Initialize the filebrowser window.
 */
void gui_browser_init(void);
/**
 * @brief Destroy the filebrowser window.
 */
void gui_browser_destroy(void);
/**
 * @brief Redraw the filebrowser, because of a terminal resize.
 */
void gui_browser_resize(void);

/**
 * @brief Move the cursor of the filebrowser one position up.
 */
void gui_browser_cursor_up(void);
/**
 * @brief Move the cursor of the filebrowser one position down.
 */
void gui_browser_cursor_down(void);
/**
 * @brief Move the cursor and the viewport of the filebrowser one page
 *        up.
 */
void gui_browser_cursor_pageup(void);
/**
 * @brief Move the cursor and the viewport of the filebrowser one page
 *        down.
 */
void gui_browser_cursor_pagedown(void);
/**
 * @brief Move the cursor and the viewport of the filebrowser to the top
 *        of the current directory.
 */
void gui_browser_cursor_top(void);
/**
 * @brief Move the cursor and the viewport of the filebrowser to the
 *        bottom of the current directory.
 */
void gui_browser_cursor_bottom(void);
/**
 * @brief Change the current working directory of the filebrowser to the
 *        parent directory.
 */
void gui_browser_dir_parent(void);
/**
 * @brief Change the current working directory of the filebrowser to the
 *        currently selected item.
 */
void gui_browser_dir_enter(void);
/**
 * @brief Add the current selection to the tail of the playlist.
 */
void gui_browser_playq_add_tail(void);
/**
 * @brief Add the current selection to the head of the playlist.
 */
void gui_browser_playq_add_head(void);
/**
 * @brief Add the current selection after the selected item in the
 *        playlist.
 */
void gui_browser_playq_add_after(void);
/**
 * @brief Add the current selection before the selected item in the
 *        playlist.
 */
void gui_browser_playq_add_before(void);
/**
 * @brief Search for the next item matching gui_input_cursearch in the
 *        file browser.
 */
void gui_browser_searchnext(void);
/**
 * @brief Change to a specified directory.
 */
void gui_browser_chdir(void);
/**
 * @brief Focus or unfocus the filebrowser.
 */
void gui_browser_setfocus(int focus);

#endif /* !_GUI_INTERNAL_H_ */
