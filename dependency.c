/*
 * dependency.c
 * dependency parsing and management
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

#include "pkg.h"
#include "bsdstubs.h"

/*
 * pkg_dependency_parse(pkg, depends)
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

static inline pkg_dependency_t *
pkg_dependency_add(pkg_dependency_t *head, const char *package, size_t package_sz, const char *version, size_t version_sz, pkg_comparator_t compare)
{
	pkg_dependency_t *dep;

	dep = calloc(sizeof(pkg_dependency_t), 1);
	dep->package = strndup(package, package_sz);

	if (version_sz != 0)
		dep->version = strndup(version, version_sz);

	dep->compare = compare;

	dep->prev = head;
	if (dep->prev != NULL)
		dep->prev->next = dep;

#if DEBUG_PARSE
	fprintf(error_msgout, "--> %s %d %s\n", dep->package, dep->compare, dep->version);
#endif

	return dep;
}

pkg_dependency_t *
pkg_dependency_append(pkg_dependency_t *head, pkg_dependency_t *tail)
{
	pkg_dependency_t *node;

	if (head == NULL)
		return tail;

	/* skip to end of list */
	PKG_FOREACH_LIST_ENTRY(head, node)
	{
		if (node->next == NULL)
			break;
	}

	node->next = tail;
	tail->prev = node;

	return head;
}

void
pkg_dependency_free(pkg_dependency_t *head)
{
	pkg_dependency_t *node, *next;

	PKG_FOREACH_LIST_ENTRY_SAFE(head, next, node)
	{
		if (node->package != NULL)
			free(node->package);

		if (node->version != NULL)
			free(node->version);

		free(node);
	}
}

pkg_dependency_t *
pkg_dependency_parse_str(pkg_dependency_t *deplist_head, const char *depends)
{
	parse_state_t state = OUTSIDE_MODULE;
	pkg_dependency_t *deplist = NULL;
	pkg_comparator_t compare = PKG_ANY;
	char buf[PKG_BUFSIZE];
	size_t package_sz = 0, version_sz = 0;
	char *start = buf;
	char *ptr = buf;
	char *vstart = NULL;
	char *package = NULL, *version = NULL;

	strlcpy(buf, depends, sizeof buf);
	strlcat(buf, " ", sizeof buf);

	while (*ptr)
	{
		switch (state)
		{
		case OUTSIDE_MODULE:
			if (!PKG_MODULE_SEPARATOR(*ptr))
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
				else if (PKG_MODULE_SEPARATOR(*sptr))
					state = OUTSIDE_MODULE;
				else if (PKG_OPERATOR_CHAR(*sptr))
					state = BEFORE_OPERATOR;
				else
					state = OUTSIDE_MODULE;
			}
			else if (PKG_MODULE_SEPARATOR(*ptr))
				state = OUTSIDE_MODULE;
			else if (*(ptr + 1) == '\0')
			{
				ptr++;
				state = OUTSIDE_MODULE;
			}

			if (state != INSIDE_MODULE_NAME && start != ptr)
			{
				char *iter = start;

				while (PKG_MODULE_SEPARATOR(*iter))
					iter++;

				package = iter;
				package_sz = ptr - iter;
#if DEBUG_PARSE
				fprintf(error_msgout, "Found package: %s\n", package);
#endif
				start = ptr;
			}

			if (state == OUTSIDE_MODULE)
			{
				deplist = pkg_dependency_add(deplist, package, package_sz, NULL, 0, compare);

				if (deplist_head == NULL)
					deplist_head = deplist;

				compare = PKG_ANY;
				package_sz = 0;
				version_sz = 0;
			}

			break;

		case BEFORE_OPERATOR:
			if (PKG_OPERATOR_CHAR(*ptr))
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
			if (!PKG_OPERATOR_CHAR(*ptr))
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
			fprintf(error_msgout, "Found op: %d\n", compare);
#endif

			if (!isspace(*ptr))
			{
				vstart = ptr;
				state = INSIDE_VERSION;
			}
			break;

		case INSIDE_VERSION:
			if (PKG_MODULE_SEPARATOR(*ptr) || *(ptr + 1) == '\0')
			{
				version = vstart;
				version_sz = ptr - vstart;
				state = OUTSIDE_MODULE;

#if DEBUG_PARSE
				fprintf(error_msgout, "Found version: %s\n", version);
#endif
				deplist = pkg_dependency_add(deplist, package, package_sz, version, version_sz, compare);

				if (deplist_head == NULL)
					deplist_head = deplist;

				compare = PKG_ANY;
				package_sz = 0;
				version_sz = 0;
			}

			if (state == OUTSIDE_MODULE)
				start = ptr;
			break;
		}

		ptr++;
	}

	return deplist_head;
}

pkg_dependency_t *
pkg_dependency_parse(pkg_t *pkg, const char *depends)
{
	pkg_dependency_t *list = NULL;
	char *kvdepends = pkg_tuple_parse(pkg->vars, depends);

	list = pkg_dependency_parse_str(list, kvdepends);
	free(kvdepends);

	return list;
}
