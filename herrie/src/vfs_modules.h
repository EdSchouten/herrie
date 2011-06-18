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
 * @file vfs_modules.h
 * @brief Virtual filesystem access modules.
 */

struct vfsent;
struct vfslist;

/**
 * @brief Open a directory.
 */
int	vfs_dir_match(struct vfsent *ve, int isdir);
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
int	vfs_file_match(struct vfsent *ve, int isdir);
/**
 * @brief Create a file handle to the file
 */
FILE	*vfs_file_open(struct vfsent *ve);

#ifdef BUILD_HTTP
/**
 * @brief Test whether the current node contains a HTTP address.
 */
int	vfs_http_match(struct vfsent *ve, int isdir);
/**
 * @brief Return a filehandle to a HTTP audio stream.
 */
FILE	*vfs_http_open(struct vfsent *ve);
#endif /* BUILD_HTTP */

/**
 * @brief Test whether the current node is an M3U file.
 */
int	vfs_m3u_match(struct vfsent *ve, int isdir);
/**
 * @brief Add all items in the M3U file to the population of the current
 *        node.
 */
int	vfs_m3u_populate(struct vfsent *ve);
/**
 * @brief Write the current list of VFS entries to a M3U formatted
 *        playlist.
 */
int	vfs_m3u_write(const struct vfslist *vl, const char *filename);

/**
 * @brief Test whether the current node is an PLS file.
 */
int	vfs_pls_match(struct vfsent *ve, int isdir);
/**
 * @brief Add all items in the PLS file to the population of the current
 *        node.
 */
int	vfs_pls_populate(struct vfsent *ve);
/**
 * @brief Write the current list of VFS entries to a PLS formatted
 *        playlist.
 */
int	vfs_pls_write(const struct vfslist *vl, const char *filename);

#ifdef BUILD_XSPF
/**
 * @brief Test whether the current node is an XSPF file.
 */
int	vfs_xspf_match(struct vfsent *ve, int isdir);
/**
 * @brief Add all items in the XSPF file to the population of the
 *        current node.
 */
int	vfs_xspf_populate(struct vfsent *ve);
/**
 * @brief Write the current list of VFS entries to a XSPF formatted
 *        playlist.
 */
int	vfs_xspf_write(const struct vfslist *vl, const char *filename);
#endif /* BUILD_XSPF */
