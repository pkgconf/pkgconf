/*
 * path.c
 * filesystem path management
 *
 * Copyright (c) 2016 pkgconf authors (see AUTHORS).
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

void
pkgconf_path_add(const char *text, pkgconf_list_t *dirlist)
{
	pkgconf_path_t *node;

	node = calloc(sizeof(pkgconf_path_t), 1);
	node->path = strdup(text);

	pkgconf_node_insert_tail(&node->lnode, node, dirlist);
}

size_t
pkgconf_path_split(const char *text, pkgconf_list_t *dirlist)
{
	size_t count = 0;
	char *workbuf, *p, *iter;

	if (text == NULL)
		return 0;

	iter = workbuf = strdup(text);
	while ((p = strtok(iter, PKG_CONFIG_PATH_SEP_S)) != NULL)
	{
		pkgconf_path_add(p, dirlist);

		count++, iter = NULL;
	}
	free(workbuf);

	return count;
}

size_t
pkgconf_path_build_from_environ(const char *environ, const char *fallback, pkgconf_list_t *dirlist)
{
	const char *data;

	data = getenv(environ);
	if (data != NULL)
		return pkgconf_path_split(data, dirlist);

	if (fallback != NULL)
		return pkgconf_path_split(fallback, dirlist);

	/* no fallback and no environment variable, thusly no nodes added */
	return 0;
}

bool
pkgconf_path_match_list(const char *path, pkgconf_list_t *dirlist)
{
	pkgconf_node_t *n = NULL;

	PKGCONF_FOREACH_LIST_ENTRY(dirlist->head, n)
	{
		pkgconf_path_t *pnode = n->data;

		if (!strcmp(pnode->path, path))
			return true;
	}

	return false;
}
