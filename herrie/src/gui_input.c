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
 * @file gui_input.c
 */

#include <glib.h>

#include <ctype.h>
#include <signal.h>
#if defined(SIGWINCH) && defined(G_THREADS_IMPL_POSIX)
#include <pthread.h>
#endif /* SIGWINCH && G_THREADS_IMPL_POSIX */
#ifdef BUILD_GUI_SIGWINCH_WRAPPER
#include <sys/ioctl.h>
#include <unistd.h>
#endif /* BUILD_GUI_SIGWINCH_WRAPPER */

#include "trans.h"
#include "playq.h"
#include "gui_internal.h"

/**
 * @brief The focus is on the browser.
 */
#define GUI_FOCUS_BROWSER	0
/**
 * @brief The focus is on the playlist.
 */
#define GUI_FOCUS_PLAYQ		1
/**
 * @brief The number of focusable windows.
 */
#define GUI_FOCUS_COUNT		2
/**
 * @brief Window that is currently focused.
 */
static int gui_input_curfocus = GUI_FOCUS_BROWSER;
/**
 * @brief Indicator of current selected window.
 */
char *gui_input_cursearch = NULL;

/**
 * @brief Fetch a character from the keyboard, already processing
 *        terminal resizes.
 */
static int
gui_input_getch(void)
{
	int ch;

	do {
		ch = getch();
		if (ch == KEY_RESIZE) {
			gui_draw_resize();
			ch = ERR;
		}
	} while (ch == ERR);

	/* End-key on xBSD is select? */
	if (ch == KEY_SELECT)
		return (KEY_END);

	/* ^? backspace */
	if (ch == 0x7f)
		return (KEY_BACKSPACE);
	
	/* Valid character */
	return (ch);
}

/**
 * @brief Switch the focus to the next window.
 */
static void
gui_input_switchfocus(void)
{
	gui_input_curfocus++;
	gui_input_curfocus %= GUI_FOCUS_COUNT;

	/* Update the selection colors */
	gui_playq_setfocus(gui_input_curfocus == GUI_FOCUS_PLAYQ);
	gui_browser_setfocus(gui_input_curfocus == GUI_FOCUS_BROWSER);
}

/**
 * @brief Ask the user to enter a new search string and perform the
 *        first search.
 */
static void
gui_input_search(void)
{
	char *str;

	/* Allow the user to enter a search string */
	str = gui_input_askstring(_("Search for"), gui_input_cursearch);
	if (str == NULL)
		return;

	/* Replace our search string */
	g_free(gui_input_cursearch);
	gui_input_cursearch = str;

	if (gui_input_curfocus == GUI_FOCUS_BROWSER)
		gui_browser_searchnext();
	else if (gui_input_curfocus == GUI_FOCUS_PLAYQ)
		gui_playq_searchnext();
}


/**
 * @brief Prompt the user with a message to confirm termination of the
 *        application.
 */
static int
gui_input_quit(void)
{
	return gui_input_askyesno(_("Quit herrie?"));
}

/**
 * @brief A simple binding from a keyboard character input to a function.
 */
struct gui_binding {
	/**
	 * @brief The window that should be focussed.
	 */
	int focus;
	/**
	 * @brief The character that should be pressed. 
	 */
	int input;
	/**
	 * @brief The function that will be run.
	 */
	void (*func)(void);
};

/**
 * @brief List of keybindings available in the GUI.
 */
static struct gui_binding kbdbindings[] = {
	/* Application-wide keyboard bindings */
	{ -1, '<',			playq_cursong_seek_backward, },
	{ -1, '>',			playq_cursong_seek_forward, },
	{ -1, 'p',			playq_cursong_pause, },
	{ -1, 'q',			NULL }, /* Quit the application */
	{ -1, 's',			playq_cursong_skip },
	{ -1, '\t', 			gui_input_switchfocus },
	{ -1, 0x17, 			gui_input_switchfocus }, /* ^W */
	{ -1, '/',			gui_input_search },

	/* Keyboard bindings for the file browser */
	{ GUI_FOCUS_BROWSER, ' ',	gui_browser_cursor_pagedown },
	{ GUI_FOCUS_BROWSER, 'a',	gui_browser_playq_add_after },
	{ GUI_FOCUS_BROWSER, 'A',	gui_browser_playq_add_tail },
	{ GUI_FOCUS_BROWSER, 'c',	gui_browser_chdir },
	{ GUI_FOCUS_BROWSER, 'G',	gui_browser_cursor_bottom },
	{ GUI_FOCUS_BROWSER, 'g',	gui_browser_cursor_top },
	{ GUI_FOCUS_BROWSER, 'h',	gui_browser_dir_parent },
	{ GUI_FOCUS_BROWSER, 'i',	gui_browser_playq_add_before },
	{ GUI_FOCUS_BROWSER, 'I',	gui_browser_playq_add_head },
	{ GUI_FOCUS_BROWSER, 'j',	gui_browser_cursor_down },
	{ GUI_FOCUS_BROWSER, 'k',	gui_browser_cursor_up },
	{ GUI_FOCUS_BROWSER, 'l',	gui_browser_dir_enter },
	{ GUI_FOCUS_BROWSER, 'n',	gui_browser_searchnext },
	{ GUI_FOCUS_BROWSER, 0x02,	gui_browser_cursor_pageup }, /* ^B */
	{ GUI_FOCUS_BROWSER, 0x06,	gui_browser_cursor_pagedown }, /* ^F */
	{ GUI_FOCUS_BROWSER, KEY_DOWN,	gui_browser_cursor_down },
	{ GUI_FOCUS_BROWSER, KEY_END,	gui_browser_cursor_bottom },
	{ GUI_FOCUS_BROWSER, KEY_HOME,	gui_browser_cursor_top },
	{ GUI_FOCUS_BROWSER, KEY_LEFT,	gui_browser_dir_parent },
	{ GUI_FOCUS_BROWSER, KEY_NPAGE,	gui_browser_cursor_pagedown },
	{ GUI_FOCUS_BROWSER, KEY_PPAGE,	gui_browser_cursor_pageup },
	{ GUI_FOCUS_BROWSER, KEY_RIGHT,	gui_browser_dir_enter },
	{ GUI_FOCUS_BROWSER, KEY_UP,	gui_browser_cursor_up },

	/* Keyboard bindings for the playlist */
	{ GUI_FOCUS_PLAYQ, ' ',		gui_playq_cursor_pagedown },
	{ GUI_FOCUS_PLAYQ, 'd',		gui_playq_song_remove },
	{ GUI_FOCUS_PLAYQ, 'D',		gui_playq_song_remove_all },
	{ GUI_FOCUS_PLAYQ, 'G',		gui_playq_cursor_bottom },
	{ GUI_FOCUS_PLAYQ, 'g',		gui_playq_cursor_top },
	{ GUI_FOCUS_PLAYQ, 'j',		gui_playq_cursor_down },
	{ GUI_FOCUS_PLAYQ, 'k',		gui_playq_cursor_up },
	{ GUI_FOCUS_PLAYQ, 'n',		gui_playq_searchnext },
	{ GUI_FOCUS_PLAYQ, 'r',		gui_playq_song_randomize },
	{ GUI_FOCUS_PLAYQ, '[',		gui_playq_song_moveup },
	{ GUI_FOCUS_PLAYQ, ']',		gui_playq_song_movedown },
	{ GUI_FOCUS_PLAYQ, 0x02,	gui_playq_cursor_pageup }, /* ^B */
	{ GUI_FOCUS_PLAYQ, 0x06,	gui_playq_cursor_pagedown }, /* ^F */
	{ GUI_FOCUS_PLAYQ, KEY_DOWN,	gui_playq_cursor_down },
	{ GUI_FOCUS_PLAYQ, KEY_END,	gui_playq_cursor_bottom },
	{ GUI_FOCUS_PLAYQ, KEY_HOME,	gui_playq_cursor_top },
	{ GUI_FOCUS_PLAYQ, KEY_NPAGE,	gui_playq_cursor_pagedown },
	{ GUI_FOCUS_PLAYQ, KEY_PPAGE,	gui_playq_cursor_pageup },
	{ GUI_FOCUS_PLAYQ, KEY_UP,	gui_playq_cursor_up },
};
/**
 * @brief Amount of keybindings.
 */
#define NUM_BINDINGS (sizeof kbdbindings / sizeof(struct gui_binding))

void
gui_input_sigmask(void)
{
#if defined(SIGWINCH) && defined(G_THREADS_IMPL_POSIX)
	sigset_t sset;

	sigemptyset(&sset);
	sigaddset(&sset, SIGWINCH);
	pthread_sigmask(SIG_BLOCK, &sset, NULL);
#endif /* SIGWINCH && G_THREADS_IMPL_POSIX */
}

/**
 * @brief Handler of all incoming signals with a custom action.
 */
static void
gui_input_sighandler(int signal)
{
#ifdef BUILD_GUI_SIGWINCH_WRAPPER
	struct winsize ws;
#endif /* BUILD_GUI_SIGWINCH_WRAPPER */

	switch (signal) {
	case SIGUSR1:
		playq_cursong_pause();
		break;
	case SIGUSR2:
		playq_cursong_skip();
		break;
#ifdef BUILD_GUI_SIGWINCH_WRAPPER
	case SIGWINCH:
		if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != -1) {
			resizeterm(ws.ws_row, ws.ws_col);
			gui_draw_resize();
		}
		break;
#endif /* BUILD_GUI_SIGWINCH_WRAPPER */
	}
}

void
gui_input_loop(void)
{
	int ch, i;

	signal(SIGUSR1, gui_input_sighandler);
	signal(SIGUSR2, gui_input_sighandler);
#ifdef BUILD_GUI_SIGWINCH_WRAPPER
	signal(SIGWINCH, gui_input_sighandler);
#endif /* BUILD_GUI_SIGWINCH_WRAPPER */

	for (;;) {
		ch = gui_input_getch();
		gui_msgbar_flush();

		for (i = 0; i < NUM_BINDINGS; i++) {
			/* Let's see if the button matches */
			if (kbdbindings[i].input != ch ||
			    (kbdbindings[i].focus != -1 &&
			     kbdbindings[i].focus != gui_input_curfocus))
				continue;
			
			if (kbdbindings[i].func == NULL) {
				if (gui_input_quit() == 0)
					return;
			} else {
				kbdbindings[i].func();
			}
		}

		gui_msgbar_refresh();
	}
}

int
gui_input_askyesno(const char *question)
{
	char *msg;

	/* Print the question on screen */
	msg = g_strdup_printf("%s ([%s]/%s): ", question, _("yes"), _("no"));
	gui_msgbar_ask(msg);
	g_free(msg);

	for (;;) {
		switch(gui_input_getch()) {
		case 'y':
		case 'Y':
		case '\r':
			gui_msgbar_flush();
			return (0);
		case 'n':
		case 'N':
		case 0x03: /* ^C */
			gui_msgbar_flush();
			return (-1);
		}
	}
}

/**
 * @brief Find the offset to where a string should be trimmed to remove
 *        one word or special character sequence, including trailing
 *        whitespace.
 */
static int
gui_input_trimword(GString *gs)
{
	const char *end;

	/* Last character */
	end = (gs->str + gs->len) - 1;

	/* Trim as much trailing whitespace as possible */
	for (;;) {
		if (end < gs->str) return (0);
		if (!isspace(*end)) break;
		end--;
	}

	if (isalnum(*end)) {
		/* Trim alphanumerics */
		do {
			if (--end < gs->str) return (0);
		} while (isalnum(*end));
	} else {
		/* Trim special characters */
		do {
			if (--end < gs->str) return (0);
		} while (!isalnum(*end) && !isspace(*end));
	}

	return (end - gs->str) + 1;
}

char *
gui_input_askstring(char *question, char *defstr)
{
	GString *msg;
	int origlen, newlen, clearfirst = 0;
	int c;
	char *ret = NULL;

	msg = g_string_new(question);
	g_string_append(msg, ": ");
	origlen = msg->len;
	if (defstr != NULL) {
		g_string_append(msg, defstr);
		clearfirst = 1;
	}

	for(;;) {
		gui_msgbar_ask(msg->str);

		switch(c = gui_input_getch()) {
		case '\r':
			goto done;
		case KEY_BACKSPACE:
			clearfirst = 0;
			if (msg->len > origlen) {
				/* Prompt has contents */
				g_string_truncate(msg, msg->len - 1);
			}
			break;
		case 0x03: /* ^C */
			/* Just empty the return */
			g_string_truncate(msg, origlen);
			goto done;
		case 0x15: /* ^U */
			g_string_truncate(msg, origlen);
			break;
		case 0x17: /* ^W */
			clearfirst = 0;
			newlen = gui_input_trimword(msg);
			g_string_truncate(msg, MAX(newlen, origlen));
			break;
		default:
			if (!g_ascii_iscntrl(c)) {
				if (clearfirst) {
					g_string_truncate(msg, origlen);
					clearfirst = 0;
				}
				g_string_append_c(msg, c);
			}
		}
	}

done:
	gui_msgbar_flush();

	/* Only return a string when there are contents */
	if (msg->len > origlen)
		ret = g_strdup(msg->str + origlen);

	g_string_free(msg, TRUE);
	return ret;
}
