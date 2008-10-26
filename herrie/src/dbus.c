/*
 * Copyright (c) 2006-2008 Ed Schouten <ed@80386.nl>
 * All rights reserved.
 *
 * Copyright (c) 2008 Steve Jothen <sjothen@gmail.com>
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
 * @file dbus.c
 * @brief DBus integration.
 */

#include "stdinc.h"

#include <dbus/dbus-glib-bindings.h>

#include "dbus.h"
#include "playq.h"
#include "gui_internal.h"

/**
 * @brief Obtain the type ID of the DBusServer object. This should
 *        actually be static, but G_DEFINE_TYPE does not allow us to
 *        mark it that way.
 */
GType dbus_server_get_type(void);

/**
 * @brief Per-DBusServer instance private data.
 */
typedef struct {
	/**
	 * @brief Parent class data.
	 */
	GObject parent;
} DBusServer;

/**
 * @brief DBusServer class private data.
 */
typedef struct {
	/**
	 * @brief Parent class data.
	 */
	GObjectClass parent_class;
} DBusServerClass;

/**
 * @brief Class registration for DBusServer.
 */
G_DEFINE_TYPE(DBusServer, dbus_server, G_TYPE_OBJECT);

GMutex *dbus_mtx;

/**
 * @brief DBus bus name used by this application.
 */
#define HERRIE_BUS_NAME "info.herrie.Herrie"
/**
 * @brief DBus path name used by this application.
 */
#define HERRIE_PATH_NAME "/info/herrie/Herrie"

/**
 * @brief Main loop to process incoming DBus events.
 */
static void *
dbus_runner_thread(void *unused)
{
	DBusServer *ds;
	DBusGConnection *dc;
	GMainLoop *loop;
	DBusGProxy *proxy;
	guint ret;
  
	g_type_init();
	ds = g_object_new(dbus_server_get_type(), NULL);
	loop = g_main_loop_new(NULL, FALSE);

	/* Connect to the session bus. */
	dc = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	if (dc == NULL)
		goto done;

	/* Register the controller object with DBus. */
	dbus_g_connection_register_g_object(dc, HERRIE_PATH_NAME,
	    G_OBJECT(ds));

	proxy = dbus_g_proxy_new_for_name(dc, DBUS_SERVICE_DBUS,
	    DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	/* Register our bus name with DBus. */
	if (org_freedesktop_DBus_request_name(proxy, HERRIE_BUS_NAME,
	    0, &ret, NULL) == 0)
		goto done;

	/*
	 * Another process is using our DBus path, probably another
	 * instance of the application.
	 */
	if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
		goto done;

	g_main_loop_run(loop);

done:	g_main_loop_unref(loop);
	g_object_unref(ds);
	return (NULL);
}

/**
 * @brief Initialize the lock used by DBus.
 */
void
dbus_init(void)
{
	g_type_init();
	dbus_mtx = g_mutex_new();
}

/**
 * @brief Spawn the thread to process DBus events.
 */
void
dbus_spawn(void)
{
	g_thread_create(dbus_runner_thread, NULL, 0, NULL);
}

/**
 * @brief DBus event to skip to the next track.
 */
static gboolean
dbus_server_next(DBusServer *self, GError **error)
{
	dbus_lock();
	playq_cursong_next();
	dbus_unlock();

	return (TRUE);
}

/**
 * @brief DBus event to pause the current track.
 */
static gboolean
dbus_server_pause(DBusServer *self, GError **error)
{
	dbus_lock();
	playq_cursong_pause();
	dbus_unlock();

	return (TRUE);
}

/**
 * @brief DBus event to start playback.
 */
static gboolean
dbus_server_play(DBusServer *self, GError **error)
{
	dbus_lock();
	gui_playq_song_select();
	dbus_unlock();

	return (TRUE);
}

/**
 * @brief DBus event to stop playback.
 */
static gboolean
dbus_server_stop(DBusServer *self, GError **error)
{
	dbus_lock();
	playq_cursong_stop();
	dbus_unlock();

	return (TRUE);
}

/**
 * @brief DBus event to adjust the volume upwards.
 */
static gboolean
dbus_server_volume_down(DBusServer *self, GError **error)
{
	dbus_lock();
	gui_playq_volume_down();
	dbus_unlock();

	return (TRUE);
}

/**
 * @brief DBus event to adjust the volume downwards.
 */
static gboolean
dbus_server_volume_up(DBusServer *self, GError **error)
{
	dbus_lock();
	gui_playq_volume_up();
	dbus_unlock();

	return (TRUE);
}

#include <dbus_binding.h>

/**
 * @brief Per-DBusServer instance constructor.
 */
static void
dbus_server_init(DBusServer *self)
{
}

/**
 * @brief DBusServer class constructor.
 */
static void
dbus_server_class_init(DBusServerClass *klass)
{
	dbus_g_object_type_install_info(dbus_server_get_type(),
	    &dbus_glib_dbus_server_object_info);
}
