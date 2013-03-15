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

static pkg_list_t pkg_cache = PKG_LIST_INITIALIZER;

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
	pkg_node_t *node;

	PKG_FOREACH_LIST_ENTRY(pkg_cache.head, node)
	{
		pkg_t *pkg = node->data;

		if (!strcmp(pkg->id, id))
			return pkg_ref(pkg);
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
	if (pkg == NULL)
		return;

	pkg_ref(pkg);

	pkg_node_insert(&pkg->cache_iter, pkg, &pkg_cache);
}

/*
 * pkg_cache_remove(pkg)
 *
 * deletes a package from the cache entry.
 */
void
pkg_cache_remove(pkg_t *pkg)
{
	if (pkg == NULL)
		return;

	pkg_node_delete(&pkg->cache_iter, &pkg_cache);
}

void
pkg_cache_free(void)
{
	pkg_node_t *iter, *iter2;

	PKG_FOREACH_LIST_ENTRY_SAFE(pkg_cache.head, iter2, iter)
	{
		pkg_t *pkg = iter->data;

		pkg_free(pkg);
	}
}
