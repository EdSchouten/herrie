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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct dependency;

struct file {
	char			*filename;
	struct dependency	*firstdep;
	struct file		*next;
};

struct dependency {
	struct file		*depfile;
	struct dependency	*next;
};

struct file *filelist = NULL;

static struct file *
file_lookup(const char *filename)
{
	struct file *cur;

	for (cur = filelist; cur != NULL; cur = cur->next) {
		if (strcmp(filename, cur->filename) == 0)
			return (cur);
	}

	return (NULL);
}

static struct file *
file_add(const char *filename)
{
	FILE *rf;
	struct file *ret, *df;
	char *line, *end;
	size_t len;

	if ((ret = file_lookup(filename)) != NULL)
		return (ret);

	rf = fopen(filename, "rt");
	if (rf == NULL) {
		fprintf(stderr, "Cannot open %s: %s\n", filename,
		    strerror(errno));
	}

	/* Allocate a new file and add it to the file list */
	ret = malloc(sizeof(struct file));
	ret->filename = strdup(filename);
	ret->firstdep = NULL;
	ret->next = filelist;
	filelist = ret;

	/* Look for deps in the file */
	while ((line = fgetln(rf, &len)) != NULL) {
		if (strncmp(line, "#include \"", 10) == 0) {
			line += 10;
			if ((end = memchr(line, '"', len)) == NULL)
				continue;
			*end = '\0';

			df = file_add(line);
		}
	}
	
	fclose(rf);
	return (ret);
}

static void
usage(void)
{
	fprintf(stderr, "usage: cdepend filename ...\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	if (argc == 1)
		usage();

	/* Analyze all input files */
	while (--argc > 0)
		file_add(argv[argc]);
	
	/* XXX */
	struct file *cur;
	for (cur = filelist; cur != NULL; cur = cur->next) {
		printf("%s\n", cur->filename);
	}
	
	return (0);
}
