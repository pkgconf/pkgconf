/*
 * tuple.c
 * management of key->value tuples
 *
 * Copyright (c) 2011, 2012 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "pkg.h"
#include "bsdstubs.h"

static pkg_tuple_t *pkg_global_var;

void
pkg_tuple_add_global(const char *key, const char *value)
{
	pkg_global_var = pkg_tuple_add(pkg_global_var, key, value);
}

char *
pkg_tuple_find_global(const char *key)
{
	pkg_tuple_t *node;

	PKG_FOREACH_LIST_ENTRY(pkg_global_var, node)
	{
		if (!strcasecmp(node->key, key))
			return node->value;
	}

	return NULL;
}

void
pkg_tuple_free_global(void)
{
	pkg_tuple_free(pkg_global_var);
}

pkg_tuple_t *
pkg_tuple_add(pkg_tuple_t *parent, const char *key, const char *value)
{
	pkg_tuple_t *tuple = calloc(sizeof(pkg_tuple_t), 1);

	tuple->key = strdup(key);
	tuple->value = pkg_tuple_parse(parent, value);

	tuple->next = parent;
	if (tuple->next != NULL)
		tuple->next->prev = tuple;

	return tuple;
}

char *
pkg_tuple_find(pkg_tuple_t *head, const char *key)
{
	pkg_tuple_t *node;

	PKG_FOREACH_LIST_ENTRY(head, node)
	{
		if (!strcasecmp(node->key, key))
			return node->value;
	}

	return pkg_tuple_find_global(key);
}

char *
pkg_tuple_parse(pkg_tuple_t *vars, const char *value)
{
	char buf[BUFSIZ];
	const char *ptr;
	char *bptr = buf;

	for (ptr = value; *ptr != '\0' && bptr - buf < BUFSIZ; ptr++)
	{
		if (*ptr != '$')
			*bptr++ = *ptr;
		else if (*(ptr + 1) == '{')
		{
			static char varname[BUFSIZ];
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
			kv = pkg_tuple_find(vars, varname);

			if (kv != NULL)
			{
				parsekv = pkg_tuple_parse(vars, kv);

				strncpy(bptr, parsekv, BUFSIZ - (bptr - buf));
				bptr += strlen(parsekv);

				free(parsekv);
			}
		}
	}

	*bptr = '\0';

	return strdup(buf);
}

void
pkg_tuple_free(pkg_tuple_t *head)
{
	pkg_tuple_t *node, *next;

	PKG_FOREACH_LIST_ENTRY_SAFE(head, next, node)
	{
		free(node->key);
		free(node->value);
		free(node);
	}
}
