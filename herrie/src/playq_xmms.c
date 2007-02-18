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
 * @file playq_xmms.c
 */

#include "gui.h"
#include "playq_modules.h"

static struct vfsref *cursong = NULL, *nextsong = NULL;

struct vfsref *
playq_xmms_givenext(void)
{
	struct vfsref *vr = NULL;

	if (cursong != NULL)
		vfs_unmark(cursong);

	/* Move on to the next track */
	cursong = nextsong;
	if (cursong != NULL)
		nextsong = vfs_list_next(cursong);

	if (cursong != NULL) {
		vfs_mark(cursong);
		vr = vfs_dup(cursong);
	}

	/* Update markers */
	gui_playq_notify_done();

	return (vr);
}

void
playq_xmms_select(struct vfsref *vr)
{
	nextsong = vr;
}

void
playq_xmms_notify_pre_removal(struct vfsref *vr)
{
	if (cursong == vr)
		cursong = NULL;
	if (nextsong == vr)
		nextsong = vfs_list_next(nextsong);
}
