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

/*
 * !doc
 *
 * libpkgconf `path` module
 * ========================
 *
 * The `path` module provides functions for manipulating lists of paths in a cross-platform manner.  Notably,
 * it is used by the `pkgconf client` to parse the ``PKG_CONFIG_PATH``, ``PKG_CONFIG_LIBDIR`` and related environment
 * variables.
 */

/*
 * !doc
 *
 * .. c:function:: void pkgconf_path_add(const char *text, pkgconf_list_t *dirlist)
 *
 *    Adds a path node to a path list.
 *
 *    :param char* text: The path text to add as a path node.
 *    :param pkgconf_list_t* dirlist: The path list to add the path node to.
 *    :return: nothing
 */
void
pkgconf_path_add(const char *text, pkgconf_list_t *dirlist)
{
	pkgconf_path_t *node;

	node = calloc(sizeof(pkgconf_path_t), 1);
	node->path = strdup(text);

	pkgconf_node_insert_tail(&node->lnode, node, dirlist);
}

/*
 * !doc
 *
 * .. c:function:: size_t pkgconf_path_split(const char *text, pkgconf_list_t *dirlist)
 *
 *    Splits a given text input and inserts paths into a path list.
 *
 *    :param char* text: The path text to split and add as path nodes.
 *    :param pkgconf_list_t* dirlist: The path list to have the path nodes added to.
 *    :return: number of path nodes added to the path list
 *    :rtype: size_t
 */
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

/*
 * !doc
 *
 * .. c:function:: size_t pkgconf_path_build_from_environ(const char *environ, const char *fallback, pkgconf_list_t *dirlist)
 *
 *    Adds the paths specified in an environment variable to a path list.  If the environment variable is not set,
 *    an optional default set of paths is added.
 *
 *    :param char* environ: The environment variable to look up.
 *    :param char* fallback: The fallback paths to use if the environment variable is not set.
 *    :param pkgconf_list_t* dirlist: The path list to add the path nodes to.
 *    :return: number of path nodes added to the path list
 *    :rtype: size_t
 */
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

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_path_match_list(const char *path, pkgconf_list_t *dirlist)
 *
 *    Checks whether a path has a matching prefix in a path list.
 *
 *    :param char* path: The path to check against a path list.
 *    :param pkgconf_list_t* dirlist: The path list to check the path against.
 *    :return: true if the path list has a matching prefix, otherwise false
 *    :rtype: bool
 */
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

/*
 * !doc
 *
 * .. c:function:: void pkgconf_path_free(pkgconf_list_t *dirlist)
 *
 *    Releases any path nodes attached to the given path list.
 *
 *    :param pkgconf_list_t* dirlist: The path list to clean up.
 *    :return: nothing
 */
void
pkgconf_path_free(pkgconf_list_t *dirlist)
{
	pkgconf_node_t *n, *tn;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(dirlist->head, tn, n)
	{
		pkgconf_path_t *pnode = n->data;

		free(pnode->path);
		free(pnode);
	}
}
