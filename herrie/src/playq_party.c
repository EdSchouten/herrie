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
 * @file playq_party.c
 * @brief Party-mode playlist handling.
 */

#include "stdinc.h"

#include "gui.h"
#include "playq.h"
#include "playq_modules.h"
#include "vfs.h"

struct vfsref *
playq_party_give(void)
{
	struct vfsref *vr, *nvr;

	/* Fetch the first song in the list */
	vr = vfs_list_first(&playq_list);
	if (vr == NULL)
		return (NULL);

	/* Remove it from the list */
	nvr = vfs_dup(vr);
	gui_playq_notify_pre_removal(1);
	vfs_list_remove(&playq_list, vr);
	if (playq_repeat) {
		/* Add it back to the tail */
		vfs_list_insert_tail(&playq_list, vr);
		gui_playq_notify_post_insertion(vfs_list_items(&playq_list));
	} else {
		vfs_close(vr);
	}
	gui_playq_notify_done();

	return (nvr);
}

void
playq_party_idle(void)
{
}

int
playq_party_select(struct vfsref *vr)
{
	return (0);
}

int
playq_party_next(void)
{
	return (0);
}

int
playq_party_prev(void)
{
	return (-1);
}

void
playq_party_notify_pre_removal(struct vfsref *vr)
{
}
