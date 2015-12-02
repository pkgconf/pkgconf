/*
 * dependency.c
 * dependency parsing and management
 *
 * Copyright (c) 2011, 2012, 2013 pkgconf authors (see AUTHORS).
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
 * pkgconf_dependency_parse(pkg, depends)
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

static inline pkgconf_dependency_t *
pkgconf_dependency_add(pkgconf_list_t *list, const char *package, size_t package_sz, const char *version, size_t version_sz, pkgconf_pkg_comparator_t compare)
{
	pkgconf_dependency_t *dep;

	dep = calloc(sizeof(pkgconf_dependency_t), 1);
	dep->package = strndup(package, package_sz);

	if (version_sz != 0)
		dep->version = strndup(version, version_sz);

	dep->compare = compare;

	pkgconf_node_insert_tail(&dep->iter, dep, list);

	return dep;
}

void
pkgconf_dependency_append(pkgconf_list_t *list, pkgconf_dependency_t *tail)
{
	pkgconf_node_insert_tail(&tail->iter, tail, list);
}

void
pkgconf_dependency_free(pkgconf_list_t *list)
{
	pkgconf_node_t *node, *next;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(list->head, next, node)
	{
		pkgconf_dependency_t *dep = node->data;

		if (dep->package != NULL)
			free(dep->package);

		if (dep->version != NULL)
			free(dep->version);

		free(dep);
	}
}

void
pkgconf_dependency_parse_str(pkgconf_list_t *deplist_head, const char *depends)
{
	parse_state_t state = OUTSIDE_MODULE;
	pkgconf_pkg_comparator_t compare = PKGCONF_CMP_ANY;
	char cmpname[PKGCONF_BUFSIZE];
	char buf[PKGCONF_BUFSIZE];
	size_t package_sz = 0, version_sz = 0;
	char *start = buf;
	char *ptr = buf;
	char *vstart = NULL;
	char *package = NULL, *version = NULL;
	char *cnameptr = cmpname;

	memset(cmpname, '\0', sizeof cmpname);

	strlcpy(buf, depends, sizeof buf);
	strlcat(buf, " ", sizeof buf);

	while (*ptr)
	{
		switch (state)
		{
		case OUTSIDE_MODULE:
			if (!PKGCONF_IS_MODULE_SEPARATOR(*ptr))
				state = INSIDE_MODULE_NAME;

			break;

		case INSIDE_MODULE_NAME:
			if (isspace((unsigned int)*ptr))
			{
				const char *sptr = ptr;

				while (*sptr && isspace((unsigned int)*sptr))
					sptr++;

				if (*sptr == '\0')
					state = OUTSIDE_MODULE;
				else if (PKGCONF_IS_MODULE_SEPARATOR(*sptr))
					state = OUTSIDE_MODULE;
				else if (PKGCONF_IS_OPERATOR_CHAR(*sptr))
					state = BEFORE_OPERATOR;
				else
					state = OUTSIDE_MODULE;
			}
			else if (PKGCONF_IS_MODULE_SEPARATOR(*ptr))
				state = OUTSIDE_MODULE;
			else if (*(ptr + 1) == '\0')
			{
				ptr++;
				state = OUTSIDE_MODULE;
			}

			if (state != INSIDE_MODULE_NAME && start != ptr)
			{
				char *iter = start;

				while (PKGCONF_IS_MODULE_SEPARATOR(*iter))
					iter++;

				package = iter;
				package_sz = ptr - iter;
				start = ptr;
			}

			if (state == OUTSIDE_MODULE)
			{
				pkgconf_dependency_add(deplist_head, package, package_sz, NULL, 0, compare);

				compare = PKGCONF_CMP_ANY;
				package_sz = 0;
				version_sz = 0;
			}

			break;

		case BEFORE_OPERATOR:
			if (PKGCONF_IS_OPERATOR_CHAR(*ptr))
			{
				state = INSIDE_OPERATOR;
				*cnameptr++ = *ptr;
			}

			break;

		case INSIDE_OPERATOR:
			if (!PKGCONF_IS_OPERATOR_CHAR(*ptr))
			{
				state = AFTER_OPERATOR;
				compare = pkgconf_pkg_comparator_lookup_by_name(cmpname);
			}
			else
				*cnameptr++ = *ptr;

			break;

		case AFTER_OPERATOR:
			if (!isspace((unsigned int)*ptr))
			{
				vstart = ptr;
				state = INSIDE_VERSION;
			}
			break;

		case INSIDE_VERSION:
			if (PKGCONF_IS_MODULE_SEPARATOR(*ptr) || *(ptr + 1) == '\0')
			{
				version = vstart;
				version_sz = ptr - vstart;
				state = OUTSIDE_MODULE;

				pkgconf_dependency_add(deplist_head, package, package_sz, version, version_sz, compare);

				compare = PKGCONF_CMP_ANY;
				cnameptr = cmpname;
				memset(cmpname, 0, sizeof cmpname);
				package_sz = 0;
				version_sz = 0;
			}

			if (state == OUTSIDE_MODULE)
				start = ptr;
			break;
		}

		ptr++;
	}
}

void
pkgconf_dependency_parse(pkgconf_pkg_t *pkg, pkgconf_list_t *deplist, const char *depends)
{
	char *kvdepends = pkgconf_tuple_parse(&pkg->vars, depends);

	pkgconf_dependency_parse_str(deplist, kvdepends);
	free(kvdepends);
}
