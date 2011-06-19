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
 * @file gui_internal.h
 * @brief Internal textual user interface routines.
 */

#include CURSES_HEADER

struct vfsref;
struct vfsmatch;

/**
 * @brief Determine whether our terminal is black or white or color.
 */
extern int gui_draw_colors;
/**
 * @brief Height percentage of the playlist.
 */
extern int gui_draw_ratio;
/**
 * @brief Refresh the curses GUI after a terminal resize.
 */
void gui_draw_resize(void);
/**
 * @brief Write all altered data back to the physical terminal.
 */
void gui_draw_done(void);
/**
 * @brief The mutex that locks the GUI down.
 */
extern GMutex *gui_mtx;

/**
 * @brief Acquire a lock on the GUI.
 */
static inline void
gui_lock(void)
{
	g_mutex_lock(gui_mtx);
}

/**
 * @brief Release a lock on the GUI.
 */
static inline void
gui_unlock(void)
{
	g_mutex_unlock(gui_mtx);
}

/* Display size ratios */
/**
 * @brief Height of playlist window.
 */
#define GUI_SIZE_PLAYQ_HEIGHT		(((LINES * gui_draw_ratio) / 100) - 1)
/**
 * @brief Offset of the window containing the filebrowser's directory
 *        name.
 */
#define GUI_SIZE_BROWSER_DIRNAME_TOP	(GUI_SIZE_PLAYQ_HEIGHT + 1)
/**
 * @brief Offset of the filebrowser window.
 */
#define GUI_SIZE_BROWSER_TOP		(GUI_SIZE_BROWSER_DIRNAME_TOP + 1)
/**
 * @brief Height of the filebrowser window.
 */
#define GUI_SIZE_BROWSER_HEIGHT		(LINES - GUI_SIZE_BROWSER_TOP - 1)
/**
 * @brief Offset of the message bar window.
 */
#define GUI_SIZE_MSGBAR_TOP		(LINES - 1)

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
 * @brief Curses colour code used to draw marked items (song currenty
 *        played).
 */
#define GUI_COLOR_MARKED		6

/**
 * @brief Display a standard Yes/No question at the bottom of the screen
 *        and return the user response.
 */
int gui_input_askyesno(const char *question);
/**
 * @brief Display a string input question at the bottom of the screen
 *        and return the user response.
 */
char *gui_input_askstring(const char *question, const char *defstr,
    int (*validator)(const char *str, char c));

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
 * @brief Flush the text in the message bar.
 */
void gui_msgbar_flush(void);
/**
 * @brief Show a message in the message bar that will only be
 *        overwritten by other messages with this priority. This
 *        routine will also unhide the cursor and show it right after
 *        the message.
 */
void gui_msgbar_ask(const char *msg);

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
void gui_playq_cursor_head(void);
/**
 * @brief Move the cursor and the viewport of the playlist to the bottom
 *        of the playlist.
 */
void gui_playq_cursor_tail(void);
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
void gui_playq_song_move_up(void);
/**
 * @brief Move the currently selected song one down.
 */
void gui_playq_song_move_down(void);
/**
 * @brief Move the currently selected song to the top of the playlist.
 */
void gui_playq_song_move_head(void);
/**
 * @brief Move the currently selected song to the bottom of the
 *        playlist.
 */
void gui_playq_song_move_tail(void);
/**
 * @brief Start playback on the currently selected song.
 */
void gui_playq_song_select(void);
/**
 * @brief Search for the next item matching gui_input_cursearch in the
 *        playlist.
 */
int gui_playq_searchnext(const struct vfsmatch *vm);
/**
 * @brief Focus or unfocus the playlist.
 */
void gui_playq_setfocus(int focus);
/**
 * @brief Write the full pathname of the selected item in the message
 *        bar.
 */
void gui_playq_fullpath(void);
#ifdef BUILD_VOLUME
/**
 * @brief Increment the volume and display the new value.
 */
void gui_playq_volume_up(void);
/**
 * @brief Decrement the volume and display the new value.
 */
void gui_playq_volume_down(void);
#endif /* BUILD_VOLUME */
/**
 * @brief Go to the directory containing the selected item.
 */
void gui_playq_gotofolder(void);
/**
 * @brief Go to the current user's home directory.
 */
void gui_browser_gotohome(void);
/**
 * @brief Go to the parent directory of the VFS reference and select the
 *        item which shares the same filename.
 */
void gui_browser_gotofile(struct vfsref *vr);

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
void gui_browser_cursor_head(void);
/**
 * @brief Move the cursor and the viewport of the filebrowser to the
 *        bottom of the current directory.
 */
void gui_browser_cursor_tail(void);
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
int gui_browser_searchnext(const struct vfsmatch *vm);
/**
 * @brief Change to a directory, filled in by the user.
 */
void gui_browser_chdir(void);
/**
 * @brief Focus or unfocus the filebrowser.
 */
void gui_browser_setfocus(int focus);
/**
 * @brief Write the playlist to a file.
 */
void gui_browser_write_playlist(void);
/**
 * @brief Write the full pathname of the selected item in the message
 *        bar.
 */
void gui_browser_fullpath(void);
/**
 * @brief Apply a recursive search filter on the current directory.
 */
int gui_browser_locate(const struct vfsmatch *vm);
/**
 * @brief Go to the directory containing the selected item.
 */
void gui_browser_gotofolder(void);
