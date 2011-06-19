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
 * @file playq_xmms.c
 * @brief XMMS-style playlist handling.
 */

#include "stdinc.h"

#include "gui.h"
#include "playq.h"
#include "playq_modules.h"
#include "vfs.h"

/**
 * @brief Currently played song.
 */
static struct vfsref *cursong = NULL;
/**
 * @brief Used-specified song that should be played next.
 */
static struct vfsref *selectsong = NULL;

struct vfsref *
playq_xmms_give(void)
{
	struct vfsref *vr = NULL;

	if (cursong != NULL)
		/* Unmark it */
		vfs_unmark(cursong);

	if (selectsong != NULL) {
		/* Selected song has the highest priority */
		cursong = selectsong;
		selectsong = NULL;
	} else if (cursong != NULL) {
		/* Song after current song */
		cursong = vfs_list_next(cursong);
		/* We've reached the end */
		if (cursong == NULL && playq_repeat)
			cursong = vfs_list_first(&playq_list);
	} else {
		cursong = vfs_list_first(&playq_list);
	}

	if (cursong != NULL) {
		vfs_mark(cursong);
		vr = vfs_dup(cursong);
	}

	/* Update markers */
	gui_playq_notify_done();

	return (vr);
}

void
playq_xmms_idle(void)
{
	if (cursong != NULL) {
		/* Remove left-over marking */
		vfs_unmark(cursong);
		gui_playq_notify_done();
	}
}

int
playq_xmms_select(struct vfsref *vr)
{
	selectsong = vr;

	return (0);
}

int
playq_xmms_next(void)
{
	if (cursong != NULL) {
		selectsong = vfs_list_next(cursong);
		if (selectsong == NULL)
			selectsong = vfs_list_first(&playq_list);
	}

	return (selectsong == NULL);
}

int
playq_xmms_prev(void)
{
	if (cursong != NULL) {
		selectsong = vfs_list_prev(cursong);
		if (selectsong == NULL)
			selectsong = vfs_list_last(&playq_list);
	}

	return (selectsong == NULL);
}

void
playq_xmms_notify_pre_removal(struct vfsref *vr)
{
	/* Remove dangling pointers */
	if (cursong == vr)
		cursong = NULL;

	g_assert(selectsong != vr);
}
