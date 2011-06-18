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
 * @file vfs.c
 * @brief Virtual filesystem.
 */

#include "stdinc.h"

#include "config.h"
#include "vfs.h"
#include "vfs_modules.h"

/**
 * @brief List of available virtual filesystem modules and their options
 *        and functions.
 */
static struct vfsmodule modules[] = {
#ifdef BUILD_HTTP
	{ vfs_http_match, NULL, vfs_http_open, 1, 1, '^' },
#endif /* BUILD_HTTP */
	{ vfs_m3u_match, vfs_m3u_populate, NULL, 0, 1, '@' },
	{ vfs_pls_match, vfs_pls_populate, NULL, 0, 1, '@' },
#ifdef BUILD_XSPF
	{ vfs_xspf_match, vfs_xspf_populate, NULL, 0, 1, '@' },
#endif /* BUILD_XSPF */
	/*
	 * Leave these two rules at the bottom of the list. They have
	 * the weakest matching rules.
	 */
	{ vfs_dir_match, vfs_dir_populate, NULL, 0, 0, G_DIR_SEPARATOR },
	{ vfs_file_match, NULL, vfs_file_open, 0, 1, '\0' },
};
/**
 * @brief The number of virtual file system modules currently available
 *        in the application.
 */
#define NUM_MODULES (sizeof modules / sizeof(struct vfsmodule))

/**
 * @brief Playlist writing module object. Matching is performed by
 *        extension.
 */
struct vfswriter {
	/**
	 * @brief Write routine.
	 */
	int (*write)(const struct vfslist *vl, const char *filename);
	/**
	 * @brief File extension.
	 */
	char *ext;
};

/**
 * @brief List of VFS writing modules. The first item in the list is
 *        used when no matching extension was found. In that case, the
 *        extension of the first item will be appended to the filename.
 */
static struct vfswriter writers[] = {
#ifdef BUILD_XSPF
	{ vfs_xspf_write, ".xspf" },
#endif /* BUILD_XSPF */
	{ vfs_pls_write, ".pls" },
	{ vfs_m3u_write, ".m3u" },
};
/**
 * @brief The number of VFS writing modules currently available in the
 *        application.
 */
#define NUM_WRITERS (sizeof writers / sizeof(struct vfswriter))

/**
 * @brief Concatenate a path- and filename. The resulting filename will
 *        not contain /./'s and /../'s.
 */
static char *
vfs_path_concat(const char *dir, const char *file, int strict)
{
	GString *npath;
	char *tmp, *off;
#ifdef G_OS_UNIX
	const char *uend;
	struct passwd *pw;
#endif /* G_OS_UNIX */

	if (!strict && file[0] == '~' &&
	    (file[1] == '\0' || file[1] == G_DIR_SEPARATOR)) {
		/* Expand ~ to go to the home directory */
		npath = g_string_new(g_get_home_dir());
		g_string_append(npath, file + 1);
#ifdef G_OS_UNIX
	} else if (!strict && file[0] == '~') {
		/* Expand ~username - UNIX only */
		uend = strchr(file + 1, G_DIR_SEPARATOR);

		/* Temporarily split the string and resolve the username */
		if (uend != NULL) {
			tmp = g_strndup(file + 1, uend - (file + 1));
			pw = getpwnam(tmp);
			g_free(tmp);
		} else {
			pw = getpwnam(file + 1);
		}
		if (pw == NULL)
			return (NULL);

		/* Create the new pathname */
		npath = g_string_new(pw->pw_dir);
		if (uend != NULL)
			g_string_append(npath, uend);
#endif /* G_OS_UNIX */
	} else if (g_path_is_absolute(file)) {
		/* We already have an absolute path */
		npath = g_string_new(file);
	} else if (dir != NULL) {
		/* Basedir should be absolute */
		if (!g_path_is_absolute(dir))
			return (NULL);

		/* Use the predefined basedir */
		tmp = g_build_filename(dir, file, NULL);
		npath = g_string_new(tmp);
		g_free(tmp);
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
			tmp = strrchr(npath->str, G_DIR_SEPARATOR);
			if (tmp != NULL) {
				g_string_erase(npath, tmp - npath->str,
				    (off - tmp) + 3);
				off = tmp;
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

const char *
vfs_lockup(void)
{
#ifdef G_OS_UNIX
	const char *root;
	const char *user;
	char *rootpath;
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
#ifdef BUILD_RES_INIT
		/* Already load the resolv.conf */
		res_init();
#endif /* BUILD_RES_INIT */

		/* Try to lock ourselves in */
		rootpath = vfs_path_concat(NULL, root, 0);
		if (rootpath == NULL || chroot(rootpath) != 0)
			return g_strdup_printf(
			    _("Unable to chroot in %s\n"),
			    rootpath != NULL ? rootpath : root);

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

struct vfsref *
vfs_lookup(const char *filename, const char *name, const char *basepath,
    int strict)
{
	char *fn;
	struct vfsent *ve;
	struct vfsref *vr;
	struct stat fs;
	unsigned int i;
	int pseudo = 0;

	fn = vfs_path_concat(basepath, filename, strict);
	if (fn != NULL && (vr = vfs_cache_lookup(fn)) != NULL)
		return (vr);

	/* We only allow files and directories */
	if (fn == NULL || stat(fn, &fs) != 0) {
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
	ve->recurse = 1;
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
		/* Only allow pseudo-filenames when module supports it */
		if (pseudo && !modules[i].pseudo)
			continue;

		/* Try to attach the module */
		ve->vmod = &modules[i];
		if (ve->vmod->match(ve, S_ISDIR(fs.st_mode)) == 0)
			goto found;
	}

	/* No matching VFS module */
	vfs_dealloc(ve);
	return (NULL);

found:
	/* Return reference object */
	ve->refcount = 1;
	vr = g_slice_new0(struct vfsref);
	vr->ent = ve;
	if (!pseudo)
		vfs_cache_add(vr);
	return (vr);
}

struct vfsref *
vfs_dup(const struct vfsref *vr)
{
	struct vfsent *ve;
	struct vfsref *rvr;

	ve = vr->ent;
	g_atomic_int_inc(&ve->refcount);

	rvr = g_slice_new(struct vfsref);
	rvr->ent = ve;
	rvr->marked = 0;

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

	return vr->ent->vmod->populate(vr->ent);
}

void
vfs_unfold(struct vfslist *vl, const struct vfsref *vr)
{
	struct vfsref *cvr;

	if (vfs_playable(vr)) {
		/* Single item - add it to the list */
		vfs_list_insert_tail(vl, vfs_dup(vr));
	} else {
		/* See if we can recurse it */
		vfs_populate(vr);
		VFS_LIST_FOREACH(&vr->ent->population, cvr) {
			if (cvr->ent->recurse)
				vfs_unfold(vl, cvr);
		}
	}
}

void
vfs_locate(struct vfslist *vl, const struct vfsref *vr,
    const struct vfsmatch *vm)
{
	struct vfsref *cvr;

	vfs_populate(vr);
	VFS_LIST_FOREACH(&vr->ent->population, cvr) {
		/* Add matching objects to the results */
		if (vfs_playable(cvr) &&
		    vfs_match_compare(vm, vfs_filename(cvr)))
			vfs_list_insert_tail(vl, vfs_dup(cvr));
		/* Also search through its children */
		if (cvr->ent->recurse)
			vfs_locate(vl, cvr, vm);
	}
}

struct vfsref *
vfs_write_playlist(const struct vfslist *vl, const struct vfsref *vr,
    const char *filename)
{
	const char *base = NULL;
	char *fn, *nfn;
	struct vfsref *rvr = NULL;
	struct vfswriter *wr;
	unsigned int i;

	if (vr != NULL)
		base = vfs_filename(vr);
	fn = vfs_path_concat(base, filename, 0);
	if (fn == NULL)
		return (NULL);

	/* Search for a matching extension */
	for (i = 0; i < NUM_WRITERS; i++) {
		if (g_str_has_suffix(fn, writers[i].ext)) {
			wr = &writers[i];
			goto match;
		}
	}

	/* No extension matched, use default format */
	wr = &writers[0];
	nfn = g_strdup_printf("%s%s", fn, wr->ext);
	g_free(fn);
	fn = nfn;
match:
	/* Write the playlist to disk */
	if (wr->write(vl, fn) == 0)
		rvr = vfs_lookup(fn, NULL, NULL, 0);

	g_free(fn);
	return (rvr);
}

int
vfs_delete(const char *filename)
{
	char *fn;
	int ret;

	fn = vfs_path_concat(NULL, filename, 0);
	if (fn == NULL)
		return (-1);

	ret = unlink(fn);
	g_free(fn);

	return (ret);
}

FILE *
vfs_fopen(const char *filename, const char *mode)
{
	char *fn;
	FILE *ret;

	fn = vfs_path_concat(NULL, filename, 0);
	if (fn == NULL)
		return (NULL);

	ret = fopen(fn, mode);
	g_free(fn);

	return (ret);
}

int
vfs_fgets(char *str, size_t size, FILE *fp)
{
	char *eol;

	if (fgets(str, size, fp) == NULL)
		return (-1);

	eol = strchr(str, '\0');
	g_assert(eol != NULL);

	/* Strip the final \n */
	if (--eol >= str && *eol == '\n') {
		*eol = '\0';
		/* Strip the final \r */
		if (--eol >= str && *eol == '\r')
			*eol = '\0';
	}

	return (0);
}

struct vfsmatch *
vfs_match_new(const char *str)
{
	struct vfsmatch *vm;

	vm = g_slice_new0(struct vfsmatch);

	if (regcomp(&vm->regex, str, REG_EXTENDED|REG_ICASE) != 0) {
		g_slice_free(struct vfsmatch, vm);
		return (NULL);
	}
	vm->string = g_strdup(str);

	return (vm);
}

void
vfs_match_free(struct vfsmatch *vm)
{
	regfree(&vm->regex);
	g_free(vm->string);
	g_slice_free(struct vfsmatch, vm);
}
