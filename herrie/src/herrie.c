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
 * @file herrie.c
 */

#include <glib.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef BUILD_TRANS
#include <locale.h>
#endif /* BUILD_TRANS */

#include "audio_output.h"
#include "config.h"
#include "gui.h"
#include "playq.h"
#include "scrobbler.h"
#include "trans.h"
#include "vfs.h"

/**
 * @brief Display the application version and compile-time options
 */
static void
version(void)
{
	g_printerr(APP_NAME " " APP_VERSION "\n"
		"%s: " AUDIO_OUTPUT "\n"
		"%s: %s\n"
		"%s:\n"
#ifdef BUILD_VORBIS
		"- Ogg Vorbis\n"
#endif /* BUILD_VORBIS */
#ifdef BUILD_MP3
		"- MP3\n"
#endif /* BUILD_MP3 */
#ifdef BUILD_SNDFILE
		"- libsndfile\n"
#endif /* BUILD_SNDFILE */
		,
		_("audio output"),
		_("support for audioscrobbler"),
#ifdef BUILD_SCROBBLER
		_("yes"),
#else /* !BUILD_SCROBBLER */
		_("no"),
#endif /* BUILD_SCROBBLER */
		_("supported audio file formats"));

	exit(0);
}

/**
 * @brief Display the command line usage flags
 */
static void
usage(void)
{
	g_printerr("%s: " APP_NAME " [-v] [-c configfile] "
	    "[file ...]\n", _("usage"));
	exit(1);
}

/**
 * @brief Startup routine for Herrie
 */
int
main(int argc, char *argv[])
{
	int ch, i, show_version = 0;
	char *configfile = NULL, *cwd;
	struct vfsref *vr;
#ifdef CLOSE_STDERR
	int devnull;
#endif /* CLOSE_STDERR */

#ifdef BUILD_TRANS
	setlocale(LC_ALL, "");
	bindtextdomain(APP_NAME, TRANSDIR);
	textdomain(APP_NAME);
#endif /* BUILD_TRANS */

	while ((ch = getopt(argc, argv, "vc:")) != -1) {
		switch (ch) {
		case 'v':
			show_version = 1;
			break;
		case 'c':
			configfile = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (show_version)
		version();

	config_load(configfile);

	if (audio_output_open() != 0)
		return (-1);

	/* Already initialize the GUI before chroot() */
	gui_draw_init_pre();

#ifdef CLOSE_STDERR
	devnull = open("/dev/null", O_WRONLY);
#endif /* CLOSE_STDERR */
#ifdef BUILD_LOCKUP
	if (vfs_lockup() != 0)
		return (-1);
#endif /* BUILD_LOCKUP */

	/* Initialize the locks */
	g_thread_init(NULL);
#ifdef BUILD_SCROBBLER
	scrobbler_init();
#endif /* BUILD_SCROBBLER */
	playq_init();

	/* Draw all the windows */
	gui_draw_init_post();

	cwd = g_get_current_dir();
	for (i = 0; i < argc; i++) {
		if ((vr = vfs_open(argv[i], NULL, cwd)) != NULL) {
			playq_song_add_tail(vr);
			vfs_close(vr);
		}
	}
	g_free(cwd);

	/* All set and done - spawn our threads */
	playq_spawn();
#ifdef BUILD_SCROBBLER
	scrobbler_spawn();
#endif /* BUILD_SCROBBLER */
#ifdef CLOSE_STDERR
	dup2(devnull, STDERR_FILENO);
	close(devnull);
#endif /* CLOSE_STDERR */
	gui_input_loop();

	/* Shutdown the application */
	playq_shutdown();
	audio_output_close();
	gui_draw_destroy();

	return (0);
}
