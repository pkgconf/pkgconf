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

#include <libpkgconf/config.h>
#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>

#ifdef _WIN32
#	define strcasecmp _stricmp
#endif

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
pkgconf_cross_personality_t *
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

#ifndef PKGCONF_LITE
static bool
valid_triplet(const char *triplet)
{
	const char *c = triplet;

	for (; *c; c++)
		if (!isalnum(*c) && *c != '-' && *c != '_')
			return false;

	return true;
}

typedef void (*personality_keyword_func_t)(pkgconf_cross_personality_t *p, const char *keyword, const size_t lineno, const ptrdiff_t offset, char *value);
typedef struct {
	const char *keyword;
	const personality_keyword_func_t func;
	const ptrdiff_t offset;
} personality_keyword_pair_t;

static void
personality_copy_func(pkgconf_cross_personality_t *p, const char *keyword, const size_t lineno, const ptrdiff_t offset, char *value)
{
	(void) keyword;
	(void) lineno;

	char **dest = (char **)((char *) p + offset);
	*dest = strdup(value);
}

static void
personality_fragment_func(pkgconf_cross_personality_t *p, const char *keyword, const size_t lineno, const ptrdiff_t offset, char *value)
{
	(void) keyword;
	(void) lineno;

	pkgconf_list_t *dest = (pkgconf_list_t *)((char *) p + offset);
	pkgconf_path_split(value, dest, false);
}

/* keep in alphabetical order! */
static const personality_keyword_pair_t personality_keyword_pairs[] = {
	{"DefaultSearchPaths", personality_fragment_func, offsetof(pkgconf_cross_personality_t, dir_list)},
	{"SysrootDir", personality_copy_func, offsetof(pkgconf_cross_personality_t, sysroot_dir)},
	{"SystemIncludePaths", personality_fragment_func, offsetof(pkgconf_cross_personality_t, filter_includedirs)},
	{"SystemLibraryPaths", personality_fragment_func, offsetof(pkgconf_cross_personality_t, filter_libdirs)},
	{"Triplet", personality_copy_func, offsetof(pkgconf_cross_personality_t, name)},
};

static int
personality_keyword_pair_cmp(const void *key, const void *ptr)
{
	const personality_keyword_pair_t *pair = ptr;
	return strcasecmp(key, pair->keyword);
}

static void
personality_keyword_set(pkgconf_cross_personality_t *p, const size_t lineno, const char *keyword, char *value)
{
	const personality_keyword_pair_t *pair = bsearch(keyword,
		personality_keyword_pairs, PKGCONF_ARRAY_SIZE(personality_keyword_pairs),
		sizeof(personality_keyword_pair_t), personality_keyword_pair_cmp);

	if (pair == NULL || pair->func == NULL)
		return;

	pair->func(p, keyword, lineno, pair->offset, value);
}

static const pkgconf_parser_operand_func_t personality_parser_ops[] = {
	[':'] = (pkgconf_parser_operand_func_t) personality_keyword_set
};

static void personality_warn_func(void *p, const char *fmt, ...) PRINTFLIKE(2, 3);

static void
personality_warn_func(void *p, const char *fmt, ...)
{
	va_list va;

	(void) p;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

static pkgconf_cross_personality_t *
load_personality_with_path(const char *path, const char *triplet)
{
	char pathbuf[PKGCONF_ITEM_SIZE];
	FILE *f;
	pkgconf_cross_personality_t *p;

	/* if triplet is null, assume that path is a direct path to the personality file */
	if (triplet != NULL)
		snprintf(pathbuf, sizeof pathbuf, "%s/%s.personality", path, triplet);
	else
		pkgconf_strlcpy(pathbuf, path, sizeof pathbuf);

	f = fopen(pathbuf, "r");
	if (f == NULL)
		return NULL;

	p = calloc(sizeof(pkgconf_cross_personality_t), 1);
	if (triplet != NULL)
		p->name = strdup(triplet);
	pkgconf_parser_parse(f, p, personality_parser_ops, personality_warn_func, pathbuf);

	return p;
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
	pkgconf_list_t plist = PKGCONF_LIST_INITIALIZER;
	pkgconf_node_t *n;
	pkgconf_cross_personality_t *out = NULL;

	out = load_personality_with_path(triplet, NULL);
	if (out != NULL)
		return out;

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
#endif
