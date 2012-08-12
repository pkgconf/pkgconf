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

pkg_queue_t *
pkg_queue_push(pkg_queue_t *parent, const char *package)
{
	pkg_queue_t *pkgq = calloc(sizeof(pkg_queue_t), 1);

	pkgq->package = strdup(package);
	pkgq->prev = parent;
	if (pkgq->prev != NULL)
		pkgq->prev->next = pkgq;

	return pkgq;
}

bool
pkg_queue_compile(pkg_t *world, pkg_queue_t *head)
{
	pkg_queue_t *pkgq;

	PKG_FOREACH_LIST_ENTRY(head, pkgq)
	{
		pkg_dependency_t *pkgdep;

		pkgdep = pkg_dependency_parse(world, pkgq->package);
		if (pkgdep != NULL)
			world->requires = pkg_dependency_append(world->requires, pkgdep);
		else
			return false;
	}

	return true;
}

void
pkg_queue_free(pkg_queue_t *head)
{
	pkg_queue_t *pkgq, *next_pkgq;

	PKG_FOREACH_LIST_ENTRY_SAFE(head, next_pkgq, pkgq)
	{
		free(pkgq->package);
		free(pkgq);
	}
}

static inline unsigned int
pkg_queue_verify(pkg_t *world, pkg_queue_t *head, int maxdepth, unsigned int flags)
{
	if (!pkg_queue_compile(world, head))
		return PKG_ERRF_DEPGRAPH_BREAK;

	return pkg_verify_graph(world, maxdepth, flags);
}

bool
pkg_queue_apply(pkg_queue_t *head, pkg_queue_apply_func_t func, int maxdepth, unsigned int flags, void *data)
{
	pkg_t world = {
		.id = "world",
		.realname = "virtual world package",
		.flags = PKG_PROPF_VIRTUAL,
	};

	/* if maxdepth is one, then we will not traverse deeper than our virtual package. */
	if (!maxdepth)
		maxdepth = -1;

	if (pkg_queue_verify(&world, head, maxdepth, flags) != PKG_ERRF_OK)
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
pkg_queue_validate(pkg_queue_t *head, int maxdepth, unsigned int flags)
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

	if (pkg_queue_verify(&world, head, maxdepth, flags) != PKG_ERRF_OK)
		retval = false;

	pkg_free(&world);

	return retval;
}
