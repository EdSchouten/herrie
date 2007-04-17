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

/* Teach glibc a little lesson */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* !_GNU_SOURCE */
#undef _FORTIFY_SOURCE

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <langinfo.h>
#ifdef BUILD_TRANS
#include <locale.h>
#endif /* BUILD_TRANS */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>
#ifdef BUILD_TRANS
#include <glib/gi18n.h>
#else /* !BUILD_TRANS */
#define _(str) str
#endif /* BUILD_TRANS */

/* Older Glib's don't have GSlice yet */
#ifndef g_slice_new
#define g_slice_new(type)	g_malloc(sizeof(type))
#define g_slice_new0(type)	g_malloc0(sizeof(type))
#define g_slice_free(type,obj)	g_free(obj)
#endif /* !g_slice_new */

#ifdef G_THREADS_IMPL_POSIX
#include <pthread.h>
#endif /* G_THREADS_IMPL_POSIX */
#ifdef G_OS_UNIX
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <pwd.h>
#ifdef BUILD_RES_INIT
#include <resolv.h>
#endif /* BUILD_RES_INIT */
#endif /* G_OS_UNIX */

#ifndef BUILD_FTELLO
#define ftello(stream)		ftell(stream)
#endif /* !BUILD_FTELLO */
