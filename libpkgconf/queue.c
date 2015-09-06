/*
 * queue.c
 * compilation of a list of packages into a world dependency set
 *
 * Copyright (c) 2012 pkgconf authors (see AUTHORS).
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

typedef struct {
	pkgconf_node_t iter;
	char *package;
} pkgconf_queue_t;

void
pkgconf_queue_push(pkgconf_list_t *list, const char *package)
{
	pkgconf_queue_t *pkgq = calloc(sizeof(pkgconf_queue_t), 1);

	pkgq->package = strdup(package);
	pkgconf_node_insert_tail(&pkgq->iter, pkgq, list);
}

bool
pkgconf_queue_compile(pkgconf_pkg_t *world, pkgconf_list_t *list)
{
	pkgconf_node_t *iter;

	PKGCONF_FOREACH_LIST_ENTRY(list->head, iter)
	{
		pkgconf_queue_t *pkgq;

		pkgq = iter->data;
		pkgconf_dependency_parse(world, &world->requires, pkgq->package);
	}

	return (world->requires.head != NULL);
}

void
pkgconf_queue_free(pkgconf_list_t *list)
{
	pkgconf_node_t *node, *tnode;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(list->head, tnode, node)
	{
		pkgconf_queue_t *pkgq = node->data;

		free(pkgq->package);
		free(pkgq);
	}
}

static inline unsigned int
pkgconf_queue_verify(pkgconf_pkg_t *world, pkgconf_list_t *list, int maxdepth, unsigned int flags)
{
	if (!pkgconf_queue_compile(world, list))
		return PKGCONF_PKG_ERRF_DEPGRAPH_BREAK;

	return pkgconf_pkg_verify_graph(world, maxdepth, flags);
}

bool
pkgconf_queue_apply(pkgconf_list_t *list, pkgconf_queue_apply_func_t func, int maxdepth, unsigned int flags, void *data)
{
	pkgconf_pkg_t world = {
		.id = "world",
		.realname = "virtual world package",
		.flags = PKGCONF_PKG_PROPF_VIRTUAL,
	};

	/* if maxdepth is one, then we will not traverse deeper than our virtual package. */
	if (!maxdepth)
		maxdepth = -1;

	if (pkgconf_queue_verify(&world, list, maxdepth, flags) != PKGCONF_PKG_ERRF_OK)
		return false;

	if (!func(&world, data, maxdepth, flags))
	{
		pkgconf_pkg_free(&world);
		return false;
	}

	pkgconf_pkg_free(&world);

	return true;
}

bool
pkgconf_queue_validate(pkgconf_list_t *list, int maxdepth, unsigned int flags)
{
	bool retval = true;
	pkgconf_pkg_t world = {
		.id = "world",
		.realname = "virtual world package",
		.flags = PKGCONF_PKG_PROPF_VIRTUAL,
	};

	/* if maxdepth is one, then we will not traverse deeper than our virtual package. */
	if (!maxdepth)
		maxdepth = -1;

	if (pkgconf_queue_verify(&world, list, maxdepth, flags) != PKGCONF_PKG_ERRF_OK)
		retval = false;

	pkgconf_pkg_free(&world);

	return retval;
}
