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
 * @file playq.c
 */

#include "audio_file.h"
#include "audio_output.h"
#include "config.h"
#include "gui.h"
#include "playq.h"
#include "vfs.h"

struct vfslist		playq_list = VFSLIST_INITIALIZER;
GMutex 			*playq_lock;
/**
 * @brief Conditional variable used to kick the playq alive when it was
 *        waiting for a new song to be added to the playlist or when
 *        paused.
 */
static GCond		*playq_wakeup;
/**
 * @brief Reference to playback thread.
 */
static GThread		*playq_runner;

/**
 * @brief Order the playback thread to pause itself.
 */
static volatile int	do_pause;
/**
 * @brief Order the playback thread to skip the current song.
 */
static volatile int	do_skip;
/**
 * @brief Order the playback thread to shut itself down.
 */
static volatile int	do_quit;
/**
 * @brief Amount of seconds which the current song should seek.
 */
static volatile int	do_seek_pos;
/**
 * @brief Flag to determine a positive or relative seek will be called.
 */
static volatile int	do_seek_rel;

/**
 * @brief Infinitely play music in the playlist, honouring the do_
 *        flags.
 */
static void *
playq_runner_thread(void *unused)
{
	struct vfsref		*nvr;
	struct audio_file	*cur;
	char			*errmsg;
	int			repeat;

	gui_input_sigmask();

	repeat = config_getopt_bool("playq.repeat");

	do {
		/* Wait until there's a song available */
		PLAYQ_LOCK;
		while ((nvr = vfs_list_first(&playq_list)) == NULL) {
			/* Change the current status to idle */
			gui_playq_song_update(NULL, 0);

			g_cond_wait(playq_wakeup, playq_lock);
			if (do_quit) {
				PLAYQ_UNLOCK;
				goto done;
			}
		}
		gui_playq_notify_pre_removal(1);
		vfs_list_remove(&playq_list, nvr);
		gui_playq_notify_done();
		PLAYQ_UNLOCK;

		cur = audio_file_open(nvr);
		if (cur == NULL) {
			/* Skip broken songs */
			errmsg = g_strdup_printf(
			    _("Failed to open \"%s\" for playback."),
			    vfs_name(nvr));
			gui_msgbar_warn(errmsg);
			g_free(errmsg);
		} else if (repeat) {
			/* Place it back */
			playq_song_add_tail(nvr);
		}
		vfs_close(nvr);
		if (cur == NULL) {
			/* Try next item */
			continue;
		}

		gui_playq_song_update(cur, 0);

		do_pause = do_skip = do_seek_pos = 0;
		do_seek_rel = 1;

		do  {
			if (!cur->stream && do_pause) {
				/*
				 * XXX: We must specify a mutex when
				 * waiting for a conditional variable.
				 */
				PLAYQ_LOCK;
				while (do_pause && !do_quit)
					g_cond_wait(playq_wakeup, playq_lock);
				PLAYQ_UNLOCK;
			}

			if (do_seek_pos != 0 || do_seek_rel == 0) {
				audio_file_seek(cur, do_seek_pos, do_seek_rel);
				do_seek_pos = 0;
				do_seek_rel = 1;
			}

			gui_playq_song_update(cur, 1);
		} while (!do_quit && !do_skip && audio_output_play(cur) > 0);

		audio_file_close(cur);
	} while (!do_quit);
done:
	return (NULL);
}

void
playq_init(void)
{
	playq_lock = g_mutex_new();
	playq_wakeup = g_cond_new();
}

void
playq_spawn(void)
{
	do_quit = 0;
	playq_runner = g_thread_create_full(playq_runner_thread, NULL,
	    0, 1, TRUE, G_THREAD_PRIORITY_URGENT, NULL);
}

void
playq_shutdown(void)
{
	do_quit = 1;
	g_cond_signal(playq_wakeup);
	g_thread_join(playq_runner);
}

/*
 * Public queue functions: playq_song_*
 */

void
playq_song_add_head(struct vfsref *vr)
{
	/* Recursively expand the item */
	struct vfslist newlist = VFSLIST_INITIALIZER;
	vfs_unfold(&newlist, vr);

	PLAYQ_LOCK;
	/* Copy the expanded contents to the playlist */
	while ((vr = vfs_list_last(&newlist)) != NULL) {
		vfs_list_remove(&newlist, vr);
		vfs_list_insert_head(&playq_list, vr);
		gui_playq_notify_post_insertion(1);
	}

	gui_playq_notify_done();
	g_cond_signal(playq_wakeup);
	PLAYQ_UNLOCK;
}

void
playq_song_add_tail(struct vfsref *vr)
{
	/* Recursively expand the item */
	struct vfslist newlist = VFSLIST_INITIALIZER;
	vfs_unfold(&newlist, vr);

	PLAYQ_LOCK;
	/* Copy the expanded contents to the playlist */
	while ((vr = vfs_list_first(&newlist)) != NULL) {
		vfs_list_remove(&newlist, vr);
		vfs_list_insert_tail(&playq_list, vr);
		gui_playq_notify_post_insertion(vfs_list_items(&playq_list));
	}

	gui_playq_notify_done();
	g_cond_signal(playq_wakeup);
	PLAYQ_UNLOCK;
}

/**
 * @brief Seek the current song by a certain amount of time.
 */
void
playq_cursong_seek(int len, int rel)
{
	do_seek_pos = len;
	do_seek_rel = rel;
	/* We could be paused */
	do_pause = 0;
	g_cond_signal(playq_wakeup);
}

void
playq_cursong_skip(void)
{
	do_skip = 1;
	/* We could be paused */
	do_pause = 0;
	g_cond_signal(playq_wakeup);
}

void
playq_cursong_pause(void)
{
	if (do_pause) {
		/* Kick it back alive */
		do_pause = 0;
		g_cond_signal(playq_wakeup);
	} else {
		/* Pause on next read */
		do_pause = 1;
	}
}

void
playq_song_fast_remove(struct vfsref *vr, unsigned int index)
{
	gui_playq_notify_pre_removal(index);
	vfs_list_remove(&playq_list, vr);
	vfs_close(vr);
	gui_playq_notify_done();
}

void
playq_song_fast_add_before(struct vfsref *nvr, struct vfsref *lvr,
    unsigned int index)
{
	/* Recursively expand the item */
	struct vfslist newlist = VFSLIST_INITIALIZER;
	vfs_unfold(&newlist, nvr);

	/* Copy the expanded contents to the playlist */
	while ((nvr = vfs_list_first(&newlist)) != NULL) {
		vfs_list_remove(&newlist, nvr);
		vfs_list_insert_before(&playq_list, nvr, lvr);
		gui_playq_notify_post_insertion(index);
	}

	gui_playq_notify_done();
	g_cond_signal(playq_wakeup);
}

void
playq_song_fast_add_after(struct vfsref *nvr, struct vfsref *lvr,
    unsigned int index)
{
	/* Recursively expand the item */
	struct vfslist newlist = VFSLIST_INITIALIZER;
	vfs_unfold(&newlist, nvr);

	/* Copy the expanded contents to the playlist */
	while ((nvr = vfs_list_last(&newlist)) != NULL) {
		vfs_list_remove(&newlist, nvr);
		vfs_list_insert_after(&playq_list, nvr, lvr);
		gui_playq_notify_post_insertion(index + 1);
	}

	gui_playq_notify_done();
	g_cond_signal(playq_wakeup);
}

void
playq_song_fast_moveup(struct vfsref *vr, unsigned int index)
{
	struct vfsref *pvr;
	
	/* Remove the item above */
	pvr = vfs_list_prev(vr);
	gui_playq_notify_pre_removal(index - 1);
	vfs_list_remove(&playq_list, pvr);

	/* Add it below */
	vfs_list_insert_after(&playq_list, pvr, vr);
	gui_playq_notify_post_insertion(index);
	gui_playq_notify_done();
}

void
playq_song_fast_movedown(struct vfsref *vr, unsigned int index)
{
	struct vfsref *nvr;
	
	/* Remove the item below */
	nvr = vfs_list_next(vr);
	gui_playq_notify_pre_removal(index + 1);
	vfs_list_remove(&playq_list, nvr);

	/* Add it above */
	vfs_list_insert_before(&playq_list, nvr, vr);
	gui_playq_notify_post_insertion(index);
	gui_playq_notify_done();
}

void
playq_song_remove_all(void)
{
	struct vfsref *vr;

	PLAYQ_LOCK;
	while ((vr = vfs_list_first(&playq_list)) != NULL) {
		gui_playq_notify_pre_removal(1);
		vfs_list_remove(&playq_list, vr);
		vfs_close(vr);
	}
	gui_playq_notify_done();
	PLAYQ_UNLOCK;
}

void
playq_song_randomize(void)
{
	unsigned int remaining, idx = 0;
	struct vfsref *vr, **vrlist;

	/*
	 * This code implements the Fisher-Yates algorithm to randomize
	 * the playlist. Only difference is that we already place items
	 * back in the list instead of actually swapping them.
	 */

	PLAYQ_LOCK;
	remaining = vfs_list_items(&playq_list);
	if (remaining < 2)
		goto done;

	/* Generate a shadow list */
	vrlist = g_new(struct vfsref *, remaining);
	vfs_list_foreach(&playq_list, vr)
		vrlist[idx++] = vr;
	vfs_list_init(&playq_list);

	for (; remaining != 0; remaining--) {
		/* Pick a random item from the beginning */
		idx = g_random_int_range(0, remaining);
		
		/* Add it to the list */
		vfs_list_insert_tail(&playq_list, vrlist[idx]);
		/* Remove fragmentation  */
		vrlist[idx] = vrlist[remaining - 1];
	}

	/* Trash the shadow list */
	g_free(vrlist);

	gui_playq_notify_post_randomization();
	gui_playq_notify_done();
done:	PLAYQ_UNLOCK;
}
