/*
 * cache.c
 * package object cache
 *
 * Copyright (c) 2013 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include "pkg.h"

static pkg_t *pkg_cache = NULL;

/*
 * pkg_cache_lookup(id)
 *
 * looks up a package in the cache given an 'id' atom,
 * such as 'gtk+-3.0' and returns the already loaded version
 * if present.
 */
pkg_t *
pkg_cache_lookup(const char *id)
{
	pkg_t *pkg;

	PKG_FOREACH_LIST_ENTRY(pkg_cache, pkg)
	{
		if (!strcmp(pkg->id, id))
			return pkg;
	}

	return NULL;
}

/*
 * pkg_cache_add(pkg)
 *
 * adds an entry for the package to the package cache.
 * the cache entry must be removed if the package is freed.
 */
void
pkg_cache_add(pkg_t *pkg)
{
	pkg->next = pkg_cache;
	pkg_cache = pkg;

	if (pkg->next != NULL)
		pkg->next->prev = pkg;
}

/*
 * pkg_cache_remove(pkg)
 *
 * deletes a package from the cache entry.
 */
void
pkg_cache_remove(pkg_t *pkg)
{
	if (pkg->prev != NULL)
		pkg->prev->next = pkg->next;

	if (pkg->next != NULL)
		pkg->next->prev = pkg->prev;

	if (pkg == pkg_cache)
	{
		if (pkg->prev != NULL)
			pkg_cache = pkg->prev;
		else
			pkg_cache = pkg->next;
	}
}
