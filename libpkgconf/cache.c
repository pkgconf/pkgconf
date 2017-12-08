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

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>

/*
 * !doc
 *
 * libpkgconf `cache` module
 * =========================
 *
 * The libpkgconf `cache` module manages a package/module object cache, allowing it to
 * avoid loading duplicate copies of a package/module.
 *
 * A cache is tied to a specific pkgconf client object, so package objects should not
 * be shared across threads.
 */

/*
 * !doc
 *
 * .. c:function:: pkgconf_pkg_t *pkgconf_cache_lookup(const pkgconf_client_t *client, const char *id)
 *
 *    Looks up a package in the cache given an `id` atom,
 *    such as ``gtk+-3.0`` and returns the already loaded version
 *    if present.
 *
 *    :param pkgconf_client_t* client: The client object to access.
 *    :param char* id: The package atom to look up in the client object's cache.
 *    :return: A package object if present, else ``NULL``.
 *    :rtype: pkgconf_pkg_t *
 */
pkgconf_pkg_t *
pkgconf_cache_lookup(pkgconf_client_t *client, const char *id)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(client->pkg_cache.head, node)
	{
		pkgconf_pkg_t *pkg = node->data;

		if (!strcmp(pkg->id, id))
		{
			PKGCONF_TRACE(client, "found: %s @%p", id, pkg);
			return pkgconf_pkg_ref(client, pkg);
		}
	}

	PKGCONF_TRACE(client, "miss: %s", id);
	return NULL;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_cache_add(pkgconf_client_t *client, pkgconf_pkg_t *pkg)
 *
 *    Adds an entry for the package to the package cache.
 *    The cache entry must be removed if the package is freed.
 *
 *    :param pkgconf_client_t* client: The client object to modify.
 *    :param pkgconf_pkg_t* pkg: The package object to add to the client object's cache.
 *    :return: nothing
 */
void
pkgconf_cache_add(pkgconf_client_t *client, pkgconf_pkg_t *pkg)
{
	if (pkg == NULL)
		return;

	pkgconf_pkg_ref(client, pkg);
	pkgconf_node_insert(&pkg->cache_iter, pkg, &client->pkg_cache);

	PKGCONF_TRACE(client, "added @%p to cache", pkg);

	/* mark package as cached */
	pkg->flags |= PKGCONF_PKG_PROPF_CACHED;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_cache_remove(pkgconf_client_t *client, pkgconf_pkg_t *pkg)
 *
 *    Deletes a package from the client object's package cache.
 *
 *    :param pkgconf_client_t* client: The client object to modify.
 *    :param pkgconf_pkg_t* pkg: The package object to remove from the client object's cache.
 *    :return: nothing
 */
void
pkgconf_cache_remove(pkgconf_client_t *client, pkgconf_pkg_t *pkg)
{
	if (pkg == NULL)
		return;

	if (!(pkg->flags & PKGCONF_PKG_PROPF_CACHED))
		return;

	PKGCONF_TRACE(client, "removed @%p from cache", pkg);

	pkgconf_node_delete(&pkg->cache_iter, &client->pkg_cache);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_cache_free(pkgconf_client_t *client)
 *
 *    Releases all resources related to a client object's package cache.
 *    This function should only be called to clear a client object's package cache,
 *    as it may release any package in the cache.
 *
 *    :param pkgconf_client_t* client: The client object to modify.
 */
void
pkgconf_cache_free(pkgconf_client_t *client)
{
	pkgconf_node_t *iter, *iter2;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(client->pkg_cache.head, iter2, iter)
	{
		pkgconf_pkg_t *pkg = iter->data;
		pkgconf_pkg_unref(client, pkg);
	}

	memset(&client->pkg_cache, 0, sizeof client->pkg_cache);

	PKGCONF_TRACE(client, "cleared package cache");
}
