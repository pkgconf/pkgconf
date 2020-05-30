/*
 * tuple.c
 * management of key->value tuples
 *
 * Copyright (c) 2011, 2012 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>

/*
 * !doc
 *
 * libpkgconf `tuple` module
 * =========================
 *
 * The `tuple` module provides key-value mappings backed by a linked list.  The key-value
 * mapping is mainly used for variable substitution when parsing .pc files.
 *
 * There are two sets of mappings: a ``pkgconf_pkg_t`` specific mapping, and a `global` mapping.
 * The `tuple` module provides convenience wrappers for managing the `global` mapping, which is
 * attached to a given client object.
 */

/*
 * !doc
 *
 * .. c:function:: void pkgconf_tuple_add_global(pkgconf_client_t *client, const char *key, const char *value)
 *
 *    Defines a global variable, replacing the previous declaration if one was set.
 *
 *    :param pkgconf_client_t* client: The pkgconf client object to modify.
 *    :param char* key: The key for the mapping (variable name).
 *    :param char* value: The value for the mapped entry.
 *    :return: nothing
 */
void
pkgconf_tuple_add_global(pkgconf_client_t *client, const char *key, const char *value)
{
	pkgconf_tuple_add(client, &client->global_vars, key, value, false);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_tuple_find_global(const pkgconf_client_t *client, const char *key)
 *
 *    Looks up a global variable.
 *
 *    :param pkgconf_client_t* client: The pkgconf client object to access.
 *    :param char* key: The key or variable name to look up.
 *    :return: the contents of the variable or ``NULL``
 *    :rtype: char *
 */
char *
pkgconf_tuple_find_global(const pkgconf_client_t *client, const char *key)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(client->global_vars.head, node)
	{
		pkgconf_tuple_t *tuple = node->data;

		if (!strcmp(tuple->key, key))
			return tuple->value;
	}

	return NULL;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_tuple_free_global(pkgconf_client_t *client)
 *
 *    Delete all global variables associated with a pkgconf client object.
 *
 *    :param pkgconf_client_t* client: The pkgconf client object to modify.
 *    :return: nothing
 */
void
pkgconf_tuple_free_global(pkgconf_client_t *client)
{
	pkgconf_tuple_free(&client->global_vars);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_tuple_define_global(pkgconf_client_t *client, const char *kv)
 *
 *    Parse and define a global variable.
 *
 *    :param pkgconf_client_t* client: The pkgconf client object to modify.
 *    :param char* kv: The variable in the form of ``key=value``.
 *    :return: nothing
 */
void
pkgconf_tuple_define_global(pkgconf_client_t *client, const char *kv)
{
	char *workbuf = strdup(kv);
	char *value;

	value = strchr(workbuf, '=');
	if (value == NULL)
		goto out;

	*value++ = '\0';
	pkgconf_tuple_add_global(client, workbuf, value);
out:
	free(workbuf);
}

static void
pkgconf_tuple_find_delete(pkgconf_list_t *list, const char *key)
{
	pkgconf_node_t *node, *next;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(list->head, next, node)
	{
		pkgconf_tuple_t *tuple = node->data;

		if (!strcmp(tuple->key, key))
		{
			pkgconf_tuple_free_entry(tuple, list);
			return;
		}
	}
}

static char *
dequote(const char *value)
{
	char *buf = calloc((strlen(value) + 1) * 2, 1);
	char *bptr = buf;
	const char *i;
	char quote = 0;

	if (*value == '\'' || *value == '"')
		quote = *value;

	for (i = value; *i != '\0'; i++)
	{
		if (*i == '\\' && quote && *(i + 1) == quote)
		{
			i++;
			*bptr++ = *i;
		}
		else if (*i != quote)
			*bptr++ = *i;
	}

	return buf;
}

/*
 * !doc
 *
 * .. c:function:: pkgconf_tuple_t *pkgconf_tuple_add(const pkgconf_client_t *client, pkgconf_list_t *list, const char *key, const char *value, bool parse)
 *
 *    Optionally parse and then define a variable.
 *
 *    :param pkgconf_client_t* client: The pkgconf client object to access.
 *    :param pkgconf_list_t* list: The variable list to add the new variable to.
 *    :param char* key: The name of the variable being added.
 *    :param char* value: The value of the variable being added.
 *    :param bool parse: Whether or not to parse the value for variable substitution.
 *    :return: a variable object
 *    :rtype: pkgconf_tuple_t *
 */
pkgconf_tuple_t *
pkgconf_tuple_add(const pkgconf_client_t *client, pkgconf_list_t *list, const char *key, const char *value, bool parse)
{
	char *dequote_value;
	pkgconf_tuple_t *tuple = calloc(sizeof(pkgconf_tuple_t), 1);

	pkgconf_tuple_find_delete(list, key);

	dequote_value = dequote(value);

	PKGCONF_TRACE(client, "adding tuple to @%p: %s => %s (parsed? %d)", list, key, dequote_value, parse);

	tuple->key = strdup(key);
	if (parse)
		tuple->value = pkgconf_tuple_parse(client, list, dequote_value);
	else
		tuple->value = strdup(dequote_value);

	pkgconf_node_insert(&tuple->iter, tuple, list);

	free(dequote_value);

	return tuple;
}

/*
 * !doc
 *
 * .. c:function:: char *pkgconf_tuple_find(const pkgconf_client_t *client, pkgconf_list_t *list, const char *key)
 *
 *    Look up a variable in a variable list.
 *
 *    :param pkgconf_client_t* client: The pkgconf client object to access.
 *    :param pkgconf_list_t* list: The variable list to search.
 *    :param char* key: The variable name to search for.
 *    :return: the value of the variable or ``NULL``
 *    :rtype: char *
 */
char *
pkgconf_tuple_find(const pkgconf_client_t *client, pkgconf_list_t *list, const char *key)
{
	pkgconf_node_t *node;
	char *res;

	if ((res = pkgconf_tuple_find_global(client, key)) != NULL)
		return res;

	PKGCONF_FOREACH_LIST_ENTRY(list->head, node)
	{
		pkgconf_tuple_t *tuple = node->data;

		if (!strcmp(tuple->key, key))
			return tuple->value;
	}

	return NULL;
}

/*
 * !doc
 *
 * .. c:function:: char *pkgconf_tuple_parse(const pkgconf_client_t *client, pkgconf_list_t *vars, const char *value)
 *
 *    Parse an expression for variable substitution.
 *
 *    :param pkgconf_client_t* client: The pkgconf client object to access.
 *    :param pkgconf_list_t* list: The variable list to search for variables (along side the global variable list).
 *    :param char* value: The ``key=value`` string to parse.
 *    :return: the variable data with any variables substituted
 *    :rtype: char *
 */
char *
pkgconf_tuple_parse(const pkgconf_client_t *client, pkgconf_list_t *vars, const char *value)
{
	char buf[PKGCONF_BUFSIZE];
	const char *ptr;
	char *bptr = buf;

	if (*value == '/' && client->sysroot_dir != NULL && strncmp(value, client->sysroot_dir, strlen(client->sysroot_dir)))
		bptr += pkgconf_strlcpy(buf, client->sysroot_dir, sizeof buf);

	for (ptr = value; *ptr != '\0' && bptr - buf < PKGCONF_BUFSIZE; ptr++)
	{
		if (*ptr != '$' || (*ptr == '$' && *(ptr + 1) != '{'))
			*bptr++ = *ptr;
		else if (*(ptr + 1) == '{')
		{
			char varname[PKGCONF_ITEM_SIZE];
			char *vend = varname + PKGCONF_ITEM_SIZE - 1;
			char *vptr = varname;
			const char *pptr;
			char *kv, *parsekv;

			*vptr = '\0';

			for (pptr = ptr + 2; *pptr != '\0'; pptr++)
			{
				if (*pptr != '}')
					if (vptr < vend)
						*vptr++ = *pptr;
				else
				{
					*vptr = '\0';
					break;
				}
			}

			ptr += (pptr - ptr);
			kv = pkgconf_tuple_find_global(client, varname);
			if (kv != NULL)
			{
				strncpy(bptr, kv, PKGCONF_BUFSIZE - (bptr - buf));
				bptr += strlen(kv);
			}
			else
			{
				kv = pkgconf_tuple_find(client, vars, varname);

				if (kv != NULL)
				{
					parsekv = pkgconf_tuple_parse(client, vars, kv);

					strncpy(bptr, parsekv, PKGCONF_BUFSIZE - (bptr - buf));
					bptr += strlen(parsekv);

					free(parsekv);
				}
			}
		}
	}

	*bptr = '\0';

	/*
	 * Sigh.  Somebody actually attempted to use freedesktop.org pkg-config's broken sysroot support,
	 * which was written by somebody who did not understand how sysroots are supposed to work.  This
	 * results in an incorrect path being built as the sysroot will be prepended twice, once explicitly,
	 * and once by variable expansion (the pkgconf approach).  We could simply make ${pc_sysrootdir} blank,
	 * but sometimes it is necessary to know the explicit sysroot path for other reasons, so we can't really
	 * do that.
	 *
	 * As a result, we check to see if ${pc_sysrootdir} is prepended as a duplicate, and if so, remove the
	 * prepend.  This allows us to handle both our approach and the broken freedesktop.org implementation's
	 * approach.  Because a path can be shorter than ${pc_sysrootdir}, we do some checks first to ensure it's
	 * safe to skip ahead in the string to scan for our sysroot dir.
	 *
	 * Finally, we call pkgconf_path_relocate() to clean the path of spurious elements.
	 */
	if (*buf == '/' &&
	    client->sysroot_dir != NULL &&
	    strcmp(client->sysroot_dir, "/") != 0 &&
	    strlen(buf) > strlen(client->sysroot_dir) &&
	    strstr(buf + strlen(client->sysroot_dir), client->sysroot_dir) != NULL)
	{
		char cleanpath[PKGCONF_ITEM_SIZE];

		pkgconf_strlcpy(cleanpath, buf + strlen(client->sysroot_dir), sizeof cleanpath);
		pkgconf_path_relocate(cleanpath, sizeof cleanpath);

		return strdup(cleanpath);
	}

	return strdup(buf);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_tuple_free_entry(pkgconf_tuple_t *tuple, pkgconf_list_t *list)
 *
 *    Deletes a variable object, removing it from any variable lists and releasing any memory associated
 *    with it.
 *
 *    :param pkgconf_tuple_t* tuple: The variable object to release.
 *    :param pkgconf_list_t* list: The variable list the variable object is attached to.
 *    :return: nothing
 */
void
pkgconf_tuple_free_entry(pkgconf_tuple_t *tuple, pkgconf_list_t *list)
{
	pkgconf_node_delete(&tuple->iter, list);

	free(tuple->key);
	free(tuple->value);
	free(tuple);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_tuple_free(pkgconf_list_t *list)
 *
 *    Deletes a variable list and any variables attached to it.
 *
 *    :param pkgconf_list_t* list: The variable list to delete.
 *    :return: nothing
 */
void
pkgconf_tuple_free(pkgconf_list_t *list)
{
	pkgconf_node_t *node, *next;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(list->head, next, node)
		pkgconf_tuple_free_entry(node->data, list);
}
