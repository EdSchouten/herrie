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
 * @file vfs.h
 * @brief Virtual filesystem.
 */

/*
 * All structures mentioned in this file are only directly dereferenced
 * in the VFS itself, so we could remove their declaration to a private
 * header file. The problem is that this results in worse performance,
 * especially on the vfs_list_* functions.
 */

struct vfsref;
struct vfsent;

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
 * @brief Module representing a type of file or directory on disk, containing
 *        functions to read its contents (populate it).
 */
struct vfsmodule {
	/**
	 * @brief Attach the VFS module to a VFS entity
	 */
	int	(*match)(struct vfsent *ve, int isdir);
	/**
	 * @brief Populate the VFS entity with its childs
	 */
	int	(*populate)(struct vfsent *ve);
	/**
	 * @brief Return a FILE* to the file on disk
	 */
	FILE	*(*open)(struct vfsent *ve);

	/**
	 * @brief Does not need an on-disk file
	 */
	char	pseudo;

	/**
	 * @brief Order in which files should be sorted in vfs_dir
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
	/**
	 * @brief Whether or not we should recurse down this object
	 */
	int	recurse;
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
	/**
	 * @brief Indicator that this reference should be drawn in a
	 *        different color in the browser/playlist.
	 */
	int marked;
};

/**
 * @brief Compiled regular expression or string matching data.
 */
struct vfsmatch {
	/**
	 * @brief Regular expression pattern.
	 */
	regex_t		regex;
	/**
	 * @brief Original user input.
	 */
	char		*string;
};

/**
 * @brief Contents of an empty VFS list structure.
 */
#define VFSLIST_INITIALIZER { NULL, NULL, 0 }

/**
 * @brief Run-time initialize a VFS list structure.
 */
static inline void
vfs_list_init(struct vfslist *vl)
{
	vl->first = vl->last = NULL;
	vl->items = 0;
}

/**
 * @brief Move the contents of one list to another.
 */
static inline void
vfs_list_move(struct vfslist *vdst, const struct vfslist *vsrc)
{
	memcpy(vdst, vsrc, sizeof(struct vfslist));
}

/**
 * @brief Return the first item in a VFS list.
 */
static inline struct vfsref *
vfs_list_first(const struct vfslist *vl)
{
	return (vl->first);
}

/**
 * @brief Return the last item in a VFS list.
 */
static inline struct vfsref *
vfs_list_last(const struct vfslist *vl)
{
	return (vl->last);
}

/**
 * @brief Return whether the VFS list is empty or not.
 */
static inline int
vfs_list_empty(const struct vfslist *vl)
{
	return (vl->first == NULL);
}

/**
 * @brief Return the amount of items in the VFS list.
 */
static inline unsigned int
vfs_list_items(const struct vfslist *vl)
{
	return (vl->items);
}

/**
 * @brief Return the next item in the VFS list.
 */
static inline struct vfsref *
vfs_list_next(const struct vfsref *vr)
{
	return (vr->next);
}

/**
 * @brief Return the preivous item in the VFS list.
 */
static inline struct vfsref *
vfs_list_prev(const struct vfsref *vr)
{
	return (vr->prev);
}

/**
 * @brief Loop through all items in the VFS list.
 */
#define VFS_LIST_FOREACH(vl, vr) \
	for (vr = vfs_list_first(vl); vr != NULL; vr = vfs_list_next(vr))
/**
 * @brief Reverse loop through all items in the VFS list.
 */
#define VFS_LIST_FOREACH_REVERSE(vl, vr) \
	for (vr = vfs_list_last(vl); vr != NULL; vr = vfs_list_prev(vr))

/**
 * @brief Remove the VFS reference from the VFS list.
 */
static inline void
vfs_list_remove(struct vfslist *vl, struct vfsref *vr)
{
	if (vr->next == NULL) {
		/* Item was the last in the list */
		g_assert(vl->last == vr);
		vl->last = vr->prev;
	} else {
		/* There is an item after this one */
		vr->next->prev = vr->prev;
	}
	if (vr->prev == NULL) {
		/* Item was the first in the list */
		g_assert(vl->first == vr);
		vl->first = vr->next;
	} else {
		/* There is an item before this one */
		vr->prev->next = vr->next;
	}
	vl->items--;
}

/**
 * @brief Insert the VFS reference at the head of the VFS list.
 */
static inline void
vfs_list_insert_head(struct vfslist *vl, struct vfsref *vr)
{
	vr->prev = NULL;
	vr->next = vl->first;
	vl->first = vr;
	if (vr->next != NULL) {
		/* Item is stored before other ones */
		vr->next->prev = vr;
	} else {
		/* New item is only item in the list */
		vl->last = vr;
	}
	vl->items++;
}

/**
 * @brief Insert the VFS reference at the tail of the VFS list.
 */
static inline void
vfs_list_insert_tail(struct vfslist *vl, struct vfsref *vr)
{
	vr->prev = vl->last;
	vr->next = NULL;
	vl->last = vr;
	if (vr->prev != NULL) {
		/* Item is stored after other ones */
		vr->prev->next = vr;
	} else {
		/* New item is only item in the list */
		vl->first = vr;
	}
	vl->items++;
}

/**
 * @brief Insert the VFS reference before another one in the VFS list.
 */
static inline void
vfs_list_insert_before(struct vfslist *vl, struct vfsref *nvr,
    struct vfsref *lvr)
{
	nvr->prev = lvr->prev;
	nvr->next = lvr;
	lvr->prev = nvr;
	if (nvr->prev != NULL) {
		/* Store item between two others */
		nvr->prev->next = nvr;
	} else {
		/* New item is added before the first one */
		vl->first = nvr;
	}
	vl->items++;
}

/**
 * @brief Insert the VFS reference after another one in the VFS list.
 */
static inline void
vfs_list_insert_after(struct vfslist *vl, struct vfsref *nvr,
    struct vfsref *lvr)
{
	nvr->prev = lvr;
	nvr->next = lvr->next;
	lvr->next = nvr;
	if (nvr->next != NULL) {
		/* Store item between two others */
		nvr->next->prev = nvr;
	} else {
		/* New item is added after the first one */
		vl->last = nvr;
	}
	vl->items++;
}

/**
 * @brief Try to lock the application in a specified directory on
 *        startup. This function returns an error message.
 */
const char	*vfs_lockup(void);

/**
 * @brief Create a VFS reference from a filename. The name argument is
 *        optional. It only allows you to display entities with a
 *        different name (inside playlists). When setting the basepath
 *        variable, all relative pathnames are appended to the basepath.
 *        When unset, it can only open absolute filenames. When forcing
 *        strict pathnames, application-implemented features like ~ are
 *        discarded.
 */
struct vfsref	*vfs_lookup(const char *filename, const char *name,
    const char *basepath, int strict);
/**
 * @brief Duplicate the reference by increasing the reference count.
 */
struct vfsref	*vfs_dup(const struct vfsref *vr);
/**
 * @brief Decrease the reference count of the entity and deallocate the
 *        entity when no references are left. The reference itself is also
 *        deallocated.
 */
void		vfs_close(struct vfsref *vr);
/**
 * @brief Populate the VFS entity with references to its children.
 */
int		vfs_populate(const struct vfsref *vr);
/**
 * @brief Recursively expand a VFS reference to all their usable
 *        children and append them to the specified list.
 */
void		vfs_unfold(struct vfslist *vl, const struct vfsref *vr);
/**
 * @brief Recursively search through a VFS reference and add all
 *        matching objects to a list. The VFS reference itself will be
 *        excluded from the results.
 */
void		vfs_locate(struct vfslist *vl, const struct vfsref *vr,
    const struct vfsmatch *vm);
/**
 * @brief Write a VFS list to a PLS file on disk.
 */
struct vfsref	*vfs_write_playlist(const struct vfslist *vl,
    const struct vfsref *vr, const char *filename);
/**
 * @brief Delete a local file. Use with caution. ;-)
 */
int		vfs_delete(const char *filename);
/**
 * @brief fopen()-like routine that uses VFS path expansion.
 */
FILE		*vfs_fopen(const char *filename, const char *mode);
/**
 * @brief fgets()-like routine that performs newline-stripping.
 */
int		vfs_fgets(char *str, size_t size, FILE *fp);
/**
 * @brief Initialize the VFS system cache if enabled.
 */
void		vfs_cache_init(void);
/**
 * @brief Add entry to the VFS cache.
 */
void		vfs_cache_add(const struct vfsref *vr);
/**
 * @brief Obtain entry from the VFS cache.
 */
struct vfsref 	*vfs_cache_lookup(const char *filename);
/**
 * @brief Purge the VFS cache.
 */
void		vfs_cache_purge(void);

/**
 * @brief Get the friendly name of the current VFS reference.
 */
static inline const char *
vfs_name(const struct vfsref *vr)
{
	return (vr->ent->name);
}

/**
 * @brief Get the full pathname of the current VFS reference.
 */
static inline const char *
vfs_filename(const struct vfsref *vr)
{
	return (vr->ent->filename);
}

/**
 * @brief Determine if a VFS entity is playable audio.
 */
static inline int
vfs_playable(const struct vfsref *vr)
{
	return (vr->ent->vmod->open != NULL);
}

/**
 * @brief Open a new filehandle to the entity.
 */
static inline FILE *
vfs_open(const struct vfsref *vr)
{
	return vr->ent->vmod->open(vr->ent);
}

/**
 * @brief Determine if the VFS entity can have population.
 */
static inline int
vfs_populatable(const struct vfsref *vr)
{
	return (vr->ent->vmod->populate != NULL);
}

/**
 * @brief Get the character that should be used to mark the file with in
 *        the filebrowser.
 */
static inline char
vfs_marking(const struct vfsref *vr)
{
	return (vr->ent->vmod->marking);
}

/**
 * @brief Return a pointer to the VFS list inside the VFS reference.
 */
static inline const struct vfslist *
vfs_population(const struct vfsref *vr)
{
	return (&vr->ent->population);
}

/**
 * @brief Return whether the current reference is marked.
 */
static inline int
vfs_marked(const struct vfsref *vr)
{
	return (vr->marked != 0);
}

/**
 * @brief Mark the current reference.
 */
static inline void
vfs_mark(struct vfsref *vr)
{
	vr->marked = 1;
}

/**
 * @brief Unmark the current reference.
 */
static inline void
vfs_unmark(struct vfsref *vr)
{
	vr->marked = 0;
}

/**
 * @brief Compile a regular expression for matching.
 */
struct vfsmatch	*vfs_match_new(const char *str);
/**
 * @brief Deallocate a compiled regular expression.
 */
void		vfs_match_free(struct vfsmatch *vm);

/**
 * @brief Match a VFS reference with a regular expression.
 */
static inline int
vfs_match_compare(const struct vfsmatch *vm, const char *name)
{
	return (regexec(&vm->regex, name, 0, NULL, 0) == 0);
}

/**
 * @brief Return the search string that the user has entered.
 */
static inline const char *
vfs_match_value(const struct vfsmatch *vm)
{
	return (vm->string);
}
