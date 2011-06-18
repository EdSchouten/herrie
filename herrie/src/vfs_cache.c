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
 * @file vfs_cache.c
 * @brief Virtual filesystem cache.
 */

#include "stdinc.h"

#include "config.h"
#include "gui.h"
#include "vfs.h"

static GHashTable *refcache = NULL;

static void
vfs_cache_destroyvalue(gpointer data)
{
	struct vfsref *vr = data;

	vfs_close(vr);
}

void
vfs_cache_init(void)
{
	if (!config_getopt_bool("vfs.cache"))
		return;
	refcache = g_hash_table_new_full(g_str_hash, g_str_equal, NULL,
	    vfs_cache_destroyvalue);
}

void
vfs_cache_purge(void)
{
	if (refcache != NULL) {
		g_hash_table_remove_all(refcache);
		gui_msgbar_warn(_("VFS cache purged."));
	}
}

void
vfs_cache_add(const struct vfsref *nvr)
{
	struct vfsref *vr;

	if (refcache != NULL) {
		vr = vfs_dup(nvr);
		g_hash_table_replace(refcache, vr->ent->filename, vr);
	}
}

struct vfsref *
vfs_cache_lookup(const char *filename)
{
	struct vfsref *vr;

	if (refcache != NULL) {
		vr = g_hash_table_lookup(refcache, filename);
		if (vr != NULL)
			return vfs_dup(vr);
	}
	return (NULL);
}
