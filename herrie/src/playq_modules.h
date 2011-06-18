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
 * @file playq_modules.h
 * @brief Custom playlist behaviour routines.
 */

struct vfsref;

/**
 * @brief Flag whether repeat is turned on by the user.
 */
extern int playq_repeat;

/**
 * @brief Herrie's routine to fetch the next song from the playlist
 *        (always the first song).
 */
struct vfsref *playq_party_give(void);
/**
 * @brief Herrie's idle indication function.
 */
void playq_party_idle(void);
/**
 * @brief Herrie's song selection routine (bogus with Herrie, because we
 *        always start the first song).
 */
int playq_party_select(struct vfsref *vr);
/**
 * @brief Herrie's routine to switch to the next function.
 */
int playq_party_next(void);
/**
 * @brief Herrie's routine to switch to the previous function.
 */
int playq_party_prev(void);
/**
 * @brief Herrie's notification that a song is about to be deleted.
 */
void playq_party_notify_pre_removal(struct vfsref *vr);

/**
 * @brief XMMS-like function that retreives the next song from the
 *        playlist.
 */
struct vfsref *playq_xmms_give(void);
/**
 * @brief XMMS-like function to notify that playback is going idle.
 */
void playq_xmms_idle(void);
/**
 * @brief XMMS-like function to start playback of a specific song.
 */
int playq_xmms_select(struct vfsref *vr);
/**
 * @brief XMMS-like function that switches playback to the next song.
 */
int playq_xmms_next(void);
/**
 * @brief XMMS-like function that switches playback to the previous song.
 */
int playq_xmms_prev(void);
/**
 * @brief XMMS-like notification that a song is about to be deleted.
 */
void playq_xmms_notify_pre_removal(struct vfsref *vr);
