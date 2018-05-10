/*
 * personality.c
 * libpkgconf cross-compile personality database
 *
 * Copyright (c) 2018 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>
#include <libpkgconf/config.h>

static bool default_personality_init = false;
static pkgconf_cross_personality_t default_personality = {
	.name = "default",
};

static inline void
build_default_search_path(pkgconf_list_t* dirlist)
{
#ifdef _WIN32
	char namebuf[MAX_PATH];
	char outbuf[MAX_PATH];
	char *p;

	int sizepath = GetModuleFileName(NULL, namebuf, sizeof namebuf);
	char * winslash;
	namebuf[sizepath] = '\0';
	while ((winslash = strchr (namebuf, '\\')) != NULL)
	{
		*winslash = '/';
	}
	p = strrchr(namebuf, '/');
	if (p == NULL)
		pkgconf_path_split(PKG_DEFAULT_PATH, dirlist, true);

	*p = '\0';
	pkgconf_strlcpy(outbuf, namebuf, sizeof outbuf);
	pkgconf_strlcat(outbuf, "/", sizeof outbuf);
	pkgconf_strlcat(outbuf, "../lib/pkgconfig", sizeof outbuf);
	pkgconf_path_add(outbuf, dirlist, true);
	pkgconf_strlcpy(outbuf, namebuf, sizeof outbuf);
	pkgconf_strlcat(outbuf, "/", sizeof outbuf);
	pkgconf_strlcat(outbuf, "../share/pkgconfig", sizeof outbuf);
	pkgconf_path_add(outbuf, dirlist, true);
#elif __HAIKU__
	char **paths;
	size_t count;
	if (find_paths(B_FIND_PATH_DEVELOP_LIB_DIRECTORY, "pkgconfig", &paths, &count) == B_OK) {
		for (size_t i = 0; i < count; i++)
			pkgconf_path_add(paths[i], dirlist, true);
		free(paths);
		paths = NULL;
	}
	if (find_paths(B_FIND_PATH_DATA_DIRECTORY, "pkgconfig", &paths, &count) == B_OK) {
		for (size_t i = 0; i < count; i++)
			pkgconf_path_add(paths[i], dirlist, true);
		free(paths);
		paths = NULL;
	}
#else
	pkgconf_path_split(PKG_DEFAULT_PATH, dirlist, true);
#endif
}

/*
 * !doc
 *
 * .. c:function:: const pkgconf_cross_personality_t *pkgconf_cross_personality_default(void)
 *
 *    Returns the default cross-compile personality.
 *
 *    :rtype: pkgconf_cross_personality_t*
 *    :return: the default cross-compile personality
 */
const pkgconf_cross_personality_t *
pkgconf_cross_personality_default(void)
{
	if (default_personality_init)
		return &default_personality;

	build_default_search_path(&default_personality.dir_list);

	pkgconf_path_split(SYSTEM_LIBDIR, &default_personality.filter_libdirs, true);
	pkgconf_path_split(SYSTEM_INCLUDEDIR, &default_personality.filter_includedirs, true);

	default_personality_init = true;
	return &default_personality;
}

static bool
valid_triplet(const char *triplet)
{
	const char *c = triplet;

	for (; c != '\0'; c++)
		if (!isalnum(*c) && *c != '-')
			return false;

	return true;
}

static pkgconf_cross_personality_t *
load_personality_with_path(const char *path, const char *triplet)
{
	return NULL;
}

/*
 * !doc
 *
 * .. c:function:: pkgconf_cross_personality_t *pkgconf_cross_personality_find(const char *triplet)
 *
 *    Attempts to find a cross-compile personality given a triplet.
 *
 *    :rtype: pkgconf_cross_personality_t*
 *    :return: the default cross-compile personality
 */
pkgconf_cross_personality_t *
pkgconf_cross_personality_find(const char *triplet)
{
	pkgconf_list_t plist;
	pkgconf_node_t *n;
	pkgconf_cross_personality_t *out = NULL;

	if (!valid_triplet(triplet))
		return NULL;

	pkgconf_path_split(PERSONALITY_PATH, &plist, true);

	PKGCONF_FOREACH_LIST_ENTRY(plist.head, n)
	{
		pkgconf_path_t *pn = n->data;

		out = load_personality_with_path(pn->path, triplet);
		if (out != NULL)
			goto finish;
	}

finish:
	pkgconf_path_free(&plist);
	return out;
}
