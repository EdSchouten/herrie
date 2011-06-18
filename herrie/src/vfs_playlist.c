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
 * @file vfs_playlist.c
 * @brief M3U and PLS playlist file access.
 */

#include "stdinc.h"

#include "vfs.h"
#include "vfs_modules.h"

/**
 * @brief Open a VFS entry by relative Win32 pathname and add it to the
 *        tail of the current entry.
 */
static void
vfs_playlist_add_tail(struct vfsent *ve, char *fn, char *title, char *dirname)
{
	struct vfsref *nvr;

#if G_DIR_SEPARATOR != '\\'
	/* Convert to proper separator */
	g_strdelimit(fn, "\\", G_DIR_SEPARATOR);
#endif
	nvr = vfs_lookup(fn, title, dirname, 0);

	if (nvr != NULL)
		vfs_list_insert_tail(&ve->population, nvr);
}

/*
 * PLS interface
 */

int
vfs_pls_match(struct vfsent *ve, int isdir)
{
	/* In order to speed up the process, we only match *.pls */
	if (isdir || !g_str_has_suffix(ve->name, ".pls"))
		return (-1);

	ve->recurse = 0;
	return (0);
}

int
vfs_pls_populate(struct vfsent *ve)
{
	FILE *fio;
	char fbuf[1024];
	char *ch, *dn, **out;
	int idxoff, nidx;

	/* Current item */
	int idx = -1;
	char *fn = NULL;
	char *title = NULL;

	if ((fio = fopen(ve->filename, "r")) == NULL)
		return (-1);
	dn = g_path_get_dirname(ve->filename);

	while (vfs_fgets(fbuf, sizeof fbuf, fio) == 0) {
		if (strncmp(fbuf, "File", 4) == 0) {
			/* Filename */
			out = &fn;
			idxoff = 4;
		} else if (strncmp(fbuf, "Title", 5) == 0) {
			/* Song title */
			out = &title;
			idxoff = 5;
		} else {
			continue;
		}

		/* Parse the index number */
		if ((nidx = atoi(fbuf + idxoff)) == 0)
			continue;

		/* See if we have a value */
		ch = strchr(fbuf + idxoff, '=');
		if ((ch == NULL) || (ch[1] == '\0'))
			continue;

		if (nidx != idx) {
			/* We've arrived at the next entry */
			if (fn != NULL)
				vfs_playlist_add_tail(ve, fn, title, dn);

			g_free(fn); fn = NULL;
			g_free(title); title = NULL;
			idx = nidx;
		}

		/* Copy the new value */
		g_free(*out);
		*out = g_strdup(ch + 1);
	}

	/* Don't forget our trailing entry */
	if (fn != NULL)
		vfs_playlist_add_tail(ve, fn, title, dn);

	g_free(fn);
	g_free(title);

	fclose(fio);
	g_free(dn);

	return (0);
}

int
vfs_pls_write(const struct vfslist *vl, const char *filename)
{
	const char *base = NULL;
	size_t cmplen;
	FILE *fio;
	struct vfsref *vr;
	unsigned int idx = 1;

	fio = fopen(filename, "w");
	if (fio == NULL)
		return (-1);

	/* Directory name length of .pls filename */
	base = strrchr(filename, G_DIR_SEPARATOR);
	g_assert(base != NULL);
	cmplen = base - filename + 1;

	fprintf(fio, "[playlist]\nNumberOfEntries=%u\n",
	    vfs_list_items(vl));
	VFS_LIST_FOREACH(vl, vr) {
		/* Skip directory name when relative is possible */
		base = vfs_filename(vr);
		if (strncmp(filename, base, cmplen) == 0)
			base += cmplen;

		fprintf(fio, "File%u=%s\nTitle%u=%s\n",
		    idx, base,
		    idx, vfs_name(vr));
		idx++;
	}
	fclose(fio);

	return (0);
}

/*
 * M3U interface
 */

int
vfs_m3u_match(struct vfsent *ve, int isdir)
{
	/* In order to speed up the process, we only match *.m3u */
	if (isdir || !g_str_has_suffix(ve->name, ".m3u"))
		return (-1);

	ve->recurse = 0;
	return (0);
}

int
vfs_m3u_populate(struct vfsent *ve)
{
	FILE *fio;
	char fbuf[1024];
	char *ch, *dn;
	char *title = NULL;

	if ((fio = fopen(ve->filename, "r")) == NULL)
		return (-1);
	dn = g_path_get_dirname(ve->filename);

	while (vfs_fgets(fbuf, sizeof fbuf, fio) == 0) {
		if (fbuf[0] == '#') {
			/* Only EXTINF is supported */
			if (strncmp(fbuf, "#EXTINF:", 8) == 0) {
				/* Consolidate double lines */
				g_free(title); title = NULL;

				/* Remove the duration in seconds */
				if (((ch = strchr(fbuf + 8, ',')) != NULL)
				    && (ch[1] != '\0'))
					title = g_strdup(ch + 1);
			}
		} else if (fbuf[0] != '\0') {
			vfs_playlist_add_tail(ve, fbuf, title, dn);
			g_free(title); title = NULL;
		}
	}

	/* Trailing EXTINF */
	g_free(title);

	fclose(fio);
	g_free(dn);

	return (0);
}

int
vfs_m3u_write(const struct vfslist *vl, const char *filename)
{
	const char *base = NULL;
	size_t cmplen;
	FILE *fio;
	struct vfsref *vr;

	fio = fopen(filename, "w");
	if (fio == NULL)
		return (-1);

	/* Directory name length of .meu filename */
	base = strrchr(filename, G_DIR_SEPARATOR);
	g_assert(base != NULL);
	cmplen = base - filename + 1;

	fprintf(fio, "#EXTM3U\n");
	VFS_LIST_FOREACH(vl, vr) {
		/* Skip directory name when relative is possible */
		base = vfs_filename(vr);
		if (strncmp(filename, base, cmplen) == 0)
			base += cmplen;

		fprintf(fio, "#EXTINF:-1,%s\n%s\n", vfs_name(vr), base);
	}
	fclose(fio);

	return (0);
}
