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

struct pkgconf_fragment_check {
	char *token;
	size_t len;
};

static inline bool
pkgconf_fragment_is_unmergeable(const char *string)
{
	static struct pkgconf_fragment_check check_fragments[] = {
		{"-framework", 10},
		{"-isystem", 8},
	};

	if (*string != '-')
		return true;

	for (size_t i = 0; i < PKGCONF_ARRAY_SIZE(check_fragments); i++)
		if (!strncmp(string, check_fragments[i].token, check_fragments[i].len))
			return true;

	return false;
}

static inline bool
pkgconf_fragment_should_munge(const char *string, const char *sysroot_dir)
{
	static struct pkgconf_fragment_check check_fragments[] = {
		{"-isystem", 8},
	};

	if (*string != '/')
		return false;

	if (sysroot_dir != NULL && strncmp(sysroot_dir, string, strlen(sysroot_dir)))
		return true;

	for (size_t i = 0; i < PKGCONF_ARRAY_SIZE(check_fragments); i++)
		if (!strncmp(string, check_fragments[i].token, check_fragments[i].len))
			return true;

	return false;
}

static inline bool
pkgconf_fragment_is_special(const char *string)
{
	if (*string != '-')
		return true;

	if (!strncmp(string, "-lib:", 5))
		return true;

	return pkgconf_fragment_is_unmergeable(string);
}

static inline void
pkgconf_fragment_munge(const pkgconf_client_t *client, char *buf, size_t buflen, const char *source, const char *sysroot_dir)
{
	*buf = '\0';

	if (sysroot_dir == NULL)
		sysroot_dir = pkgconf_tuple_find_global(client, "pc_sysrootdir");

	if (pkgconf_fragment_should_munge(source, sysroot_dir))
		strlcat(buf, sysroot_dir, buflen);

	strlcat(buf, source, buflen);
}

static inline char *
pkgconf_fragment_copy_munged(const pkgconf_client_t *client, const char *source)
{
	char mungebuf[PKGCONF_BUFSIZE];

	if (client->sysroot_dir == NULL)
		return strdup(source);

	pkgconf_fragment_munge(client, mungebuf, sizeof mungebuf, source, client->sysroot_dir);

	return strdup(mungebuf);
}

void
pkgconf_fragment_add(const pkgconf_client_t *client, pkgconf_list_t *list, const char *string)
{
	pkgconf_fragment_t *frag;

	if (*string == '\0')
		return;

	if (!pkgconf_fragment_is_special(string))
	{
		frag = calloc(sizeof(pkgconf_fragment_t), 1);

		frag->type = *(string + 1);
		frag->data = pkgconf_fragment_copy_munged(client, string + 2);
	}
	else
	{
		char mungebuf[PKGCONF_BUFSIZE];

		if (list->tail != NULL && list->tail->data != NULL)
		{
			pkgconf_fragment_t *parent = list->tail->data;

			if (pkgconf_fragment_is_unmergeable(parent->data))
			{
				size_t len;
				char *newdata;

				pkgconf_fragment_munge(client, mungebuf, sizeof mungebuf, string, NULL);

				len = strlen(parent->data) + strlen(mungebuf) + 2;
				newdata = malloc(len);

				strlcpy(newdata, parent->data, len);
				strlcat(newdata, " ", len);
				strlcat(newdata, mungebuf, len);

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
pkgconf_fragment_lookup(pkgconf_list_t *list, const pkgconf_fragment_t *base)
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
pkgconf_fragment_can_merge_back(const pkgconf_fragment_t *base, unsigned int flags, bool is_private)
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
pkgconf_fragment_can_merge(const pkgconf_fragment_t *base, unsigned int flags, bool is_private)
{
	(void) flags;

	if (is_private)
		return false;

	return pkgconf_fragment_is_unmergeable(base->data);
}

static inline pkgconf_fragment_t *
pkgconf_fragment_exists(pkgconf_list_t *list, const pkgconf_fragment_t *base, unsigned int flags, bool is_private)
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
pkgconf_fragment_parse(const pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_list_t *vars, const char *value)
{
	int i, argc;
	char **argv;
	char *repstr = pkgconf_tuple_parse(client, vars, value);

	pkgconf_argv_split(repstr, &argc, &argv);

	for (i = 0; i < argc; i++)
		pkgconf_fragment_add(client, list, argv[i]);

	pkgconf_argv_free(argv);
	free(repstr);
}
