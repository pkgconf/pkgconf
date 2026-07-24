/*
 * index.c
 * sorted-array index with bsearch lookup and ordered insert/remove
 *
 * SPDX-License-Identifier: pkgconf
 *
 * Copyright (c) 2026 pkgconf authors (see AUTHORS).
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
 * libpkgconf `index` module
 * =========================
 *
 * The `index` module maintains a sorted array of opaque entry pointers, so that
 * membership tests are a binary search rather than a linear scan.  It is the
 * shared implementation behind the package cache and the fragment dedup cursor.
 */

/*
 * !doc
 *
 * .. c:function:: void *pkgconf_index_lookup(const pkgconf_index_t *index, const void *key, pkgconf_index_cmp_func_t keycmp)
 *
 *    Binary-searches the index for an entry matching `key`.  `keycmp` compares
 *    the key against a stored entry (``keycmp(key, entry)``), returning the
 *    usual negative/zero/positive ordering.  Returns the matching entry or NULL.
 */
void *
pkgconf_index_lookup(const pkgconf_index_t *index, const void *key, pkgconf_index_cmp_func_t keycmp)
{
	size_t lo = 0, hi = index->count;

	while (lo < hi)
	{
		size_t mid = lo + (hi - lo) / 2;
		int c = keycmp(key, index->entries[mid]);

		if (c < 0)
			hi = mid;
		else if (c > 0)
			lo = mid + 1;
		else
			return index->entries[mid];
	}

	return NULL;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_index_insert(pkgconf_index_t *index, void *entry)
 *
 *    Inserts `entry` while keeping the index sorted by its `compare` function.
 *    The backing array grows geometrically.
 *
 *    :return: true on success, false on allocation failure.
 */
bool
pkgconf_index_insert(pkgconf_index_t *index, void *entry)
{
	size_t lo = 0, hi = index->count;

	if (index->count == index->alloc)
	{
		size_t newalloc = index->alloc != 0 ? index->alloc * 2 : 16;
		void **newentries = pkgconf_reallocarray(index->entries, newalloc, sizeof(void *));

		if (newentries == NULL)
			return false;

		index->entries = newentries;
		index->alloc = newalloc;
	}

	while (lo < hi)
	{
		size_t mid = lo + (hi - lo) / 2;

		if (index->compare(index->entries[mid], entry) < 0)
			lo = mid + 1;
		else
			hi = mid;
	}

	memmove(&index->entries[lo + 1], &index->entries[lo], (index->count - lo) * sizeof(void *));
	index->entries[lo] = entry;
	index->count++;

	return true;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_index_remove(pkgconf_index_t *index, void *entry)
 *
 *    Removes `entry` from the index.  The entry is located by its sort key and
 *    then matched by identity across any run of equal-keyed entries, so indexes
 *    that contain duplicate keys remove exactly the requested pointer.
 */
void
pkgconf_index_remove(pkgconf_index_t *index, void *entry)
{
	size_t lo = 0, hi = index->count;

	while (lo < hi)
	{
		size_t mid = lo + (hi - lo) / 2;
		int c = index->compare(index->entries[mid], entry);

		if (c < 0)
			lo = mid + 1;
		else if (c > 0)
			hi = mid;
		else
		{
			/* mid has an equal key; walk the equal-key run to find `entry` itself */
			size_t i = mid;

			while (i > 0 && index->compare(index->entries[i - 1], entry) == 0)
				i--;

			for (; i < index->count && index->compare(index->entries[i], entry) == 0; i++)
			{
				if (index->entries[i] == entry)
				{
					memmove(&index->entries[i], &index->entries[i + 1], (index->count - i - 1) * sizeof(void *));
					index->count--;
					return;
				}
			}

			return;
		}
	}
}
