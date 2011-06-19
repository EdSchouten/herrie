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
 * @file main.c
 * @brief Application startup routine.
 */

#include "stdinc.h"

#ifdef BUILD_GST
#include <gst/gst.h>
#endif /* BUILD_GST */

#include "audio_output.h"
#include "config.h"
#include "dbus.h"
#include "gui.h"
#include "playq.h"
#include "scrobbler.h"
#include "vfs.h"

/**
 * @brief Display the application version and compile-time options.
 */
static void
version(void)
{
	g_printerr(APP_NAME " " APP_VERSION
		" (Two-clause BSD license"
#if defined(BUILD_AO) || defined(BUILD_MP3) || defined(BUILD_SNDFILE)
		", using GNU GPL licensed libraries"
#endif /* BUILD_AO || BUILD_MP3 || BUILD_SNDFILE */
		")\n\n"
		"%s: " CONFFILE "\n"
		"%s: " AUDIO_OUTPUT "\n"
		"%s: %s\n"
		"%s: %s\n"
		"%s: %s\n"
		"%s: %s\n"
		"%s:\n"
#ifdef BUILD_VORBIS
		"- Ogg Vorbis\n"
#endif /* BUILD_VORBIS */
#ifdef BUILD_GST
		"- GStreamer formats\n"
#endif /* BUILD_GST */
#ifdef BUILD_MP3
		"- MP3\n"
#endif /* BUILD_MP3 */
#ifdef BUILD_MODPLUG
		"- libmodplug\n"
#endif /* BUILD_MODPLUG */
#ifdef BUILD_SNDFILE
		"- libsndfile\n"
#endif /* BUILD_SNDFILE */
		,
		_("Global configuration file"),
		_("Audio output"),
		_("Support for AudioScrobbler"),
#ifdef BUILD_SCROBBLER
		_("yes"),
#else /* !BUILD_SCROBBLER */
		_("no"),
#endif /* BUILD_SCROBBLER */
		_("Support for DBus integration"),
#ifdef BUILD_DBUS
		_("yes"),
#else /* !BUILD_DBUS */
		_("no"),
#endif /* BUILD_DBUS */
		_("Support for HTTP streams"),
#ifdef BUILD_HTTP
		_("yes"),
#else /* !BUILD_HTTP */
		_("no"),
#endif /* BUILD_HTTP */
		_("Support for XSPF playlists (`spiff')"),
#ifdef BUILD_XSPF
		_("yes"),
#else /* !BUILD_XSPF */
		_("no"),
#endif /* BUILD_XSPF */
		_("Supported audio file formats"));

	exit(0);
}

/**
 * @brief Display the command line usage flags.
 */
static void
usage(void)
{
	g_printerr("%s: " APP_NAME " [-pvx] [-c configfile] "
	    "[file ...]\n", _("usage"));
	exit(1);
}

#if defined(BUILD_DBUS) || defined(BUILD_GST)
/**
 * @brief Main loop to process incoming DBus and GStreamer events.
 */
static void *
gmainloop_runner_thread(void *unused)
{
	GMainLoop *loop;

	gui_input_sigmask();
	loop = g_main_loop_new(NULL, FALSE);

#ifdef BUILD_DBUS
	dbus_before_mainloop();
#endif /* BUILD_DBUS */

	g_main_loop_run(loop);
	g_main_loop_unref(loop);

#ifdef BUILD_DBUS
	dbus_after_mainloop();
#endif /* BUILD_DBUS */
	return (NULL);
}
#endif /* defined(BUILD_DBUS) || defined(BUILD_GST) */

/**
 * @brief Startup routine for the application.
 */
int
main(int argc, char *argv[])
{
	int ch, i, show_version = 0, autoplay = 0, xmms = 0;
	char *cwd;
	const char *errmsg;
	struct vfsref *vr;
#ifdef CLOSE_STDERR
	int devnull;
#endif /* CLOSE_STDERR */

#ifdef BUILD_GST
	gst_init(NULL, NULL);
#endif /* BUILD_GST */

#ifdef BUILD_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(APP_NAME, TRANSDIR);
	textdomain(APP_NAME);
#endif /* BUILD_NLS */

	/* Global and local configuration files */
	config_load(CONFFILE, 1);
	config_load(CONFHOMEDIR "config", 1);

	while ((ch = getopt(argc, argv, "c:pvx")) != -1) {
		switch (ch) {
		case 'c':
			config_load(optarg, 0);
			break;
		case 'p':
			autoplay = 1;
			break;
		case 'v':
			show_version = 1;
			break;
		case 'x':
			xmms = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (show_version)
		version();

	g_thread_init(NULL);

	if (audio_output_open() != 0)
		return (-1);

	/* Already initialize the GUI before chroot() */
	gui_draw_init_pre();

#ifdef CLOSE_STDERR
	devnull = open("/dev/null", O_WRONLY);
#endif /* CLOSE_STDERR */
	if ((errmsg = vfs_lockup()) != NULL) {
		gui_draw_init_abort();
		g_printerr("%s", errmsg);
		return (1);
	}

	vfs_cache_init();

	/* Initialize the locks */
#ifdef BUILD_DBUS
	dbus_init();
#endif /* BUILD_DBUS */
#ifdef BUILD_SCROBBLER
	scrobbler_init();
#endif /* BUILD_SCROBBLER */
	playq_init(autoplay, xmms, (argc == 0));

	/* Draw all the windows */
	gui_draw_init_post();

	cwd = g_get_current_dir();
	for (i = 0; i < argc; i++) {
		/* Shell will expand ~ */
		if ((vr = vfs_lookup(argv[i], NULL, cwd, 1)) != NULL) {
			playq_song_add_tail(vr);
			vfs_close(vr);
		}
	}
	g_free(cwd);

#ifdef CLOSE_STDERR
	/* Close stderr before starting playback */
	dup2(devnull, STDERR_FILENO);
	close(devnull);
#endif /* CLOSE_STDERR */

	/* All set and done - spawn our threads */
	playq_spawn();
#if defined(BUILD_DBUS) || defined(BUILD_GST)
	g_thread_create(gmainloop_runner_thread, NULL, 0, NULL);
#endif /* defined(BUILD_DBUS) || defined(BUILD_GST) */
#ifdef BUILD_SCROBBLER
	scrobbler_spawn();
#endif /* BUILD_SCROBBLER */

	/* And off we go! */
	gui_input_loop();
	g_assert_not_reached();
	return (1);
}
