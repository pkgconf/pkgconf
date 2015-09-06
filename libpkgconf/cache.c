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

#include <libpkgconf/libpkgconf.h>

static pkgconf_list_t pkg_cache = PKGCONF_LIST_INITIALIZER;

/*
 * pkgconf_cache_lookup(id)
 *
 * looks up a package in the cache given an 'id' atom,
 * such as 'gtk+-3.0' and returns the already loaded version
 * if present.
 */
pkgconf_pkg_t *
pkgconf_cache_lookup(const char *id)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(pkg_cache.head, node)
	{
		pkgconf_pkg_t *pkg = node->data;

		if (!strcmp(pkg->id, id))
			return pkgconf_pkg_ref(pkg);
	}

	return NULL;
}

/*
 * pkgconf_cache_add(pkg)
 *
 * adds an entry for the package to the package cache.
 * the cache entry must be removed if the package is freed.
 */
void
pkgconf_cache_add(pkgconf_pkg_t *pkg)
{
	if (pkg == NULL)
		return;

	pkgconf_pkg_ref(pkg);
	pkgconf_node_insert(&pkg->cache_iter, pkg, &pkg_cache);
}

/*
 * pkgconf_cache_remove(pkg)
 *
 * deletes a package from the cache entry.
 */
void
pkgconf_cache_remove(pkgconf_pkg_t *pkg)
{
	if (pkg == NULL)
		return;

	pkgconf_node_delete(&pkg->cache_iter, &pkg_cache);
}

void
pkgconf_cache_free(void)
{
	pkgconf_node_t *iter, *iter2;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(pkg_cache.head, iter2, iter)
	{
		pkgconf_pkg_t *pkg = iter->data;
		pkgconf_pkg_free(pkg);
	}
}
