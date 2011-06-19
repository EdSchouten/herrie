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
 * @file config.c
 * @brief Configuration file handling.
 */

#include "stdinc.h"

#include "config.h"
#include "gui.h"
#include "vfs.h"

/**
 * @brief Convert a "yes"/"no" string to a boolean value.
 */
static int
string_to_bool(const char *val)
{
	if (strcmp(val, "yes") == 0)
		return (1);
	else if (strcmp(val, "no") == 0)
		return (0);
	else
		return (-1);
}

/*
 * Configuration value validation
 */

/**
 * @brief Determine if a boolean string is valid
 */
static int
valid_bool(char *val)
{
	return (string_to_bool(val) == -1);
}

/**
 * @brief Determine if a color string is valid
 */
static int
valid_color(char *val)
{
	return (gui_draw_color_number(val) < -1);
}

/**
 * @brief Determine if a percentage string is valid
 */
static int
valid_percentage(char *val)
{
	unsigned long pct;
	char *end = NULL;

	pct = strtol(val, &end, 10);
	return (pct > 100 || end == NULL || *end != '\0');
}

#ifdef BUILD_SCROBBLER
/**
 * @brief Determine if a string containing an MD5 hash is valid
 */
static int
valid_md5(char *val)
{
	int i;

	if (val[0] == '\0')
		return (0);

	for (i = 0; i < 32; i++) {
		/* Silently convert hash to lowercase */
		val[i] = g_ascii_tolower(val[i]);

		if (!g_ascii_isxdigit(val[i]))
			/* Invalid character found */
			return (-1);
	}

	if (val[i] != '\0')
		/* String too long */
		return (-1);

	return (0);
}
#endif /* BUILD_SCROBBLER */

/**
 * @brief Structure containing a single configuration entry of the
 *        application.
 */
struct config_entry {
	/**
	 * @brief Name it can be located at.
	 */
	const char	*name;
	/**
	 * @brief Default value.
	 */
	const char	*defval;
	/**
	 * @brief Function to validate when set.
	 */
	int		(*validator)(char *val);
	/**
	 * @brief Current value if not equal to the default.
	 */
	char		*curval;
};

/*
 * List of available configuration switches. Keep this list sorted at
 * all time; the config lookups have been optimised to expect an
 * alphabetical lookup. Looking up switches starting with 'z' will take
 * longer than 'a'.
 */
/**
 * @brief List of configuration switches.
 */
static struct config_entry configlist[] = {
#ifdef BUILD_ALSA
	{ "audio.output.alsa.device",	"default",	NULL,		NULL },
#ifdef BUILD_VOLUME
	{ "audio.output.alsa.mixer",	"PCM",		NULL,		NULL },
#endif /* BUILD_VOLUME */
#endif /* BUILD_ALSA */
#ifdef BUILD_AO
	{ "audio.output.ao.driver",	"",		NULL,		NULL },
	{ "audio.output.ao.host",	"",		NULL,		NULL },
#endif /* BUILD_AO */
#ifdef BUILD_OSS
	{ "audio.output.oss.device",	OSS_DEVICE,	NULL,		NULL },
#ifdef BUILD_VOLUME
	{ "audio.output.oss.mixer",	"/dev/mixer",	NULL,		NULL },
#endif /* BUILD_VOLUME */
#endif /* BUILD_OSS */
	{ "gui.browser.defaultpath",	"",		NULL,		NULL },
	{ "gui.color.bar.bg",		"blue",		valid_color,	NULL },
	{ "gui.color.bar.fg",		"white",	valid_color,	NULL },
	{ "gui.color.block.bg",		"black",	valid_color,	NULL },
	{ "gui.color.block.fg",		"white",	valid_color,	NULL },
	{ "gui.color.deselect.bg",	"white",	valid_color,	NULL },
	{ "gui.color.deselect.fg",	"black",	valid_color,	NULL },
	{ "gui.color.enabled",		"yes",		valid_bool,	NULL },
	{ "gui.color.marked.bg",	"yellow",	valid_color,	NULL },
	{ "gui.color.marked.fg",	"black",	valid_color,	NULL },
	{ "gui.color.select.bg",	"cyan",		valid_color,	NULL },
	{ "gui.color.select.fg",	"black",	valid_color,	NULL },
	{ "gui.input.confirm",		"yes",		valid_bool,	NULL },
	{ "gui.input.may_quit",		"yes",		valid_bool,	NULL },
	{ "gui.ratio",			"50",		valid_percentage, NULL },
	{ "gui.vfslist.scrollpages",	"no",		valid_bool,	NULL },
	{ "playq.autoplay",		"no",		valid_bool,	NULL },
	{ "playq.dumpfile",		CONFHOMEDIR PLAYQ_DUMPFILE, NULL, NULL },
	{ "playq.xmms",			"no",		valid_bool,	NULL },
#ifdef BUILD_SCROBBLER
	{ "scrobbler.dumpfile",		CONFHOMEDIR "scrobbler.queue", NULL, NULL },
	{ "scrobbler.hostname",		"post.audioscrobbler.com", NULL, NULL },
	{ "scrobbler.password",		"",		valid_md5,	NULL },
	{ "scrobbler.username",		"",		NULL,		NULL },
#endif /* BUILD_SCROBBLER */
	{ "vfs.cache",			"no",		valid_bool,	NULL },
	{ "vfs.dir.hide_dotfiles",	"yes",		valid_bool,	NULL },
#ifdef G_OS_UNIX
	{ "vfs.lockup.chroot",		"",		NULL,		NULL },
	{ "vfs.lockup.user",		"",		NULL,		NULL },
#endif /* G_OS_UNIX */
};
/**
 * @brief The amount of configuration switches available.
 */
#define NUM_SWITCHES (sizeof configlist / sizeof(struct config_entry))

/**
 * @brief Search for an entry in the configlist by name.
 */
static struct config_entry *
config_search(const char *opt)
{
	unsigned int i;
	int c;

	for (i = 0; i < NUM_SWITCHES; i++) {
		c = strcmp(opt, configlist[i].name);

		if (c == 0)
			/* Found it */
			return (&configlist[i]);
		else if (c < 0)
			/* We're already too far */
			break;
	}

	/* Not found */
	return (NULL);
}

/**
 * @brief Set a value to a configuration switch
 */
static int
config_setopt(const char *opt, char *val)
{
	struct config_entry *ent;
	char *newval = NULL;

	if ((ent = config_search(opt)) == NULL)
		return (-1);

	if (strcmp(val, ent->defval) != 0) {
		/* Just unset the value when it's the default */

		if (ent->validator != NULL) {
			/* Do not set invalid values */
			if (ent->validator(val) != 0)
				return (-1);
		}
		newval = g_strdup(val);
	}

	/* Free the current contents */
	g_free(ent->curval);
	ent->curval = newval;

	return (0);
}

/*
 * Public configuration file functions
 */

void
config_load(const char *file, int expand)
{
	FILE *fio;
	char fbuf[512]; /* Should be long enough */
	char *split;

	if (expand)
		fio = vfs_fopen(file, "r");
	else
		fio = fopen(file, "r");
	if (fio == NULL)
		return;

	while (vfs_fgets(fbuf, sizeof fbuf, fio) == 0) {
		/* Split at the = and set the option */
		split = strchr(fbuf, '=');
		if (split != NULL) {
			/* Split the option and value */
			*split++ = '\0';
			config_setopt(fbuf, split);
		}
	}

	fclose(fio);
}

const char *
config_getopt(const char *opt)
{
	struct config_entry *ent;

	ent = config_search(opt);
	g_assert(ent != NULL);

	/* Return the default if it is unset */
	return (ent->curval ? ent->curval : ent->defval);
}

int
config_getopt_bool(const char *val)
{
	int bval;

	bval = string_to_bool(config_getopt(val));
	g_assert(bval != -1);
	return (bval);
}

int
config_getopt_color(const char *val)
{
	int col;

	col = gui_draw_color_number(config_getopt(val));
	g_assert(col >= -1);
	return (col);
}

int
config_getopt_percentage(const char *val)
{
	return strtoul(config_getopt(val), NULL, 10);
}
