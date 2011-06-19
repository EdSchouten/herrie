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
 * @file gui_msgbar.c
 * @brief Message logging for textual user interface.
 */

#include "stdinc.h"

#include "gui.h"
#include "gui_internal.h"

/**
 * @brief Priority of the current message in the message bar.
 */
static int message_prio = 0;
/**
 * @brief Current message in the message bar.
 */
static GString *message;
/**
 * @brief Window object of the message bar at the bottom of the screen.
 */
static WINDOW *win_msgbar;

void
gui_msgbar_init(void)
{
	win_msgbar = newwin(1, 0, GUI_SIZE_MSGBAR_TOP, 0);
	clearok(win_msgbar, TRUE);
	if (gui_draw_colors)
		wbkgdset(win_msgbar, COLOR_PAIR(GUI_COLOR_BAR));
	else
		wbkgdset(win_msgbar, A_REVERSE);

	message = g_string_sized_new(32);

	/* Hide the cursor by default */
	curs_set(0);

	gui_msgbar_refresh();
}

void
gui_msgbar_destroy(void)
{
	g_string_free(message, TRUE);
	delwin(win_msgbar);
	win_msgbar = NULL;
}

void
gui_msgbar_resize(void)
{
	gui_lock();
	wresize(win_msgbar, 1, COLS);
	mvwin(win_msgbar, GUI_SIZE_MSGBAR_TOP, 0);
	clearok(win_msgbar, TRUE);
	gui_unlock();

	gui_msgbar_refresh();
}

void
gui_msgbar_refresh(void)
{
	gui_lock();
	if (win_msgbar != NULL) {
		werase(win_msgbar);
		mvwaddstr(win_msgbar, 0, 1, message->str);
		wnoutrefresh(win_msgbar);
	}
	gui_unlock();
}

void
gui_msgbar_flush(void)
{
	gui_lock();
	g_string_assign(message, "");
	message_prio = -1;
	curs_set(0);
	gui_unlock();

	gui_msgbar_refresh();
}

/**
 * @brief Update the text in the message bar.
 */
static void
gui_msgbar_update(const char *msg, int prio, int cursor)
{
	gui_lock();

	/*
	 * Do not attempt to set the message when there is a message on
	 * screen with a higher priority
	 */
	if (prio < message_prio) {
		gui_unlock();
		return;
	}

	g_string_assign(message, msg);
	message_prio = prio;
	gui_unlock();

	curs_set(cursor);
	gui_msgbar_refresh();
	gui_draw_done();
}

void
gui_msgbar_warn(const char *msg)
{
	gui_msgbar_update(msg, 0, 0);
}

void
gui_msgbar_ask(const char *msg)
{
	gui_msgbar_update(msg, 1, 1);
}
