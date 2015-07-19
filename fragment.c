/*
 * fragment.c
 * Management of fragment lists.
 *
 * Copyright (c) 2012, 2013, 2014 pkgconf authors (see AUTHORS).
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

static inline char *
pkg_fragment_copy_munged(const char *source, unsigned int flags)
{
	char mungebuf[PKG_BUFSIZE];
	char *sysroot_dir;

	if (!(flags & PKGF_MUNGE_SYSROOT_PREFIX))
		return strdup(source);

	sysroot_dir = pkg_tuple_find_global("pc_sysrootdir");

	if (*source != '/' ||
		(sysroot_dir != NULL && !strncmp(sysroot_dir, source, strlen(sysroot_dir))))
		return strdup(source);

	strlcpy(mungebuf, sysroot_dir, sizeof mungebuf);
	strlcat(mungebuf, source, sizeof mungebuf);

	return strdup(mungebuf);
}

void
pkg_fragment_add(pkg_list_t *list, const char *string, unsigned int flags)
{
	pkg_fragment_t *frag;

	if (*string == '-' && strncmp(string, "-lib:", 5) && strncmp(string, "-framework", 10))
	{
		frag = calloc(sizeof(pkg_fragment_t), 1);

		frag->type = *(string + 1);
		frag->data = pkg_fragment_copy_munged(string + 2, flags);
	}
	else
	{
		if (list->tail != NULL && list->tail->data != NULL)
		{
			pkg_fragment_t *parent = list->tail->data;

			if (!strncmp(parent->data, "-framework", 10))
			{
				size_t len = strlen(parent->data) + strlen(string) + 2;
				char *newdata;

				newdata = malloc(len);
				strlcpy(newdata, parent->data, len);
				strlcat(newdata, " ", len);
				strlcat(newdata, string, len);

				free(parent->data);
				parent->data = newdata;

				return;
			}
		}

		frag = calloc(sizeof(pkg_fragment_t), 1);

		frag->type = 0;
		frag->data = strdup(string);
	}

	pkg_node_insert_tail(&frag->iter, frag, list);
}

static inline pkg_fragment_t *
pkg_fragment_lookup(pkg_list_t *list, pkg_fragment_t *base)
{
	pkg_node_t *node;

	PKG_FOREACH_LIST_ENTRY_REVERSE(list->tail, node)
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
pkg_fragment_can_merge_back(pkg_fragment_t *base, unsigned int flags, bool is_private)
{
	(void) flags;

	if (base->type == 'l')
	{
		if (is_private)
			return false;

		return true;
	}

	if (base->type == 'F')
		return false;
	if (base->type == 'L')
		return false;
	if (base->type == 'I')
		return false;

	return true;
}

static inline bool
pkg_fragment_can_merge(pkg_fragment_t *base, unsigned int flags, bool is_private)
{
	(void) flags;

	if (is_private)
		return false;

	if (!strncmp(base->data, "-framework", 10))
		return false;

	return true;
}

static inline pkg_fragment_t *
pkg_fragment_exists(pkg_list_t *list, pkg_fragment_t *base, unsigned int flags, bool is_private)
{
	if (!pkg_fragment_can_merge_back(base, flags, is_private))
		return NULL;

	if (!pkg_fragment_can_merge(base, flags, is_private))
		return NULL;

	return pkg_fragment_lookup(list, base);
}

void
pkg_fragment_copy(pkg_list_t *list, pkg_fragment_t *base, unsigned int flags, bool is_private)
{
	pkg_fragment_t *frag;

	if ((frag = pkg_fragment_exists(list, base, flags, is_private)) != NULL)
		pkg_fragment_delete(list, frag);
	else if (!is_private && !pkg_fragment_can_merge_back(base, flags, is_private) && (pkg_fragment_lookup(list, base) != NULL))
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
pkg_fragment_parse(pkg_list_t *list, pkg_list_t *vars, const char *value, unsigned int flags)
{
	int i, argc;
	char **argv;
	char *repstr = pkg_tuple_parse(vars, value);

	pkg_argv_split(repstr, &argc, &argv);

	for (i = 0; i < argc; i++)
		pkg_fragment_add(list, argv[i], flags);

	pkg_argv_free(argv);
	free(repstr);
}
