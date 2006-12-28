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
 * @file gui_msgbar.c
 */

#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "gui_internal.h"
#include "trans.h"

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
	win_msgbar = newwin(1, 0, gui_size_msgbar_top, 0);
	wbkgdset(win_msgbar, COLOR_PAIR(GUI_COLOR_BAR));
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
	GUI_LOCK;
	wresize(win_msgbar, 1, COLS);
	mvwin(win_msgbar, gui_size_msgbar_top, 0);
	clearok(win_msgbar, TRUE);
	GUI_UNLOCK;

	gui_msgbar_refresh();
}

void
gui_msgbar_refresh(void)
{
	GUI_LOCK;
	if (win_msgbar != NULL) {
		werase(win_msgbar);
		mvwaddstr(win_msgbar, 0, 1, message->str);
		wrefresh(win_msgbar);
	}
	GUI_UNLOCK;
}

void
gui_msgbar_flush(void)
{
	GUI_LOCK;
	g_string_assign(message, "");
	message_prio = -1;
	curs_set(0);
	GUI_UNLOCK;
}

void
gui_msgbar_update(const char *msg, int prio, int cursor)
{
	GUI_LOCK;

	/*
	 * Do not attempt to set the message when there is a message on
	 * screen with a higher priority
	 */
	if (prio < message_prio) {
		GUI_UNLOCK;
		return;
	}

	g_string_assign(message, msg);
	message_prio = prio;
	GUI_UNLOCK;

	curs_set(cursor);
	gui_msgbar_refresh();
}
