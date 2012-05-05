/*
 * parse.c
 * Parser for .pc file.
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

char *
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

char *
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

static pkg_fragment_t *
parse_fragment_list(pkg_t *pkg, const char *string)
{
	int i, argc;
	char **argv;
	char *repstr = strdup_parse(pkg, string);
	pkg_fragment_t *head = NULL;

	argv_split(repstr, &argc, &argv);

	for (i = 0; i < argc; i++)
		head = pkg_fragment_add(head, argv[i]);

	free(repstr);

	return head;
}

/*
 * parse_deplist(pkg, depends)
 *
 * Add requirements to a .pc file.
 * Commas are counted as whitespace, as to allow:
 *    @SUBSTVAR@, zlib
 * getting substituted to:
 *    , zlib.
 */
typedef enum {
	OUTSIDE_MODULE = 0,
	INSIDE_MODULE_NAME = 1,
	BEFORE_OPERATOR = 2,
	INSIDE_OPERATOR = 3,
	AFTER_OPERATOR = 4,
	INSIDE_VERSION = 5
} parse_state_t;

#define DEBUG_PARSE 0

static pkg_dependency_t *
pkg_dependency_add(pkg_dependency_t *head, const char *package, const char *version, pkg_comparator_t compare)
{
	pkg_dependency_t *dep;

	dep = calloc(sizeof(pkg_dependency_t), 1);
	dep->package = strdup(package);

	if (version != NULL)
		dep->version = strdup(version);

	dep->compare = compare;

	dep->prev = head;
	if (dep->prev != NULL)
		dep->prev->next = dep;

#if DEBUG_PARSE
	fprintf(stderr, "--> %s %d %s\n", dep->package, dep->compare, dep->version);
#endif

	return dep;
}

#define MODULE_SEPARATOR(c) ((c) == ',' || isspace ((c)))
#define OPERATOR_CHAR(c) ((c) == '<' || (c) == '>' || (c) == '!' || (c) == '=')

pkg_dependency_t *
pkg_dependency_append(pkg_dependency_t *head, pkg_dependency_t *tail)
{
	pkg_dependency_t *node;

	if (head == NULL)
		return tail;

	/* skip to end of list */
	foreach_list_entry(head, node)
	{
		if (node->next == NULL)
			break;
	}

	node->next = tail;
	tail->prev = node;

	return head;
}

pkg_dependency_t *
parse_deplist(pkg_t *pkg, const char *depends)
{
	parse_state_t state = OUTSIDE_MODULE;
	pkg_dependency_t *deplist = NULL;
	pkg_dependency_t *deplist_head = NULL;
	pkg_comparator_t compare = PKG_ANY;
	char buf[BUFSIZ];
	char *kvdepends = strdup_parse(pkg, depends);
	char *start = buf;
	char *ptr = buf;
	char *vstart = NULL;
	char *package = NULL, *version = NULL;

	strlcpy(buf, kvdepends, sizeof buf);
	strlcat(buf, " ", sizeof buf);
	free(kvdepends);

	while (*ptr)
	{
		switch (state)
		{
		case OUTSIDE_MODULE:
			if (!MODULE_SEPARATOR(*ptr))
				state = INSIDE_MODULE_NAME;

			break;

		case INSIDE_MODULE_NAME:
			if (isspace(*ptr))
			{
				const char *sptr = ptr;

				while (*sptr && isspace(*sptr))
					sptr++;

				if (*sptr == '\0')
					state = OUTSIDE_MODULE;
				else if (MODULE_SEPARATOR(*sptr))
					state = OUTSIDE_MODULE;
				else if (OPERATOR_CHAR(*sptr))
					state = BEFORE_OPERATOR;
				else
					state = OUTSIDE_MODULE;
			}
			else if (MODULE_SEPARATOR(*ptr))
				state = OUTSIDE_MODULE;
			else if (*(ptr + 1) == '\0')
			{
				ptr++;
				state = OUTSIDE_MODULE;
			}

			if (state != INSIDE_MODULE_NAME && start != ptr)
			{
				char *iter = start;

				while (MODULE_SEPARATOR(*iter))
					iter++;

				package = strndup(iter, ptr - iter);
#if DEBUG_PARSE
				fprintf(stderr, "Found package: %s\n", package);
#endif
				start = ptr;
			}

			if (state == OUTSIDE_MODULE)
			{
				deplist = pkg_dependency_add(deplist, package, NULL, compare);

				if (deplist_head == NULL)
					deplist_head = deplist;

				if (package != NULL)
				{
					free(package);
					package = NULL;
				}

				if (version != NULL)
				{
					free(version);
					version = NULL;
				}

				compare = PKG_ANY;
			}

			break;

		case BEFORE_OPERATOR:
			if (OPERATOR_CHAR(*ptr))
			{
				switch(*ptr)
				{
				case '=':
					compare = PKG_EQUAL;
					break;
				case '>':
					compare = PKG_GREATER_THAN;
					break;
				case '<':
					compare = PKG_LESS_THAN;
					break;
				case '!':
					compare = PKG_NOT_EQUAL;
					break;
				default:
					break;
				}

				state = INSIDE_OPERATOR;
			}

			break;

		case INSIDE_OPERATOR:
			if (!OPERATOR_CHAR(*ptr))
				state = AFTER_OPERATOR;
			else if (*ptr == '=')
			{
				switch(compare)
				{
				case PKG_LESS_THAN:
					compare = PKG_LESS_THAN_EQUAL;
					break;
				case PKG_GREATER_THAN:
					compare = PKG_GREATER_THAN_EQUAL;
					break;
				default:
					break;
				}
			}

			break;

		case AFTER_OPERATOR:
#if DEBUG_PARSE
			fprintf(stderr, "Found op: %d\n", compare);
#endif

			if (!isspace(*ptr))
			{
				vstart = ptr;
				state = INSIDE_VERSION;
			}
			break;

		case INSIDE_VERSION:
			if (MODULE_SEPARATOR(*ptr) || *(ptr + 1) == '\0')
			{
				version = strndup(vstart, (ptr - vstart));
				state = OUTSIDE_MODULE;

#if DEBUG_PARSE
				fprintf(stderr, "Found version: %s\n", version);
#endif
				deplist = pkg_dependency_add(deplist, package, version, compare);

				if (deplist_head == NULL)
					deplist_head = deplist;

				if (package != NULL)
				{
					free(package);
					package = NULL;
				}

				if (version != NULL)
				{
					free(version);
					version = NULL;
				}

				compare = PKG_ANY;
			}

			if (state == OUTSIDE_MODULE)
				start = ptr;
			break;
		}

		ptr++;
	}

	return deplist_head;
}

/*
 * parse_file(filename)
 *
 * Parse a .pc file into a pkg_t object structure.
 */
pkg_t *
parse_file(const char *filename, FILE *f)
{
	pkg_t *pkg;
	char readbuf[BUFSIZ];

	pkg = calloc(sizeof(pkg_t), 1);
	pkg->filename = strdup(filename);

	while (pkg_fgetline(readbuf, BUFSIZ, f) != NULL)
	{
		char op, *p, *key = NULL, *value = NULL;

		readbuf[strlen(readbuf) - 1] = '\0';

		p = readbuf;
		while (*p && (isalpha(*p) || isdigit(*p) || *p == '_' || *p == '.'))
			p++;

		key = strndup(readbuf, p - readbuf);
		if (!isalpha(*key) && !isdigit(*p))
			goto cleanup;

		while (*p && isspace(*p))
			p++;

		op = *p++;

		while (*p && isspace(*p))
			p++;

		value = strdup(p);

		switch (op)
		{
		case ':':
			if (!strcasecmp(key, "Name"))
				pkg->realname = strdup_parse(pkg, value);
			else if (!strcasecmp(key, "Description"))
				pkg->description = strdup_parse(pkg, value);
			else if (!strcasecmp(key, "Version"))
				pkg->version = strdup_parse(pkg, value);
			else if (!strcasecmp(key, "CFLAGS"))
				pkg->cflags = parse_fragment_list(pkg, value);
			else if (!strcasecmp(key, "LIBS"))
				pkg->libs = parse_fragment_list(pkg, value);
			else if (!strcasecmp(key, "LIBS.private"))
				pkg->libs_private = parse_fragment_list(pkg, value);
			else if (!strcasecmp(key, "Requires"))
				pkg->requires = parse_deplist(pkg, value);
			else if (!strcasecmp(key, "Requires.private"))
				pkg->requires_private = parse_deplist(pkg, value);
			else if (!strcasecmp(key, "Conflicts"))
				pkg->conflicts = parse_deplist(pkg, value);
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
