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
 * @file vfs_modules.h
 */

#ifndef _VFS_MODULES_H_
#define _VFS_MODULES_H_

#include "vfs.h"

/**
 * @brief Open a directory.
 */
int	vfs_dir_open(struct vfsent *ve, int isdir);
/**
 * @brief Read the contents of a directory and add them to the
 *        population of the current node. This function honours the sort
 *        order each module has.
 */
int	vfs_dir_populate(struct vfsent *ve);

/**
 * @brief A fallback module that matches all files on disk (possibly
 *        audio files?)
 */
int	vfs_file_open(struct vfsent *ve, int isdir);

/**
 * @brief Test whether the current node is an M3U file.
 */
int	vfs_m3u_open(struct vfsent *ve, int isdir);
/**
 * @brief Add all items in the M3U file to the population of the current
 *        node.
 */
int	vfs_m3u_populate(struct vfsent *ve);

/**
 * @brief Test whether the current node is an PLS file.
 */
int	vfs_pls_open(struct vfsent *ve, int isdir);
/**
 * @brief Add all items in the PLS file to the population of the current
 *        node.
 */
int	vfs_pls_populate(struct vfsent *ve);

#endif /* !_VFS_MODULES_H_ */
