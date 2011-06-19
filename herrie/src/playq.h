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
 * @file playq.h
 * @brief Playlist handling.
 */

struct vfsref;

/**
 * @brief Initialize the playlist locking.
 */
void playq_init(int autoplay, int xmms, int load_dumpfile);
/**
 * @brief Spawn the playback thread.
 */
void playq_spawn(void);
/**
 * @brief Shutdown the playback thread.
 */
void playq_shutdown(void);

/**
 * @brief Seek the current played song.
 */
void playq_cursong_seek(int len, int rel);
/**
 * @brief Continue to the next song.
 */
void playq_cursong_next(void);
/**
 * @brief Go back to the previous song.
 */
void playq_cursong_prev(void);
/**
 * @brief Stop playback.
 */
void playq_cursong_stop(void);
/**
 * @brief Pause or unpause the current song, depending on its current
 *        state.
 */
void playq_cursong_pause(void);
/**
 * @brief Toggle whether repeat is turned on or off.
 */
void playq_repeat_toggle(void);

/**
 * @brief Lock queue and add a file or directory to the head of the
 *        playlist.
 */
void playq_song_add_head(struct vfsref *vr);
/**
 * @brief Lock queue and add a file or directory to the tail of the
 *        playlist.
 */
void playq_song_add_tail(struct vfsref *vr);
/**
 * @brief Remove all songs from the playlist.
 */
void playq_song_remove_all(void);
/**
 * @brief Randomize the playlist.
 */
void playq_song_randomize(void);

/**
 * @brief The mutex that locks down the playlist.
 */
extern GMutex		*playq_mtx;

/**
 * @brief Acquire the lock on the playlist.
 */
static inline void
playq_lock(void)
{
	g_mutex_lock(playq_mtx);
}

/**
 * @brief Release the lock on the playlist.
 */
static inline void
playq_unlock(void)
{
	g_mutex_unlock(playq_mtx);
}

/*
 * All functions and variables below need proper locking. Always use
 * playq_lock() and playq_unlock() when using them.
 */

/**
 * @brief The actual playlist containing all songs scheduled for playback.
 */
extern struct vfslist playq_list;

/**
 * @brief Remove the song with its corresponding index.
 */
void playq_song_fast_remove(struct vfsref *vr, unsigned int index);
/**
 * @brief Add a song before the specified song.
 */
void playq_song_fast_add_before(struct vfsref *nvr, struct vfsref *lvr,
    unsigned int index);
/**
 * @brief Add a song after the specified song.
 */
void playq_song_fast_add_after(struct vfsref *nvr, struct vfsref *lvr,
    unsigned int index);
/**
 * @brief Move the specified song one position up.
 */
void playq_song_fast_move_up(struct vfsref *vr, unsigned int index);
/**
 * @brief Move the specified song one position down.
 */
void playq_song_fast_move_down(struct vfsref *vr, unsigned int index);
/**
 * @brief Move the specified song to the top.
 */
void playq_song_fast_move_head(struct vfsref *vr, unsigned int index);
/**
 * @brief Move the specified song to the bottom.
 */
void playq_song_fast_move_tail(struct vfsref *vr, unsigned int index);
/**
 * @brief Switch playback to a specific song.
 */
void playq_song_fast_select(struct vfsref *vr);
