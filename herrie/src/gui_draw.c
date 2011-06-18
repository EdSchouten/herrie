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
 * @file gui_draw.c
 * @brief Initialization, destruction and rendering functions for interface.
 */

#include "stdinc.h"

#include "config.h"
#include "gui.h"
#include "gui_internal.h"

GMutex *gui_mtx;
int gui_draw_colors;
int gui_draw_ratio;

void
gui_draw_init_pre(void)
{
	/* We need the termcap before chroot() */
	initscr();
}

void
gui_draw_init_post(void)
{
	/* Lock playq from main thread */
	gui_mtx = g_mutex_new();

#ifdef PDCURSES
	PDC_set_title(APP_NAME);
#endif /* PDCURSES */
	nonl();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	raw();
	wnoutrefresh(stdscr);

	gui_draw_colors =
	    config_getopt_bool("gui.color.enabled") && has_colors();
	gui_draw_ratio = config_getopt_percentage("gui.ratio");

	if (gui_draw_colors) {
		start_color();
#ifdef NCURSES_VERSION
		use_default_colors();
#endif /* NCURSES_VERSION */

		init_pair(GUI_COLOR_BAR,
		    config_getopt_color("gui.color.bar.fg"),
		    config_getopt_color("gui.color.bar.bg"));
		init_pair(GUI_COLOR_BLOCK,
		    config_getopt_color("gui.color.block.fg"),
		    config_getopt_color("gui.color.block.bg"));
		init_pair(GUI_COLOR_SELECT,
		    config_getopt_color("gui.color.select.fg"),
		    config_getopt_color("gui.color.select.bg"));
		init_pair(GUI_COLOR_DESELECT,
		    config_getopt_color("gui.color.deselect.fg"),
		    config_getopt_color("gui.color.deselect.bg"));
		init_pair(GUI_COLOR_MARKED,
		    config_getopt_color("gui.color.marked.fg"),
		    config_getopt_color("gui.color.marked.bg"));
	}

	gui_msgbar_init();
	gui_playq_init();
	gui_browser_init();
	gui_draw_done();
}

void
gui_draw_init_abort(void)
{
	endwin();
}

void
gui_draw_destroy(void)
{
	gui_lock();
	gui_msgbar_destroy();
	gui_playq_destroy();
	gui_browser_destroy();

	endwin();
	gui_unlock();
}

void
gui_draw_resize(void)
{
	gui_lock();
	wnoutrefresh(stdscr);
	gui_unlock();
	gui_msgbar_resize();
	gui_playq_resize();
	gui_browser_resize();
	gui_draw_done();
}

int
gui_draw_color_number(const char *name)
{
	if (strcmp(name, "black") == 0)
		return (COLOR_BLACK);
	else if (strcmp(name, "red") == 0)
		return (COLOR_RED);
	else if (strcmp(name, "green") == 0)
		return (COLOR_GREEN);
	else if (strcmp(name, "yellow") == 0)
		return (COLOR_YELLOW);
	else if (strcmp(name, "blue") == 0)
		return (COLOR_BLUE);
	else if (strcmp(name, "magenta") == 0)
		return (COLOR_MAGENTA);
	else if (strcmp(name, "cyan") == 0)
		return (COLOR_CYAN);
	else if (strcmp(name, "white") == 0)
		return (COLOR_WHITE);
#ifdef NCURSES_VERSION
	else if (strcmp(name, "default") == 0)
		return (-1);
#endif /* NCURSES_VERSION */

	return (-2);
}

void
gui_draw_done(void)
{
	gui_msgbar_refresh();
	gui_lock();
	doupdate();
	gui_unlock();
}
