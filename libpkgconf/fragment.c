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

#include <libpkgconf/libpkgconf.h>

static inline char *
pkgconf_fragment_copy_munged(const char *source, unsigned int flags)
{
	char mungebuf[PKGCONF_BUFSIZE];
	char *sysroot_dir;

	if (!(flags & PKGCONF_PKG_PKGF_MUNGE_SYSROOT_PREFIX))
		return strdup(source);

	sysroot_dir = pkgconf_tuple_find_global("pc_sysrootdir");

	if (*source != '/' ||
		(sysroot_dir != NULL && !strncmp(sysroot_dir, source, strlen(sysroot_dir))))
		return strdup(source);

	strlcpy(mungebuf, sysroot_dir, sizeof mungebuf);
	strlcat(mungebuf, source, sizeof mungebuf);

	return strdup(mungebuf);
}

void
pkgconf_fragment_add(pkgconf_list_t *list, const char *string, unsigned int flags)
{
	pkgconf_fragment_t *frag;

	if (*string == '-' && strncmp(string, "-lib:", 5) && strncmp(string, "-framework", 10))
	{
		frag = calloc(sizeof(pkgconf_fragment_t), 1);

		frag->type = *(string + 1);
		frag->data = pkgconf_fragment_copy_munged(string + 2, flags);
	}
	else
	{
		if (list->tail != NULL && list->tail->data != NULL)
		{
			pkgconf_fragment_t *parent = list->tail->data;

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

		frag = calloc(sizeof(pkgconf_fragment_t), 1);

		frag->type = 0;
		frag->data = strdup(string);
	}

	pkgconf_node_insert_tail(&frag->iter, frag, list);
}

static inline pkgconf_fragment_t *
pkgconf_fragment_lookup(pkgconf_list_t *list, pkgconf_fragment_t *base)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY_REVERSE(list->tail, node)
	{
		pkgconf_fragment_t *frag = node->data;

		if (base->type != frag->type)
			continue;

		if (!strcmp(base->data, frag->data))
			return frag;
	}

	return NULL;
}

static inline bool
pkgconf_fragment_can_merge_back(pkgconf_fragment_t *base, unsigned int flags, bool is_private)
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
pkgconf_fragment_can_merge(pkgconf_fragment_t *base, unsigned int flags, bool is_private)
{
	(void) flags;

	if (is_private)
		return false;

	if (!strncmp(base->data, "-framework", 10))
		return false;

	return true;
}

static inline pkgconf_fragment_t *
pkgconf_fragment_exists(pkgconf_list_t *list, pkgconf_fragment_t *base, unsigned int flags, bool is_private)
{
	if (!pkgconf_fragment_can_merge_back(base, flags, is_private))
		return NULL;

	if (!pkgconf_fragment_can_merge(base, flags, is_private))
		return NULL;

	return pkgconf_fragment_lookup(list, base);
}

void
pkgconf_fragment_copy(pkgconf_list_t *list, pkgconf_fragment_t *base, unsigned int flags, bool is_private)
{
	pkgconf_fragment_t *frag;

	if ((frag = pkgconf_fragment_exists(list, base, flags, is_private)) != NULL)
		pkgconf_fragment_delete(list, frag);
	else if (!is_private && !pkgconf_fragment_can_merge_back(base, flags, is_private) && (pkgconf_fragment_lookup(list, base) != NULL))
		return;

	frag = calloc(sizeof(pkgconf_fragment_t), 1);

	frag->type = base->type;
	frag->data = strdup(base->data);

	pkgconf_node_insert_tail(&frag->iter, frag, list);
}

void
pkgconf_fragment_delete(pkgconf_list_t *list, pkgconf_fragment_t *node)
{
	pkgconf_node_delete(&node->iter, list);

	free(node->data);
	free(node);
}

void
pkgconf_fragment_free(pkgconf_list_t *list)
{
	pkgconf_node_t *node, *next;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(list->head, next, node)
	{
		pkgconf_fragment_t *frag = node->data;

		free(frag->data);
		free(frag);
	}
}

void
pkgconf_fragment_parse(pkgconf_list_t *list, pkgconf_list_t *vars, const char *value, unsigned int flags)
{
	int i, argc;
	char **argv;
	char *repstr = pkgconf_tuple_parse(vars, value);

	pkgconf_argv_split(repstr, &argc, &argv);

	for (i = 0; i < argc; i++)
		pkgconf_fragment_add(list, argv[i], flags);

	pkgconf_argv_free(argv);
	free(repstr);
}
