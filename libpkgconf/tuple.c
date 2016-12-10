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

#include <libpkgconf/libpkgconf.h>

void
pkgconf_tuple_add_global(pkgconf_client_t *client, const char *key, const char *value)
{
	pkgconf_tuple_add(client, &client->global_vars, key, value, false);
}

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

void
pkgconf_tuple_free_global(pkgconf_client_t *client)
{
	pkgconf_tuple_free(&client->global_vars);
}

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

pkgconf_tuple_t *
pkgconf_tuple_add(const pkgconf_client_t *client, pkgconf_list_t *list, const char *key, const char *value, bool parse)
{
	pkgconf_tuple_t *tuple = calloc(sizeof(pkgconf_tuple_t), 1);

	pkgconf_tuple_find_delete(list, key);

	tuple->key = strdup(key);
	if (parse)
		tuple->value = pkgconf_tuple_parse(client, list, value);
	else
		tuple->value = strdup(value);

	pkgconf_node_insert(&tuple->iter, tuple, list);

	return tuple;
}

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
			static char varname[PKGCONF_BUFSIZE];
			char *vptr = varname;
			const char *pptr;
			char *kv, *parsekv;

			*vptr = '\0';

			for (pptr = ptr + 2; *pptr != '\0'; pptr++)
			{
				if (*pptr != '}')
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

	return strdup(buf);
}

void
pkgconf_tuple_free_entry(pkgconf_tuple_t *tuple, pkgconf_list_t *list)
{
	pkgconf_node_delete(&tuple->iter, list);

	free(tuple->key);
	free(tuple->value);
	free(tuple);
}

void
pkgconf_tuple_free(pkgconf_list_t *list)
{
	pkgconf_node_t *node, *next;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(list->head, next, node)
		pkgconf_tuple_free_entry(node->data, list);
}
