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
	struct header_depend *hd;

	/* Make sure item is not already in the list */
	SLIST_FOREACH(hd, &to->depends, next) {
		if (hd->header == from)
			return;
	}

	/* Add item itself */
	hd = malloc(sizeof(struct header_depend));
	hd->header = from;
	SLIST_INSERT_HEAD(&to->depends, hd, next);

	SLIST_FOREACH(hd, &from->depends, next)
		file_add_recursive(hd->header, to);
}

static struct header *
file_scan(const char *filename)
{
	struct header *h, *hl;
	char fbuf[2048], *eol;
	FILE *fp;

	/* See if we already scanned the file */
	SLIST_FOREACH(h, &headerlist, next) {
		/* Match */
		if (strcmp(filename, h->filename) == 0)
			return (h);
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
	SLIST_INSERT_HEAD(&headerlist, h, next);

	/* Scan the file */
	while (fgets(fbuf, sizeof fbuf, fp) != NULL) {
		if (strncmp(fbuf, "#include \"", 10) == 0) {
			eol = strchr(fbuf + 10, '"');
			if (eol == NULL)
				continue;
			*eol = '\0';

			hl = file_scan(fbuf + 10);
			if (hl != NULL)
				file_add_recursive(hl, h);
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
		if (strcmp(h->filename, "conftest.c") == 0)
			continue;
		fprintf(out, "DEPENDS_");
		fwrite(h->filename, 1, ext - h->filename, out);
		fprintf(out, "=\"");
		SLIST_FOREACH(hd, &h->depends, next) {
			if (strcmp(hd->header->filename, "stdinc.h") == 0)
				continue;
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

	return (0);
}
