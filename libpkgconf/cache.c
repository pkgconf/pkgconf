/*
 * cache.c
 * package object cache
 *
 * SPDX-License-Identifier: pkgconf
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

#include <assert.h>

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

/* order two cached packages by id */
static int
cache_cmp(const void *a, const void *b)
{
	return strcmp(((const pkgconf_pkg_t *) a)->id, ((const pkgconf_pkg_t *) b)->id);
}

/* order an id string against a cached package */
static int
cache_keycmp(const void *key, const void *entry)
{
	return strcmp((const char *) key, ((const pkgconf_pkg_t *) entry)->id);
}

/* The cache table lives in two ABI-frozen client fields (cache_table,
 * cache_count).  These wrap them in a pkgconf_index_t view so the shared index
 * code can operate on them, writing any changes back afterwards. */
static inline pkgconf_index_t
cache_index(pkgconf_client_t *client)
{
	pkgconf_index_t index = {
		.entries = (void **) client->cache_table,
		.count = client->cache_count,
		.alloc = client->cache_count,
		.compare = cache_cmp,
	};
	return index;
}

static inline void
cache_index_store(pkgconf_client_t *client, const pkgconf_index_t *index)
{
	client->cache_table = (pkgconf_pkg_t **) index->entries;
	client->cache_count = index->count;
}

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
	pkgconf_index_t index = cache_index(client);
	pkgconf_pkg_t *pkg = pkgconf_index_lookup(&index, id, cache_keycmp);

	if (pkg != NULL)
	{
		PKGCONF_TRACE(client, "found: %s @%p", id, pkg);
		return pkgconf_pkg_ref(client, pkg);
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

	pkgconf_pkg_t *cached_pkg = pkgconf_cache_lookup(client, pkg->id);
	if (cached_pkg != NULL)
	{
		pkgconf_pkg_unref(client, cached_pkg);
		return;
	}

	pkgconf_pkg_ref(client, pkg);

	/* mark package as cached */
	pkg->flags |= PKGCONF_PKG_PROPF_CACHED;

	pkgconf_index_t index = cache_index(client);
	if (!pkgconf_index_insert(&index, pkg))
	{
		/* out of memory: roll back adding to cache and bail */
		pkg->flags &= ~PKGCONF_PKG_PROPF_CACHED;
		pkgconf_pkg_unref(client, pkg);
		return;
	}
	cache_index_store(client, &index);

	PKGCONF_TRACE(client, "added @%p to cache", pkg);
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
	if (client->cache_table == NULL)
		return;

	if (pkg == NULL)
		return;

	if (!(pkg->flags & PKGCONF_PKG_PROPF_CACHED))
		return;

	PKGCONF_TRACE(client, "removed @%p from cache", pkg);

	pkgconf_index_t index = cache_index(client);
	pkgconf_pkg_t *found = pkgconf_index_lookup(&index, pkg->id, cache_keycmp);

	if (found == NULL)
		return;

	found->flags &= ~PKGCONF_PKG_PROPF_CACHED;

	/* remove from the index (which reads found->id) and publish the new table
	 * before dropping our reference: pkgconf_pkg_unref() may free `found`, and
	 * freeing a package can re-enter pkgconf_cache_remove(). */
	pkgconf_index_remove(&index, found);
	cache_index_store(client, &index);

	if (client->cache_count == 0)
	{
		free(client->cache_table);
		client->cache_table = NULL;
	}

	pkgconf_pkg_unref(client, found);
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
	if (client->cache_table == NULL)
		return;

	while (client->cache_count > 0)
		pkgconf_cache_remove(client, client->cache_table[0]);

	free(client->cache_table);
	client->cache_table = NULL;
	client->cache_count = 0;

	PKGCONF_TRACE(client, "cleared package cache");
}
