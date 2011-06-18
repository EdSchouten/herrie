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
 * @file vfs_xspf.c
 * @brief XSPF file access.
 */

#include "stdinc.h"

#include <xspf_c.h>

#include "util.h"
#include "vfs.h"
#include "vfs_modules.h"

int
vfs_xspf_match(struct vfsent *ve, int isdir)
{
	/* In order to speed up the process, we only match *.xspf */
	if (isdir || !g_str_has_suffix(ve->name, ".xspf"))
		return (-1);

	ve->recurse = 0;
	return (0);
}

int
vfs_xspf_populate(struct vfsent *ve)
{
	struct xspf_list *slist;
	struct xspf_track *strack;
	struct xspf_mvalue *sloc;
	char *dirname, *baseuri, *filename;
	struct vfsref *vr;

	baseuri = url_escape(ve->filename);
	slist = xspf_parse(ve->filename, baseuri);
	g_free(baseuri);
	if (slist == NULL)
		return (-1);

	dirname = g_path_get_dirname(ve->filename);

	XSPF_LIST_FOREACH_TRACK(slist, strack) {
		XSPF_TRACK_FOREACH_LOCATION(strack, sloc) {
			/* Skip file:// part */
			filename = url_unescape(sloc->value);

			/* Add it to the list */
			vr = vfs_lookup(filename, strack->title, dirname, 1);
			if (vr != NULL)
				vfs_list_insert_tail(&ve->population, vr);
		}
	}

	g_free(dirname);
	xspf_free(slist);
	return (0);
}

int
vfs_xspf_write(const struct vfslist *vl, const char *filename)
{
	struct xspf_list *list;
	struct xspf_track *track;
	struct xspf_mvalue *location;
	char *fn, *baseuri;
	struct vfsref *vr;
	int ret;

	list = xspf_new();

	VFS_LIST_FOREACH_REVERSE(vl, vr) {
		/* Add a new track to the beginning of the list */
		track = xspf_new_track_before(&list->tracks);

		/* Make sure we don't write non-UTF-8 titles to disk */
		if (g_utf8_validate(vfs_name(vr), -1, NULL))
			xspf_setvalue(&track->title, vfs_name(vr));

		location = xspf_new_mvalue_before(&track->locations);
		fn = url_escape(vfs_filename(vr));
		xspf_setvalue(&location->value, fn);
		g_free(fn);
	}

	baseuri = url_escape(filename);
	ret = xspf_write(list, filename, baseuri);
	g_free(baseuri);
	xspf_free(list);

	return (ret);
}
