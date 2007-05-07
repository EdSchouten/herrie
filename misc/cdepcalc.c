#include <sys/types.h>
#include <sys/queue.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SLIST_HEAD(, header) headerlist = SLIST_HEAD_INITIALIZER(headerlist);

struct header_depend {
	struct header *header;
	SLIST_ENTRY(header_depend) next;
};

struct header {
	char *filename;

	SLIST_ENTRY(header) next;
	SLIST_HEAD(, header_depend) depends;
};

static void
file_add_recursive(struct header *from, struct header *to)
{
	struct header_depend *hd, *hdp, *hdn;

	/* Make sure item is not already in the list */
	SLIST_FOREACH(hd, &to->depends, next) {
		if (hd->header == from)
			return;
	}

	/* Add item itself */
	hd = malloc(sizeof(struct header_depend));
	hd->header = from;

	hdp = SLIST_FIRST(&to->depends);
	if (hdp == NULL || strcmp(hd->header->filename,
	    hdp->header->filename) < 0) {
		/* Add it to the beginning */
		SLIST_INSERT_HEAD(&to->depends, hd, next);
	} else {
		/* Add it after an existing item */
		SLIST_FOREACH(hdp, &to->depends, next) {
			hdn = SLIST_NEXT(hdp, next);
			if (hdn == NULL || strcmp(hd->header->filename,
			    hdn->header->filename) < 0) {
				SLIST_INSERT_AFTER(hdp, hd, next);
				break;
			}
		}
	}

	SLIST_FOREACH(hd, &from->depends, next)
		file_add_recursive(hd->header, to);
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

	hp = SLIST_FIRST(&headerlist);
	if (hp != NULL) {
		cmp = strcmp(filename, hp->filename);
		if (cmp == 0) {
			return (hp);
		} else if (cmp < 0) {
			/* Item cannot be in the list - add to beginning */
			hp = NULL;
		} else {
			/* Item may be in the list - look it up */
			SLIST_FOREACH(hp, &headerlist, next) {
				hn = SLIST_NEXT(hp, next);
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
	SLIST_INIT(&h->depends);

	/* Add it alphabetically to the list */
	if (hp == NULL)
		SLIST_INSERT_HEAD(&headerlist, h, next);
	else
		SLIST_INSERT_AFTER(hp, h, next);

	/* Scan the file */
	while (fgets(fbuf, sizeof fbuf, fp) != NULL) {
		if (strncmp(fbuf, "#include \"", 10) == 0) {
			eol = strchr(fbuf + 10, '"');
			if (eol == NULL)
				continue;
			*eol = '\0';

			hs = file_scan(fbuf + 10);
			if (hs != NULL)
				file_add_recursive(hs, h);
		}
	}

	return (h);
}

int
main(int argc, char *argv[])
{
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

	SLIST_FOREACH(h, &headerlist, next) {
		/* Only print C source code - skip conftest */
		ext = strchr(h->filename, '.');
		if (ext == NULL || strcmp(ext, ".c") != 0)
			continue;
		fprintf(out, "DEPENDS_");
		fwrite(h->filename, 1, ext - h->filename, out);
		fprintf(out, "=\"");
		SLIST_FOREACH(hd, &h->depends, next) {
			if (SLIST_FIRST(&h->depends) != hd)
				fprintf(out, " ");
			/* Only print header files */
			ext = strchr(hd->header->filename, '.');
			if (ext == NULL || strcmp(ext, ".h") != 0)
				continue;
			fwrite(hd->header->filename, 1,
			    ext - hd->header->filename, out);
		}
		fprintf(out, "\"\n");
	}
	fclose(out);

	return (0);
}
