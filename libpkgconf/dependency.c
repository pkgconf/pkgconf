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

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>

/*
 * !doc
 *
 * libpkgconf `dependency` module
 * ==============================
 *
 * The `dependency` module provides support for building `dependency lists` (the basic component of the overall `dependency graph`) and
 * `dependency nodes` which store dependency information.
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

static const char *
dependency_to_str(const pkgconf_dependency_t *dep, char *buf, size_t buflen)
{
	pkgconf_strlcpy(buf, dep->package, buflen);
	if (dep->version != NULL)
	{
		pkgconf_strlcat(buf, " ", buflen);
		pkgconf_strlcat(buf, pkgconf_pkg_get_comparator(dep), buflen);
		pkgconf_strlcat(buf, " ", buflen);
		pkgconf_strlcat(buf, dep->version, buflen);
	}

	return buf;
}

/* find a colliding dependency that is coloured differently */
static inline pkgconf_dependency_t *
find_colliding_dependency(const pkgconf_dependency_t *dep, const pkgconf_list_t *list)
{
	const pkgconf_node_t *n;

	PKGCONF_FOREACH_LIST_ENTRY(list->head, n)
	{
		pkgconf_dependency_t *dep2 = n->data;

		if (strcmp(dep->package, dep2->package))
			continue;

		if (dep->flags != dep2->flags)
			return dep2;
	}

	return NULL;
}

static inline pkgconf_dependency_t *
add_or_replace_dependency_node(const pkgconf_client_t *client, pkgconf_dependency_t *dep, pkgconf_list_t *list)
{
	char depbuf[PKGCONF_ITEM_SIZE];
	pkgconf_dependency_t *dep2 = find_colliding_dependency(dep, list);

	/* there is already a node in the graph which describes this dependency */
	if (dep2 != NULL)
	{
		char depbuf2[PKGCONF_ITEM_SIZE];

		PKGCONF_TRACE(client, "dependency collision: [%s/%x] -- [%s/%x]",
			      dependency_to_str(dep, depbuf, sizeof depbuf), dep->flags,
			      dependency_to_str(dep2, depbuf2, sizeof depbuf2), dep2->flags);

		/* prefer the uncoloured node, either dep or dep2 */
		if (dep->flags && dep2->flags == 0)
		{
			PKGCONF_TRACE(client, "dropping dependency [%s]@%p because of collision", depbuf, dep);

			free(dep);
			return NULL;
		}
		else if (dep2->flags && dep->flags == 0)
		{
			PKGCONF_TRACE(client, "dropping dependency [%s]@%p because of collision", depbuf2, dep2);

			pkgconf_node_delete(&dep2->iter, list);
			free(dep2);
		}
		else
			/* If both dependencies have equal strength, we keep both, because of situations like:
			 *    Requires: foo > 1, foo < 3
			 *
			 * If the situation is that both dependencies are literally equal, it is still harmless because
			 * fragment deduplication will handle the excessive fragments.
			 */
			PKGCONF_TRACE(client, "keeping both dependencies (harmless)");
	}

	PKGCONF_TRACE(client, "added dependency [%s] to list @%p; flags=%x", dependency_to_str(dep, depbuf, sizeof depbuf), list, dep->flags);
	pkgconf_node_insert_tail(&dep->iter, dep, list);

	return dep;
}

static inline pkgconf_dependency_t *
pkgconf_dependency_addraw(const pkgconf_client_t *client, pkgconf_list_t *list, const char *package, size_t package_sz, const char *version, size_t version_sz, pkgconf_pkg_comparator_t compare, unsigned int flags)
{
	pkgconf_dependency_t *dep;

	dep = calloc(sizeof(pkgconf_dependency_t), 1);
	dep->package = pkgconf_strndup(package, package_sz);

	if (version_sz != 0)
		dep->version = pkgconf_strndup(version, version_sz);

	dep->compare = compare;
	dep->flags = flags;

	return add_or_replace_dependency_node(client, dep, list);
}

/*
 * !doc
 *
 * .. c:function:: pkgconf_dependency_t *pkgconf_dependency_add(pkgconf_list_t *list, const char *package, const char *version, pkgconf_pkg_comparator_t compare)
 *
 *    Adds a parsed dependency to a dependency list as a dependency node.
 *
 *    :param pkgconf_client_t* client: The client object that owns the package this dependency list belongs to.
 *    :param pkgconf_list_t* list: The dependency list to add a dependency node to.
 *    :param char* package: The package `atom` to set on the dependency node.
 *    :param char* version: The package `version` to set on the dependency node.
 *    :param pkgconf_pkg_comparator_t compare: The comparison operator to set on the dependency node.
 *    :param uint flags: Any flags to attach to the dependency node.
 *    :return: A dependency node.
 *    :rtype: pkgconf_dependency_t *
 */
pkgconf_dependency_t *
pkgconf_dependency_add(const pkgconf_client_t *client, pkgconf_list_t *list, const char *package, const char *version, pkgconf_pkg_comparator_t compare, unsigned int flags)
{
	if (version != NULL)
		return pkgconf_dependency_addraw(client, list, package, strlen(package), version, strlen(version), compare, flags);

	return pkgconf_dependency_addraw(client, list, package, strlen(package), NULL, 0, compare, flags);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_dependency_append(pkgconf_list_t *list, pkgconf_dependency_t *tail)
 *
 *    Adds a dependency node to a pre-existing dependency list.
 *
 *    :param pkgconf_list_t* list: The dependency list to add a dependency node to.
 *    :param pkgconf_dependency_t* tail: The dependency node to add to the tail of the dependency list.
 *    :return: nothing
 */
void
pkgconf_dependency_append(pkgconf_list_t *list, pkgconf_dependency_t *tail)
{
	pkgconf_node_insert_tail(&tail->iter, tail, list);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_dependency_free(pkgconf_list_t *list)
 *
 *    Release a dependency list and it's child dependency nodes.
 *
 *    :param pkgconf_list_t* list: The dependency list to release.
 *    :return: nothing
 */
void
pkgconf_dependency_free(pkgconf_list_t *list)
{
	pkgconf_node_t *node, *next;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(list->head, next, node)
	{
		pkgconf_dependency_t *dep = node->data;

		if (dep->match != NULL)
			pkgconf_pkg_unref(NULL, dep->match);

		if (dep->package != NULL)
			free(dep->package);

		if (dep->version != NULL)
			free(dep->version);

		free(dep);
	}
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_dependency_parse_str(pkgconf_list_t *deplist_head, const char *depends)
 *
 *    Parse a dependency declaration into a dependency list.
 *    Commas are counted as whitespace to allow for constructs such as ``@SUBSTVAR@, zlib`` being processed
 *    into ``, zlib``.
 *
 *    :param pkgconf_client_t* client: The client object that owns the package this dependency list belongs to.
 *    :param pkgconf_list_t* deplist_head: The dependency list to populate with dependency nodes.
 *    :param char* depends: The dependency data to parse.
 *    :param uint flags: Any flags to attach to the dependency nodes.
 *    :return: nothing
 */
void
pkgconf_dependency_parse_str(const pkgconf_client_t *client, pkgconf_list_t *deplist_head, const char *depends, unsigned int flags)
{
	parse_state_t state = OUTSIDE_MODULE;
	pkgconf_pkg_comparator_t compare = PKGCONF_CMP_ANY;
	char cmpname[PKGCONF_ITEM_SIZE];
	char buf[PKGCONF_BUFSIZE];
	size_t package_sz = 0, version_sz = 0;
	char *start = buf;
	char *ptr = buf;
	char *vstart = NULL;
	char *package = NULL, *version = NULL;
	char *cnameptr = cmpname;
	char *cnameend = cmpname + PKGCONF_ITEM_SIZE - 1;

	memset(cmpname, '\0', sizeof cmpname);

	pkgconf_strlcpy(buf, depends, sizeof buf);
	pkgconf_strlcat(buf, " ", sizeof buf);

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
				pkgconf_dependency_addraw(client, deplist_head, package, package_sz, NULL, 0, compare, flags);

				compare = PKGCONF_CMP_ANY;
				package_sz = 0;
			}

			break;

		case BEFORE_OPERATOR:
			if (PKGCONF_IS_OPERATOR_CHAR(*ptr))
			{
				state = INSIDE_OPERATOR;
				if (cnameptr < cnameend)
					*cnameptr++ = *ptr;
			}

			break;

		case INSIDE_OPERATOR:
			if (!PKGCONF_IS_OPERATOR_CHAR(*ptr))
			{
				state = AFTER_OPERATOR;
				compare = pkgconf_pkg_comparator_lookup_by_name(cmpname);
			}
			else if (cnameptr < cnameend)
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

				pkgconf_dependency_addraw(client, deplist_head, package, package_sz, version, version_sz, compare, flags);

				compare = PKGCONF_CMP_ANY;
				cnameptr = cmpname;
				memset(cmpname, 0, sizeof cmpname);
				package_sz = 0;
			}

			if (state == OUTSIDE_MODULE)
				start = ptr;
			break;
		}

		ptr++;
	}
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_dependency_parse(const pkgconf_client_t *client, pkgconf_pkg_t *pkg, pkgconf_list_t *deplist, const char *depends)
 *
 *    Preprocess dependency data and then process that dependency declaration into a dependency list.
 *    Commas are counted as whitespace to allow for constructs such as ``@SUBSTVAR@, zlib`` being processed
 *    into ``, zlib``.
 *
 *    :param pkgconf_client_t* client: The client object that owns the package this dependency list belongs to.
 *    :param pkgconf_pkg_t* pkg: The package object that owns this dependency list.
 *    :param pkgconf_list_t* deplist: The dependency list to populate with dependency nodes.
 *    :param char* depends: The dependency data to parse.
 *    :param uint flags: Any flags to attach to the dependency nodes.
 *    :return: nothing
 */
void
pkgconf_dependency_parse(const pkgconf_client_t *client, pkgconf_pkg_t *pkg, pkgconf_list_t *deplist, const char *depends, unsigned int flags)
{
	char *kvdepends = pkgconf_tuple_parse(client, &pkg->vars, depends);

	pkgconf_dependency_parse_str(client, deplist, kvdepends, flags);
	free(kvdepends);
}
