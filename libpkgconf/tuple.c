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

static pkgconf_list_t pkg_global_var = PKGCONF_LIST_INITIALIZER;

void
pkgconf_tuple_add_global(const char *key, const char *value)
{
	pkgconf_tuple_add(&pkg_global_var, key, value, false);
}

char *
pkgconf_tuple_find_global(const char *key)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(pkg_global_var.head, node)
	{
		pkgconf_tuple_t *tuple = node->data;

		if (!strcmp(tuple->key, key))
			return tuple->value;
	}

	return NULL;
}

void
pkgconf_tuple_free_global(void)
{
	pkgconf_tuple_free(&pkg_global_var);
}

void
pkgconf_tuple_define_global(const char *kv)
{
	char *workbuf = strdup(kv);
	char *value;

	value = strchr(workbuf, '=');
	if (value == NULL)
		goto out;

	*value++ = '\0';
	pkgconf_tuple_add_global(workbuf, value);
out:
	free(workbuf);
}

pkgconf_tuple_t *
pkgconf_tuple_add(pkgconf_list_t *list, const char *key, const char *value, bool parse)
{
	pkgconf_tuple_t *tuple = calloc(sizeof(pkgconf_tuple_t), 1);

	tuple->key = strdup(key);
	if (parse)
		tuple->value = pkgconf_tuple_parse(list, value);
	else
		tuple->value = strdup(value);

	pkgconf_node_insert(&tuple->iter, tuple, list);

	return tuple;
}

char *
pkgconf_tuple_find(pkgconf_list_t *list, const char *key)
{
	pkgconf_node_t *node;
	char *res;

	if ((res = pkgconf_tuple_find_global(key)) != NULL)
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
pkgconf_tuple_parse(pkgconf_list_t *vars, const char *value)
{
	char buf[PKGCONF_BUFSIZE];
	const char *ptr;
	char *bptr = buf;

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
			kv = pkgconf_tuple_find_global(varname);
			if (kv != NULL)
			{
				strncpy(bptr, kv, PKGCONF_BUFSIZE - (bptr - buf));
				bptr += strlen(kv);
			}
			else
			{
				kv = pkgconf_tuple_find(vars, varname);

				if (kv != NULL)
				{
					parsekv = pkgconf_tuple_parse(vars, kv);

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
pkgconf_tuple_free(pkgconf_list_t *list)
{
	pkgconf_node_t *node, *next;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(list->head, next, node)
	{
		pkgconf_tuple_t *tuple = node->data;

		free(tuple->key);
		free(tuple->value);
		free(tuple);
	}
}
