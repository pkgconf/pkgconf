/*
 * parse.c
 * Parser for .pc file.
 *
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>.
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

static pkg_tuple_t *
tuple_add(pkg_tuple_t *parent, const char *key, const char *value)
{
	pkg_tuple_t *tuple = calloc(sizeof(pkg_tuple_t), 1);

	tuple->key = strdup(key);
	tuple->value = strdup(value);

	tuple->next = parent;
	if (tuple->next != NULL)
		tuple->next->prev = tuple;

	return tuple;
}

static char *
tuple_find(pkg_tuple_t *head, const char *key)
{
	pkg_tuple_t *node;

	foreach_list_entry(head, node)
	{
		if (!strcasecmp(node->key, key))
			return node->value;
	}

	return NULL;
}

static char *
strdup_parse(pkg_t *pkg, const char *value)
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
			kv = tuple_find(pkg->vars, varname);

			if (kv != NULL)
			{
				parsekv = strdup_parse(pkg, kv);

				strncpy(bptr, parsekv, BUFSIZ - (bptr - buf));
				bptr += strlen(parsekv);

				free(parsekv);
			}
		}
	}

	*bptr = '\0';

	return strdup(buf);
}

/*
 * parse_file(filename)
 *
 * Parse a .pc file into a pkg_t object structure.
 */
pkg_t *
parse_file(const char *filename)
{
	FILE *f;
	pkg_t *pkg;
	char readbuf[BUFSIZ];

	f = fopen(filename, "r");
	if (f == NULL)
		return NULL;

	pkg = calloc(sizeof(pkg_t), 1);
	pkg->filename = strdup(filename);

	while (fgets(readbuf, BUFSIZ, f) != NULL)
	{
		char *p, *key = NULL, *value = NULL;

		readbuf[strlen(readbuf) - 1] = '\0';

		p = readbuf;
		while (*p && (isalpha(*p) || isdigit(*p) || *p == '_' || *p == '.'))
			p++;

		key = strndup(readbuf, p - readbuf);
		if (!isalpha(*key) && !isdigit(*p))
			goto cleanup;

		while (*p && isspace(*p))
			p++;

		value = strdup(p + 1);

		switch (*p)
		{
		case ':':
			if (!strcasecmp(key, "Name"))
				pkg->realname = strdup_parse(pkg, value);
			if (!strcasecmp(key, "Description"))
				pkg->description = strdup_parse(pkg, value);
			if (!strcasecmp(key, "Version"))
				pkg->version = strdup_parse(pkg, value);
			if (!strcasecmp(key, "CFLAGS"))
				pkg->cflags = strdup_parse(pkg, value);
			if (!strcasecmp(key, "LIBS"))
				pkg->libs = strdup_parse(pkg, value);
			break;
		case '=':
			pkg->vars = tuple_add(pkg->vars, key, value);
			break;
		default:
			break;
		}

cleanup:
		if (key)
			free(key);

		if (value)
			free(value);
	}

	fclose(f);
	return pkg;
}
