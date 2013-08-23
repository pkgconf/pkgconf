/*
 * fragment.c
 * Management of fragment lists.
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

void
pkg_fragment_add(pkg_list_t *list, const char *string)
{
	pkg_fragment_t *frag;

	frag = calloc(sizeof(pkg_fragment_t), 1);

	if (*string == '-' && strncmp(string, "-lib:", 5) && strncmp(string, "-framework", 10))
	{
		frag->type = *(string + 1);
		frag->data = strdup(string + 2);
	}
	else
	{
		frag->type = 0;
		frag->data = strdup(string);
	}

	pkg_node_insert_tail(&frag->iter, frag, list);
}

static inline pkg_fragment_t *
pkg_fragment_lookup(pkg_list_t *list, pkg_fragment_t *base)
{
	pkg_node_t *node;

	PKG_FOREACH_LIST_ENTRY(list->head, node)
	{
		pkg_fragment_t *frag = node->data;

		if (base->type != frag->type)
			continue;

		if (!strcmp(base->data, frag->data))
			return frag;
	}

	return NULL;
}

static inline bool
pkg_fragment_can_merge_back(pkg_fragment_t *base, unsigned int flags)
{
	if ((flags & PKGF_MERGE_PRIVATE_FRAGMENTS) && base->type == 'l')
		return false;

	if (base->type == 'F')
		return false;

	return true;
}

static inline bool
pkg_fragment_can_merge(pkg_fragment_t *base, unsigned int flags)
{
	(void) flags;

	if (!strncmp(base->data, "-framework", 10))
		return false;

	return true;
}

static inline pkg_fragment_t *
pkg_fragment_exists(pkg_list_t *list, pkg_fragment_t *base, unsigned int flags)
{
	if (!pkg_fragment_can_merge_back(base, flags))
		return NULL;

	if (!pkg_fragment_can_merge(base, flags))
		return NULL;

	return pkg_fragment_lookup(list, base);
}

void
pkg_fragment_copy(pkg_list_t *list, pkg_fragment_t *base, unsigned int flags)
{
	pkg_fragment_t *frag;

	if ((frag = pkg_fragment_exists(list, base, flags)) != NULL)
		pkg_fragment_delete(list, frag);
	else if (!pkg_fragment_can_merge_back(base, flags) && (pkg_fragment_lookup(list, base) != NULL))
		return;

	frag = calloc(sizeof(pkg_fragment_t), 1);

	frag->type = base->type;
	frag->data = strdup(base->data);

	pkg_node_insert_tail(&frag->iter, frag, list);
}

void
pkg_fragment_delete(pkg_list_t *list, pkg_fragment_t *node)
{
	pkg_node_delete(&node->iter, list);

	free(node->data);
	free(node);
}

void
pkg_fragment_free(pkg_list_t *list)
{
	pkg_node_t *node, *next;

	PKG_FOREACH_LIST_ENTRY_SAFE(list->head, next, node)
	{
		pkg_fragment_t *frag = node->data;

		free(frag->data);
		free(frag);
	}
}

void
pkg_fragment_parse(pkg_list_t *list, pkg_list_t *vars, const char *value)
{
	int i, argc;
	char **argv;
	char *repstr = pkg_tuple_parse(vars, value);

	pkg_argv_split(repstr, &argc, &argv);

	for (i = 0; i < argc; i++)
		pkg_fragment_add(list, argv[i]);

	pkg_argv_free(argv);
	free(repstr);
}
