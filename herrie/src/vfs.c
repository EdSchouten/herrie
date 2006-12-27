/*
 * Copyright (c) 2006 Ed Schouten <ed@fxq.nl>
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
 *
 * $Id: vfs.c 1042 2006-12-21 10:42:23Z ed $
 */
/**
 * @file vfs.c
 */

#include <glib.h>

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef BUILD_LOCKUP
#include <pwd.h>
#endif /* BUILD_LOCKUP */

#include "config.h"
#include "trans.h"
#include "vfs.h"
#include "vfs_modules.h"

/**
 * @brief List of available virtual filesystem modules and their options
 *        and functions.
 */
static struct vfsmodule modules[] = {
	{ vfs_m3u_open, NULL, vfs_m3u_populate, 0, 0, VFS_SORT_LAST, '@' },
	{ vfs_pls_open, NULL, vfs_pls_populate, 0, 0, VFS_SORT_LAST, '@' },
	/*
	 * Leave these two rules at the bottom of the list. They have
	 * the weakest matching rules.
	 */
	{ vfs_dir_open, NULL, vfs_dir_populate, 1, 0, VFS_SORT_FIRST, G_DIR_SEPARATOR },
	{ vfs_file_open, NULL, NULL, 1, 1, VFS_SORT_LAST, '\0' },
};
/**
 * @brief The number of virtual file system modules currently available
 *        in the application.
 */
#define NUM_MODULES (sizeof(modules) / sizeof(struct vfsmodule))

#ifdef BUILD_LOCKUP
int
vfs_lockup(void)
{
	const char *root;
	const char *user;
	struct passwd *pw = NULL;

	user = config_getopt("vfs.lockup.user");
	if (user[0] != '\0') {
		pw = getpwnam(user);
		if (pw == NULL) {
			g_printerr(_("Unknown user: %s\n"), user);
			return (-1);
		}
	}

	root = config_getopt("vfs.lockup.chroot");
	if (root[0] != '\0') {
		/* Try to lock ourselves in */
		if (chroot(root) != 0) {
			g_printerr(_("Unable to chroot in %s\n"), root);
			return (-1);
		}

		chdir("/");
	}

	if (pw != NULL) {
		if (setgid(pw->pw_gid) != 0) {
			g_printerr(_("Unable to change to group %d\n"),
			    (int)pw->pw_gid);
			return (-1);
		}
		if (setuid(pw->pw_uid) != 0) {
			g_printerr(_("Unable to change to user %d\n"),
			    (int)pw->pw_uid);
			return (-1);
		}
	}

	return (0);
}
#endif /* BUILD_LOCKUP */

/**
 * @brief Deallocates the data structures for a VFS entity
 */
static void
vfs_dealloc(struct vfsent *ve)
{
	g_free(ve->name);
	g_free(ve->filename);
	g_slice_free(struct vfsent, ve);
}

/**
 * @brief Concatenate a path- and filename. The resulting filename will
 *        not contain /./'s and /../'s.
 */
static char *
vfs_path_concat(const char *dir, const char *file)
{
	GString *npath;
	char *tmp1, *off;

	if (g_path_is_absolute(file)) {
		/* We already have an absolute path */
		npath = g_string_new(file);
	} else if (dir != NULL) {
		g_assert(g_path_is_absolute(dir));

		/* Use the predefined basedir */
		tmp1 = g_build_filename(dir, file, NULL);
		npath = g_string_new(tmp1);
		g_free(tmp1);
	} else {
		/* Relative filename with no base */
		return (NULL);
	}

	/* Remove /./ and /../ */
	for (off = npath->str;
	    (off = strchr(off, G_DIR_SEPARATOR)) != NULL;) {
	    	if (off[1] == '\0' || off[1] == G_DIR_SEPARATOR) {
			/* /foo//bar -> /foo/bar */
			g_string_erase(npath, off - npath->str, 1);
		} else if (off[1] == '.' &&
		    (off[2] == '\0' || off[2] == G_DIR_SEPARATOR)) {
			/* /foo/./bar -> /foo/bar */
			g_string_erase(npath, off - npath->str, 2);
		} else if (off[1] == '.' && off[2] == '.' &&
		    (off[3] == '\0' || off[3] == G_DIR_SEPARATOR)) {
			/* /foo/../bar -> /bar */

			/* Strip one directory below */
			*off = '\0';
			tmp1 = strrchr(npath->str, G_DIR_SEPARATOR);
			if (tmp1 != NULL) {
				g_string_erase(npath, tmp1 - npath->str,
				    (off - tmp1) + 3);
				off = tmp1;
			} else {
				/* Woops! Too many ..'s */
				g_string_free(npath, TRUE);
				return (NULL);
			}
		} else {
			/* Nothing useful */
			off++;
		}
	}

	if (npath->len == 0)
		g_string_assign(npath, G_DIR_SEPARATOR_S);

	return g_string_free(npath, FALSE);
}

struct vfsref *
vfs_open(const char *filename, const char *name, const char *basepath)
{
	char *fn;
	struct vfsent *ve;
	struct vfsref *vr;
	struct stat fs;
	int i;

	fn = vfs_path_concat(basepath, filename);
	if (fn == NULL)
		return (NULL);

	/* We only allow files and directories */
	if (stat(fn, &fs) != 0 ||
	    (!S_ISREG(fs.st_mode) && !S_ISDIR(fs.st_mode))) {
		g_free(fn);
		return (NULL);
	}

	/* Initialize our new VFS structure with minimal properties */
	ve = g_slice_new0(struct vfsent);
	ve->filename = fn;
	vfs_list_init(&ve->population);

	/* The name argument */
	if (name != NULL) {
		/* Set a predefined name */
		ve->name = g_strdup(name);
	} else {
		/* Retreive it from the filename */
		ve->name = g_path_get_basename(ve->filename);
	}

	/* Try to find a matching VFS module */
	for (i = 0; i < NUM_MODULES; i++) {
		if (modules[i].vopen(ve, S_ISDIR(fs.st_mode)) == 0) {
			ve->vmod = &modules[i];
			break;
		}
	}

	/* No matching VFS module */
	if (ve->vmod == NULL) {
		vfs_dealloc(ve);
		return (NULL);
	}

	/* Return reference object */
	ve->refcount = 1;
	vr = g_slice_new(struct vfsref);
	vr->ent = ve;
	return (vr);
}

struct vfsref *
vfs_dup(struct vfsref *vr)
{
	struct vfsent *ve;

	ve = vr->ent;
	g_atomic_int_inc(&ve->refcount);

	vr = g_slice_new(struct vfsref);
	vr->ent = ve;

	return (vr);
}

void
vfs_close(struct vfsref *vr)
{
	struct vfsref *cur;

	if (g_atomic_int_dec_and_test(&vr->ent->refcount)) {
		/* Deallocate the underlying vfsent */
		while ((cur = vfs_list_first(vfs_population(vr))) != NULL) {
			vfs_list_remove(vfs_population(vr), cur);
			vfs_close(cur);
		}

		if (vr->ent->vmod->vclose != NULL)
			vr->ent->vmod->vclose(vr->ent);
		
		vfs_dealloc(vr->ent);
	}
	
	/* Deallocate the reference to it */
	g_slice_free(struct vfsref, vr);
}

int
vfs_populate(struct vfsref *vr)
{
	/* Some object cannot be populated */
	if (!vfs_populatable(vr))
		return (-1);

	/* Don't fetch double data */
	if (!vfs_list_empty(vfs_population(vr)))
		return (0);

	return vr->ent->vmod->vpopulate(vr->ent);
}

void
vfs_unfold(struct vfslist *vl, struct vfsref *vr)
{
	struct vfsref *cvr;

	if (vfs_playable(vr)) {
		/* Single item - add it to the list */
		cvr = vfs_dup(vr);
		vfs_list_insert_tail(vl, cvr);
	} else {
		/* See if we can recurse it */
		vfs_populate(vr);
		vfs_list_foreach(vfs_population(vr), cvr) {
			if (vfs_recurse(cvr))
				vfs_unfold(vl, cvr);
		}
	}
}
