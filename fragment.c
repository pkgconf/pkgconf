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

pkg_fragment_t *
pkg_fragment_append(pkg_fragment_t *head, pkg_fragment_t *tail)
{
	pkg_fragment_t *node;

	if (head == NULL)
		return tail;

	/* skip to end of list */
	PKG_FOREACH_LIST_ENTRY(head, node)
	{
		if (node->next == NULL)
			break;
	}

	node->next = tail;
	tail->prev = node;

	return head;
}

pkg_fragment_t *
pkg_fragment_add(pkg_fragment_t *head, const char *string)
{
	pkg_fragment_t *frag;

	frag = calloc(sizeof(pkg_fragment_t), 1);

	if (*string == '-' && strncmp(string, "-lib:", 5))
	{
		frag->type = *(string + 1);
		frag->data = strdup(string + 2);
	}
	else
	{
		frag->type = 0;
		frag->data = strdup(string);
	}

	return pkg_fragment_append(head, frag);
}

static inline pkg_fragment_t *
pkg_fragment_lookup(pkg_fragment_t *head, pkg_fragment_t *base)
{
	pkg_fragment_t *node;

	PKG_FOREACH_LIST_ENTRY(head, node)
	{
		if (base->type != node->type)
			continue;

		if (!strcmp(base->data, node->data))
			return node;
	}

	return NULL;
}

bool
pkg_fragment_exists(pkg_fragment_t *head, pkg_fragment_t *base)
{
	return pkg_fragment_lookup(head, base) != NULL;
}

pkg_fragment_t *
pkg_fragment_copy(pkg_fragment_t *head, pkg_fragment_t *base)
{
	pkg_fragment_t *frag;

	if ((frag = pkg_fragment_lookup(head, base)) != NULL)
	{
		if (head == frag)
			head = frag->next;

		pkg_fragment_delete(frag);
	}

	frag = calloc(sizeof(pkg_fragment_t), 1);

	frag->type = base->type;
	frag->data = strdup(base->data);

	return pkg_fragment_append(head, frag);
}

void
pkg_fragment_delete(pkg_fragment_t *node)
{
	if (node->prev != NULL)
		node->prev->next = node->next;

	if (node->next != NULL)
		node->next->prev = node->prev;

	free(node->data);
	free(node);
}

void
pkg_fragment_free(pkg_fragment_t *head)
{
	pkg_fragment_t *node, *next;

	PKG_FOREACH_LIST_ENTRY_SAFE(head, next, node)
		pkg_fragment_delete(node);
}

pkg_fragment_t *
pkg_fragment_parse(pkg_fragment_t *head, pkg_tuple_t *vars, const char *value)
{
	int i, argc;
	char **argv;
	char *repstr = pkg_tuple_parse(vars, value);

	pkg_argv_split(repstr, &argc, &argv);

	for (i = 0; i < argc; i++)
		head = pkg_fragment_add(head, argv[i]);

	pkg_argv_free(argv);
	free(repstr);

	return head;
}
