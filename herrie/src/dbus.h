/*
 * Copyright (c) 2006-2011 Ed Schouten <ed@80386.nl>
 * All rights reserved.
 *
 * Copyright (c) 2008 Steve Jothen <sjothen@gmail.com>
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
 * @file dbus.h
 * @brief DBus integration.
 */

#ifdef BUILD_DBUS
/**
 * @brief Initialize DBus locking.
 */
void dbus_init(void);

/**
 * @brief DBus mutex to provide exclusion between the user input and
 *        DBus event paths.
 */
extern GMutex *dbus_mtx;

/**
 * @brief Called before entering the gmainloop
 */
void dbus_before_mainloop(void);

/**
 * @brief Called after exiting the gmainloop
 */
void dbus_after_mainloop(void);

/**
 * @brief Acquire a lock on DBus.
 */
static inline void
dbus_lock(void)
{
	g_mutex_lock(dbus_mtx);
}

/**
 * @brief Release a lock on DBus.
 */
static inline void
dbus_unlock(void)
{
	g_mutex_unlock(dbus_mtx);
}
#endif /* BUILD_DBUS */
