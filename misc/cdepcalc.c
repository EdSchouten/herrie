/*
 * Copyright (c) 2006-2008 Ed Schouten <ed@80386.nl>
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

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct header;
struct header_depend;

struct header {
	char *filename;
	struct header_depend *depends;
	struct header *next;
};

struct header_depend {
	struct header *header;
	struct header_depend *next;
};

struct header *headerlist = NULL;

static int
depend_add(struct header *from, struct header *to)
{
	struct header_depend *hd, *hdp, *hdn;

	/* We can't depend on ourself. Prevent duplicates. */
	if (from == to)
		return (-1);
	for (hd = to->depends; hd != NULL; hd = hd->next) {
		if (hd->header == from)
			return (-1);
	}

	/* Add item itself */
	hd = malloc(sizeof(struct header_depend));
	hd->header = from;

	hdp = to->depends;
	if (hdp == NULL || strcmp(hd->header->filename,
	    hdp->header->filename) < 0) {
		/* Add it to the beginning */
		hd->next = hdp;
		to->depends = hd;
	} else {
		/* Add it after an existing item */
		for (hdp = to->depends; hdp != NULL; hdp = hdp->next) {
			hdn = hdp->next;
			if (hdn == NULL || strcmp(hd->header->filename,
			    hdn->header->filename) < 0) {
				/* Insert it in between */
			    	hd->next = hdp->next;
				hdp->next = hd;
				break;
			}
		}
	}

	return (0);
}

static void
depend_copy(struct header *from, struct header *to)
{
	struct header_depend *hd;

	if (depend_add(from, to) != 0)
		return;
	for (hd = from->depends; hd != NULL; hd = hd->next)
		depend_add(hd->header, to);
}

static struct header *
file_scan(const char *filename)
{
	int cmp;
	struct header *h, *hp, *hn, *hs;
	char fbuf[2048], *eol;
	FILE *fp;

	if (strcmp(filename, "conftest.c") == 0 ||
	    strcmp(filename, "stdinc.h") == 0)
		return (NULL);

	hp = headerlist;
	if (hp != NULL) {
		cmp = strcmp(filename, hp->filename);
		if (cmp == 0) {
			return (hp);
		} else if (cmp < 0) {
			/* Item cannot be in the list - add to beginning */
			hp = NULL;
		} else {
			/* Item may be in the list - look it up */
			for (hp = headerlist; hp != NULL; hp = hp->next) {
				hn = hp->next;
				if (hn == NULL)
					break;
				cmp = strcmp(filename, hn->filename);
				if (cmp == 0)
					/* We've found it */
					return (hn);
				else if (cmp < 0)
					/* Add in between */
					break;
			}
		}
	}

	fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Failed to open %s\n", filename);
		return (NULL);
	}

	/* We can read it - we must track it */
	h = malloc(sizeof(struct header));
	h->filename = strdup(filename);
	h->depends = NULL;
	h->next = NULL;

	/* Add it alphabetically to the list */
	if (hp == NULL) {
		h->next = headerlist;
		headerlist = h;
	} else {
		h->next = hp->next;
		hp->next = h;
	}

	/* Scan the file */
	while (fgets(fbuf, sizeof fbuf, fp) != NULL) {
		if (strncmp(fbuf, "#include \"", 10) == 0) {
			eol = strchr(fbuf + 10, '"');
			if (eol == NULL)
				continue;
			*eol = '\0';

			hs = file_scan(fbuf + 10);
			if (hs != NULL)
				depend_copy(hs, h);
		}
	}

	return (h);
}

int
main(int argc, char *argv[])
{
	int first;
	char *ext;
	DIR *srcdir;
	FILE *out;
	struct dirent *fent;
	struct header *h;
	struct header_depend *hd;

	srcdir = opendir(".");
	if (srcdir == NULL) {
		fprintf(stderr, "Failed to open current directory\n");
		return (1);
	}

	while ((fent = readdir(srcdir)) != NULL) {
		/* Only scan C source code */
		ext = strchr(fent->d_name, '.');
		if (ext == NULL || strcmp(ext, ".c") != 0)
			continue;
		file_scan(fent->d_name);
	}
	closedir(srcdir);

	out = fopen("../depends", "w");
	if (out == NULL) {
		fprintf(stderr, "Failed to open output file\n");
		return (1);
	}

	for (h = headerlist; h != NULL; h = h->next) {
		/* Only print C source code - skip conftest */
		ext = strchr(h->filename, '.');
		if (ext == NULL || strcmp(ext, ".c") != 0)
			continue;
		fprintf(out, "DEPENDS_");
		fwrite(h->filename, 1, ext - h->filename, out);
		fprintf(out, "=\"");
		first = 1;
		for (hd = h->depends; hd != NULL; hd = hd->next) {
			/* Only print header files */
			ext = strchr(hd->header->filename, '.');
			if (ext == NULL || strcmp(ext, ".h") != 0)
				continue;
			if (first)
				first = 0;
			else
				fprintf(out, " ");
			fwrite(hd->header->filename, 1,
			    ext - hd->header->filename, out);
		}
		fprintf(out, "\"\n");

#if 0
		/* Regular Makefile-style output */
		ext = strchr(h->filename, '.');
		if (ext == NULL || strcmp(ext, ".c") != 0)
			continue;
		fwrite(h->filename, 1, ext - h->filename, out);
		fprintf(out, ".o: %s", h->filename);
		for (hd = h->depends; hd != NULL; hd = hd->next)
			fprintf(out, " %s", hd->header->filename);
		fprintf(out, "\n");
#endif
	}
	fclose(out);

	return (0);
}
