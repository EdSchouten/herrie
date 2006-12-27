/*
 * Copyright (c) 2006 Ed Schouten <ed@fxq.nl>
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
 * @file vfs.h
 */

#ifndef _VFS_H_
#define _VFS_H_

#include <glib.h>

struct vfsref;

/**
 * @brief List structure that can contain a lot of VFS references.
 */
struct vfslist {
	/**
	 * @brief First item in the list.
	 */
	struct vfsref *first;
	/**
	 * @brief Last item in the list.
	 */
	struct vfsref *last;
	/**
	 * @brief The number of items in the list.
	 */
	unsigned int items;
};

/**
 * @brief Contents of an empty VFS list structure.
 */
#define VFSLIST_INITIALIZER { NULL, NULL, 0 }

/**
 * @brief Run-time initialize a VFS list structure.
 */
#define vfs_list_init(vl)						\
do {									\
	(vl)->first = (vl)->last = NULL;				\
	(vl)->items = 0;						\
} while (0)
/**
 * @brief Return the first item in a VFS list.
 */
#define vfs_list_first(vl)	((vl)->first)
/**
 * @brief Return the last item in a VFS list.
 */
#define vfs_list_last(vl)	((vl)->last)
/**
 * @brief Return whether the VFS list is empty or not.
 */
#define vfs_list_empty(vl)	(vfs_list_first(vl) == NULL)
/**
 * @brief Return the amount of items in the VFS list.
 */
#define vfs_list_items(vl)	((vl)->items)
/**
 * @brief Return the next item in the VFS list.
 */
#define vfs_list_next(vr)	((vr)->next)
/**
 * @brief Return the preivous item in the VFS list.
 */
#define vfs_list_prev(vr)	((vr)->prev)
/**
 * @brief Loop through all items in the VFS list.
 */
#define vfs_list_foreach(vl, vr) \
	for (vr = vfs_list_first(vl); vr != NULL; vr = vfs_list_next(vr))
/**
 * @brief Reverse loop through all items in the VFS list.
 */
#define vfs_list_foreach_reverse(vl, vr) \
	for (vr = vfs_list_last(vl); vr != NULL; vr = vfs_list_prev(vr))
/**
 * @brief Remove the VFS reference from the VFS list.
 */
#define vfs_list_remove(vl, vr) \
do {									\
	if (vfs_list_next(vr) == NULL) {				\
		vfs_list_last(vl) = vfs_list_prev(vr);			\
	} else {							\
		vfs_list_prev(vfs_list_next(vr)) = vfs_list_prev(vr);	\
	}								\
	if (vfs_list_prev(vr) == NULL) {				\
		vfs_list_first(vl) = vfs_list_next(vr);			\
	} else {							\
		vfs_list_next(vfs_list_prev(vr)) = vfs_list_next(vr);	\
	}								\
	vfs_list_items(vl)--;						\
} while (0)
/**
 * @brief Insert the VFS reference at the head of the VFS list.
 */
#define vfs_list_insert_head(vl, vr) \
do {									\
	vfs_list_prev(vr) = NULL;					\
	vfs_list_next(vr) = vfs_list_first(vl);				\
	vfs_list_first(vl) = (vr);					\
	if (vfs_list_next(vr) != NULL) {				\
		vfs_list_prev(vfs_list_next(vr)) = (vr);		\
	} else {							\
		vfs_list_last(vl) = (vr);				\
	}								\
	vfs_list_items(vl)++;						\
} while (0)
/**
 * @brief Insert the VFS reference at the tail of the VFS list.
 */
#define vfs_list_insert_tail(vl, vr) \
do {									\
	vfs_list_prev(vr) = vfs_list_last(vl);				\
	vfs_list_next(vr) = NULL;					\
	vfs_list_last(vl) = (vr);					\
	if (vfs_list_prev(vr) != NULL) {				\
		vfs_list_next(vfs_list_prev(vr)) = (vr);		\
	} else {							\
		vfs_list_first(vl) = (vr);				\
	}								\
	vfs_list_items(vl)++;						\
} while (0)
/**
 * @brief Insert the VFS reference before another one in the VFS list.
 */
#define vfs_list_insert_before(vl, nvr, lvr) \
do {									\
	vfs_list_prev(nvr) = vfs_list_prev(lvr);			\
	vfs_list_next(nvr) = (lvr);					\
	vfs_list_prev(lvr) = (nvr);					\
	if (vfs_list_prev(nvr) != NULL) {				\
		vfs_list_next(vfs_list_prev(nvr)) = (nvr);		\
	} else {							\
		vfs_list_first(vl) = (nvr);				\
	}								\
	vfs_list_items(vl)++;						\
} while (0)
/**
 * @brief Insert the VFS reference after another one in the VFS list.
 */
#define vfs_list_insert_after(vl, nvr, lvr) \
do {									\
	vfs_list_prev(nvr) = (lvr);					\
	vfs_list_next(nvr) = vfs_list_next(lvr);			\
	vfs_list_next(lvr) = (nvr);					\
	if (vfs_list_next(nvr) != NULL) {				\
		vfs_list_prev(vfs_list_next(nvr)) = (nvr);		\
	} else {							\
		vfs_list_last(vl) = (nvr);				\
	}								\
	vfs_list_items(vl)++;						\
} while (0)

struct vfsent;

/**
 * @brief Module representing a type of file or directory on disk, containing
 *        functions to read its contents (populate it).
 */
struct vfsmodule {
	/**
	 * @brief Attach the VFS module to a VFS entity
	 */
	int	(*vopen)(struct vfsent *ve, int isdir);
	/**
	 * @brief Detach the VFS module to a VFS entity
	 */
	void	(*vclose)(struct vfsent *ve);
	/**
	 * @brief Populate the VFS entity with its childs
	 */
	int	(*vpopulate)(struct vfsent *ve);

	/**
	 * @brief Recurse through this object when the parent is recursed
	 */
	char	recurse;

	/**
	 * @brief Entity represents a file that is a regular media file
	 */
	char	playable;

	/**
	 * @brief Sorting number of items that should appear at the top
	 */
#define VFS_SORT_FIRST	0
	/**
	 * @brief Sorting number of items that should appear at the bottom
	 */
#define VFS_SORT_LAST	255
	/**
	 * @brief Order in which files should be sorted in the browser
	 */
	unsigned char sortorder;

	/**
	 * @brief Charachter placed behind the character marking the filetype
	 */
	char	marking;
};

/**
 * @brief A VFS entity is an object representing a single file or directory on
 *        disk. Each VFS entity is handled by some kind of VFS module. By
 *        default, a VFS entity only contains the filename and a reference
 *        count. It must be manually populated to get its actual contents.
 */
struct vfsent {
	/**
	 * @brief The name of the current object (the basename)
	 */
	char	*name;
	/**
	 * @brief The complete filename of the object (realpath)
	 */
	char	*filename;

	/**
	 * @brief The reference count of the current object, used to determine
	 *        whether it should be deallocated.
	 */
	int	refcount;

	/**
	 * @brief The VFS module responsible for handling the entity
	 */
	struct vfsmodule *vmod;
	/**
	 * @brief References to its children
	 */
	struct vfslist population;
};

/**
 * @brief Reference to a VFS entity including list pointers.
 */
struct vfsref {
	/**
	 * @brief Pointer to the VFS entity.
	 */
	struct vfsent *ent;
	/**
	 * @brief Pointer to the next item in the VFS list.
	 */
	struct vfsref *next;
	/**
	 * @brief Pointer to the previous item in the VFS list.
	 */
	struct vfsref *prev;
};

#ifdef BUILD_LOCKUP
/**
 * @brief Try to lock the application in a specified directory on
 *        startup.
 */
int	vfs_lockup(void);
#endif /* BUILD_LOCKUP */

/**
 * @brief Create a VFS reference from a filename. The name argument is
 *        optional. It only allows you to display entities with a
 *        different name (inside playlists). When setting the basepath
 *        variable, all relative pathnames are appended to the basepath.
 *        When unset, it can only open absolute filenames.
 */
struct vfsref	*vfs_open(const char *filename, const char *name,
    const char *basepath);
/**
 * @brief Duplicate the reference by increasing the reference count.
 */
struct vfsref	*vfs_dup(struct vfsref *vr);
/**
 * @brief Decrease the reference count of the entity and deallocate the
 *        entity when no references are left. The reference itself is also
 *        deallocated.
 */
void		vfs_close(struct vfsref *vr);
/**
 * @brief Populate the VFS entity with references to its children.
 */
int		vfs_populate(struct vfsref *vr);
/**
 * @brief Recursively expand a VFS reference to all their usable
 *        children and append them to the specified list.
 */
void		vfs_unfold(struct vfslist *vl, struct vfsref *vr);
/**
 * @brief Get the friendly name of the current VFS reference.
 */
#define vfs_name(vr) 		((const char *)(vr)->ent->name)
/**
 * @brief Get the full pathname of the current VFS reference.
 */
#define vfs_filename(vr)	((const char *)(vr)->ent->filename)
/**
 * @brief Determine if a VFS entity is playable audio.
 */
#define vfs_playable(vr)	((vr)->ent->vmod->playable)
/**
 * @brief Determine if the VFS entity can have population.
 */
#define vfs_populatable(vr)	((vr)->ent->vmod->vpopulate != NULL)
/**
 * @brief Get the character that should be used to mark the file with in
 *        the filebrowser.
 */
#define vfs_marking(vr)		((vr)->ent->vmod->marking)
/**
 * @brief Get the sorting priority of the current object.
 */
#define vfs_sortorder(vr)	((vr)->ent->vmod->sortorder)
/**
 * @brief Determine if we should recurse this object.
 */
#define vfs_recurse(vr)		((vr)->ent->vmod->recurse)
/**
 * @brief Return a pointer to the VFS list inside the VFS reference.
 */
#define vfs_population(vr)	(&(vr)->ent->population)

#endif /* !_VFS_H_ */
