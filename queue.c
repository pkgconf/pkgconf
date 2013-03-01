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

#include "pkg.h"
#include "bsdstubs.h"

typedef struct {
	pkg_node_t iter;
	char *package;
} pkg_queue_t;

void
pkg_queue_push(pkg_list_t *list, const char *package)
{
	pkg_queue_t *pkgq = calloc(sizeof(pkg_queue_t), 1);

	pkgq->package = strdup(package);
	pkg_node_insert_tail(&pkgq->iter, pkgq, list);
}

bool
pkg_queue_compile(pkg_t *world, pkg_list_t *list)
{
	pkg_node_t *iter;

	PKG_FOREACH_LIST_ENTRY(list->head, iter)
	{
		pkg_queue_t *pkgq;

		pkgq = iter->data;
		pkg_dependency_parse(world, &world->requires, pkgq->package);
	}

	return (world->requires.head != NULL);
}

void
pkg_queue_free(pkg_list_t *list)
{
	pkg_node_t *node, *tnode;

	PKG_FOREACH_LIST_ENTRY_SAFE(list->head, tnode, node)
	{
		pkg_queue_t *pkgq = node->data;

		free(pkgq->package);
		free(pkgq);
	}
}

static inline unsigned int
pkg_queue_verify(pkg_t *world, pkg_list_t *list, int maxdepth, unsigned int flags)
{
	if (!pkg_queue_compile(world, list))
		return PKG_ERRF_DEPGRAPH_BREAK;

	return pkg_verify_graph(world, maxdepth, flags);
}

bool
pkg_queue_apply(pkg_list_t *list, pkg_queue_apply_func_t func, int maxdepth, unsigned int flags, void *data)
{
	pkg_t world = {
		.id = "world",
		.realname = "virtual world package",
		.flags = PKG_PROPF_VIRTUAL,
	};

	/* if maxdepth is one, then we will not traverse deeper than our virtual package. */
	if (!maxdepth)
		maxdepth = -1;

	if (pkg_queue_verify(&world, list, maxdepth, flags) != PKG_ERRF_OK)
		return false;

	if (!func(&world, data, maxdepth, flags))
	{
		pkg_free(&world);
		return false;
	}

	pkg_free(&world);

	return true;
}

bool
pkg_queue_validate(pkg_list_t *list, int maxdepth, unsigned int flags)
{
	bool retval = true;
	pkg_t world = {
		.id = "world",
		.realname = "virtual world package",
		.flags = PKG_PROPF_VIRTUAL,
	};

	/* if maxdepth is one, then we will not traverse deeper than our virtual package. */
	if (!maxdepth)
		maxdepth = -1;

	if (pkg_queue_verify(&world, list, maxdepth, flags) != PKG_ERRF_OK)
		retval = false;

	pkg_free(&world);

	return retval;
}
