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
 * @file vfs.c
 * @brief Virtual filesystem.
 */

#include "config.h"
#include "vfs.h"
#include "vfs_modules.h"

/**
 * @brief List of available virtual filesystem modules and their options
 *        and functions.
 */
static struct vfsmodule modules[] = {
#ifdef BUILD_HTTP
	{ vfs_http_open, NULL, vfs_http_handle, 1, 1, VFS_SORT_LAST, '^' },
#endif /* BUILD_HTTP */
	{ vfs_m3u_open, vfs_m3u_populate, NULL, 0, 0, VFS_SORT_LAST, '@' },
	{ vfs_pls_open, vfs_pls_populate, NULL, 0, 0, VFS_SORT_LAST, '@' },
	/*
	 * Leave these two rules at the bottom of the list. They have
	 * the weakest matching rules.
	 */
	{ vfs_dir_open, vfs_dir_populate, NULL, 0, 1, VFS_SORT_FIRST, G_DIR_SEPARATOR },
	{ vfs_file_open, NULL, vfs_file_handle, 0, 1, VFS_SORT_LAST, '\0' },
};
/**
 * @brief The number of virtual file system modules currently available
 *        in the application.
 */
#define NUM_MODULES (sizeof(modules) / sizeof(struct vfsmodule))

const char *
vfs_lockup(void)
{
#ifdef G_OS_UNIX
	const char *root;
	const char *user;
	struct passwd *pw = NULL;

	user = config_getopt("vfs.lockup.user");
	if (user[0] != '\0') {
		pw = getpwnam(user);
		if (pw == NULL)
			return g_strdup_printf(
			    _("Unknown user: %s\n"), user);
	}

	root = config_getopt("vfs.lockup.chroot");
	if (root[0] != '\0') {
#ifndef __CYGWIN__
		/* Already load the resolv.conf */
		res_init();
#endif /* !__CYGWIN__ */
		/* Try to lock ourselves in */
		if (chroot(root) != 0)
			return g_strdup_printf(
			    _("Unable to chroot in %s\n"), root);

		chdir("/");
	}

	if (pw != NULL) {
		if (setgid(pw->pw_gid) != 0)
			return g_strdup_printf(
			    _("Unable to change to group %d\n"),
			    (int)pw->pw_gid);
		if (setuid(pw->pw_uid) != 0)
			return g_strdup_printf(
			    _("Unable to change to user %d\n"),
			    (int)pw->pw_uid);
	}
#endif /* G_OS_UNIX */

	return (NULL);
}

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
		/* Basedir should be absolute */
		if (!g_path_is_absolute(dir))
			return (NULL);

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
	unsigned int i;
	int pseudo = 0;

	fn = vfs_path_concat(basepath, filename);
	if (fn == NULL)
		return (NULL);

	/* We only allow files and directories */
	if (stat(fn, &fs) != 0) {
		/* Could be a network stream */
		pseudo = 1;
		/* Don't prepend the dirnames */
		g_free(fn);
		fn = g_strdup(filename);
	} else if (!S_ISREG(fs.st_mode) && !S_ISDIR(fs.st_mode)) {
		/* Device nodes and such */
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
	} else if (pseudo) {
		/* Pseudo file - just copy the URL */
		ve->name = g_strdup(ve->filename);
	} else {
		/* Get the basename from the filename */
		ve->name = g_path_get_basename(ve->filename);
	}

	/* Try to find a matching VFS module */
	for (i = 0; i < NUM_MODULES; i++) {
		if (pseudo && !modules[i].pseudo)
			continue;
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
	vr = g_slice_new0(struct vfsref);
	vr->ent = ve;
	return (vr);
}

struct vfsref *
vfs_dup(const struct vfsref *vr)
{
	struct vfsent *ve;
	struct vfsref *rvr;

	ve = vr->ent;
	g_atomic_int_inc(&ve->refcount);

	rvr = g_slice_new0(struct vfsref);
	rvr->ent = ve;

	return (rvr);
}

void
vfs_close(struct vfsref *vr)
{
	struct vfsref *cur;

	if (g_atomic_int_dec_and_test(&vr->ent->refcount)) {
		/* Deallocate the underlying vfsent */
		while ((cur = vfs_list_first(&vr->ent->population)) != NULL) {
			vfs_list_remove(&vr->ent->population, cur);
			vfs_close(cur);
		}
		
		vfs_dealloc(vr->ent);
	}
	
	/* Deallocate the reference to it */
	g_slice_free(struct vfsref, vr);
}

int
vfs_populate(const struct vfsref *vr)
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
vfs_unfold(struct vfslist *vl, const struct vfsref *vr)
{
	struct vfsref *cvr;

	if (vfs_playable(vr)) {
		/* Single item - add it to the list */
		cvr = vfs_dup(vr);
		vfs_list_insert_tail(vl, cvr);
	} else {
		/* See if we can recurse it */
		vfs_populate(vr);
		vfs_list_foreach(&vr->ent->population, cvr) {
			if (vfs_recurse(cvr))
				vfs_unfold(vl, cvr);
		}
	}
}

struct vfsref *
vfs_write_playlist(struct vfslist *vl, struct vfsref *vr,
    const char *filename)
{
	const char *base;
	char *fn, *nfn;
	size_t cmplen;
	FILE *fio;
	struct vfsref *cvr, *rvr = NULL;
	unsigned int idx = 1;

	if (vr != NULL)
		base = vfs_filename(vr);
	fn = vfs_path_concat(base, filename);
	if (fn == NULL)
		return (NULL);
	if (!g_str_has_suffix(fn, ".pls")) {
		/* Append .pls extension */
		nfn = g_strdup_printf("%s.pls", fn);
		g_free(fn);
		fn = nfn;
	}
	fio = fopen(fn, "w");
	if (fio == NULL)
		goto done;
	
	/* Directory name length of .pls filename */
	base = strrchr(fn, G_DIR_SEPARATOR);
	g_assert(base != NULL);
	cmplen = base - fn + 1;

	fprintf(fio, "[playlist]\nNumberOfEntries=%u\n",
	    vfs_list_items(vl));
	vfs_list_foreach(vl, cvr) {
		/* Skip directory name when relative is possible */
		base = vfs_filename(cvr);
		if (strncmp(fn, base, cmplen) == 0)
			base += cmplen;

		fprintf(fio, "File%u=%s\nTitle%u=%s\n",
		    idx, base,
		    idx, vfs_name(cvr));
		idx++;
	}
	fclose(fio);

	rvr = vfs_open(fn, NULL, NULL);
	g_assert(rvr != NULL);
done:	g_free(fn);
	return (rvr);
}
