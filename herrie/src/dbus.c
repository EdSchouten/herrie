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
#include "gui.h"
/* XXX */
#include "gui_internal.h"

#define	TYPE_DBUS_SERVER	(dbus_server_get_type())
#define	DBUS_SERVER(obj)	\
	(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_DBUS_SERVER, DBusServer)

typedef struct {
	GObject parent;
} DBusServer;

typedef struct {
	GObjectClass parent_class;
} DBusServerClass;

#define HERRIE_BUS_NAME "info.herrie.Herrie"
#define HERRIE_PATH_NAME "/info/herrie/Herrie"

GType dbus_server_get_type(void);

G_DEFINE_TYPE(DBusServer, dbus_server, G_TYPE_OBJECT);

GMutex *dbus_mtx;

static void *
dbus_runner_thread(void *unused)
{
	DBusServer *controller;
	DBusGConnection *conn;
	GMainLoop *loop;
	DBusGProxy *proxy;
	GError *err = NULL;
	guint ret;
  
	g_type_init();
	controller = g_object_new(TYPE_DBUS_SERVER, NULL);
	loop = g_main_loop_new(NULL, FALSE);

	/* Connect to the session bus */
	conn = dbus_g_bus_get(DBUS_BUS_SESSION, &err);

	if(conn == NULL) {
		gui_msgbar_warn(_("Error getting DBus session bus."));
		goto error;
	}

	/* Register the controller object with DBus */
	dbus_g_connection_register_g_object(conn,
		HERRIE_PATH_NAME, G_OBJECT(controller));

	proxy = dbus_g_proxy_new_for_name(conn,
		DBUS_SERVICE_DBUS,
		DBUS_PATH_DBUS,
		DBUS_INTERFACE_DBUS);

	err = NULL;

	/* Register our bus name with DBus */
	if(!org_freedesktop_DBus_request_name(proxy,
		HERRIE_BUS_NAME, 0, &ret, &err)) {
		gui_msgbar_warn(_("Error requesting DBus bus name."));
		goto error;
	}

	/* Another process is using our DBus path, probably another herrie */
	if(ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		gui_msgbar_warn(_("DBus path already in use."));
		goto error;
	}

	gui_msgbar_warn("Here we go!");

	g_main_loop_run(loop);

	gui_msgbar_warn("Bye bye!");

error:
	g_main_loop_unref(loop);
	g_object_unref(controller);
	return NULL;
}

void
dbus_init(void)
{
	g_type_init();
	dbus_mtx = g_mutex_new();
}

void
dbus_spawn(void)
{
	g_thread_create(dbus_runner_thread, NULL, 0, NULL);
}

static gboolean
dbus_server_next(DBusServer *self, GError **error)
{
	dbus_lock();
	playq_cursong_next();
	dbus_unlock();

	return (TRUE);
}

static gboolean
dbus_server_pause(DBusServer *self, GError **error)
{
	dbus_lock();
	playq_cursong_pause();
	dbus_unlock();

	return (TRUE);
}

static gboolean
dbus_server_play(DBusServer *self, GError **error)
{
	dbus_lock();
	gui_playq_song_select();
	dbus_unlock();

	return (TRUE);
}

static gboolean
dbus_server_stop(DBusServer *self, GError **error)
{
	dbus_lock();
	playq_cursong_stop();
	dbus_unlock();

	return (TRUE);
}

static gboolean
dbus_server_volume_down(DBusServer *self, GError **error)
{
	dbus_lock();
	gui_playq_volume_down();
	dbus_unlock();

	return (TRUE);
}

static gboolean
dbus_server_volume_up(DBusServer *self, GError **error)
{
	dbus_lock();
	gui_playq_volume_up();
	dbus_unlock();

	return (TRUE);
}

#include <dbus_binding.h>

static void
dbus_server_init(DBusServer *self)
{
}

static void
dbus_server_class_init(DBusServerClass *klass)
{
	dbus_g_object_type_install_info(TYPE_DBUS_SERVER,
	    &dbus_glib_dbus_server_object_info);
}
