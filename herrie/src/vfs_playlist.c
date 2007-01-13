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
 * @file vfs_playlist.c
 */

#include "vfs_modules.h"

/**
 * @brief Check if the extension of the given filename matches.
 */
static int
vfs_playlist_check_ext(char *name, char *ext)
{
	size_t fnlen, extlen;

	fnlen = strlen(name);
	extlen = strlen(ext);
	if (fnlen < extlen)
		return (-1);
	
	return strcasecmp(ext, &name[fnlen - extlen]);
}

/**
 * @brief Open a VFS entry by relative Win32 pathname and add it to the
 *        tail of the current entry.
 */
static int
vfs_playlist_add_tail(struct vfsent *ve, char *fn, char *title, char *dirname)
{
	struct vfsref *nvr;
#if G_DIR_SEPARATOR != '\\'
	char *c;
	
	/* Convert to proper separator */
	for (c = fn; *c != '\0'; c++) {
		if (*c == '\\')
			*c = G_DIR_SEPARATOR;
	}
#endif
	nvr = vfs_open(fn, title, dirname);

	if (nvr != NULL) {
		vfs_list_insert_tail(&ve->population, nvr);
		return (0);
	} else {
		return (1);
	}
}

/*
 * PLS interface
 */

int
vfs_pls_open(struct vfsent *ve, int isdir)
{
	/* In order to speed up the process, we only match *.pls */
	if (isdir || vfs_playlist_check_ext(ve->name, ".pls") != 0)
		return (-1);

	return (0);
}

int
vfs_pls_populate(struct vfsent *ve)
{
	GIOChannel *fio;
	GString *fln;
	gsize eol;
	char *ch, *dn, **out;
	int idxoff, nidx;

	/* Current item */
	int idx = -1;
	char *fn = NULL;
	char *title = NULL;

	if ((fio = g_io_channel_new_file(ve->filename, "r", NULL)) == NULL)
		return (-1);
	fln = g_string_sized_new(64);
	dn = g_path_get_dirname(ve->filename);

	while (g_io_channel_read_line_string(fio, fln, &eol, NULL)
	    == G_IO_STATUS_NORMAL) {
		g_string_truncate(fln, eol);

		if (strncmp(fln->str, "File", 4) == 0) {
			/* Filename */
			out = &fn;
			idxoff = 4;
		} else if (strncmp(fln->str, "Title", 5) == 0) {
			/* Song title */
			out = &title;
			idxoff = 5;
		} else {
			continue;
		}

		/* Parse the index number */
		if ((nidx = atoi(fln->str + idxoff)) == 0)
			continue;

		/* See if we have a value */
		ch = strchr(fln->str + idxoff, '=');
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

	g_io_channel_unref(fio);
	g_string_free(fln, TRUE);
	g_free(dn);
	
	return (0);
}

/*
 * M3U interface
 */

int
vfs_m3u_open(struct vfsent *ve, int isdir)
{
	/* In order to speed up the process, we only match *.m3u */
	if (isdir || vfs_playlist_check_ext(ve->name, ".m3u") != 0)
		return (-1);

	return (0);
}

int
vfs_m3u_populate(struct vfsent *ve)
{
	GIOChannel *fio;
	GString *fln;
	gsize eol;
	char *ch, *dn;
	char *title = NULL;

	if ((fio = g_io_channel_new_file(ve->filename, "r", NULL)) == NULL)
		return (-1);
	fln = g_string_sized_new(64);
	dn = g_path_get_dirname(ve->filename);

	while (g_io_channel_read_line_string(fio, fln, &eol, NULL)
	    == G_IO_STATUS_NORMAL) {
		g_string_truncate(fln, eol);
		if (fln->str[0] == '#') {
			/* Only EXTINF is supported */
			if (strncmp(fln->str, "#EXTINF:", 8) == 0) {
				/* Consolidate double lines */
				g_free(title); title = NULL;

				/* Remove the duration in seconds */
				if (((ch = strchr(fln->str + 8, ',')) != NULL)
				    && (ch[1] != '\0'))
					title = g_strdup(ch + 1);
			}
		} else if (fln->str[0] != '\0') {
			vfs_playlist_add_tail(ve, fln->str, title, dn);
			g_free(title); title = NULL;
		}
	}

	/* Trailing EXTINF */
	g_free(title);

	g_io_channel_unref(fio);
	g_string_free(fln, TRUE);
	g_free(dn);

	return (0);
}
