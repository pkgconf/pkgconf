/*
 * test-iter.c
 * Tests for inline node and lists API.
 *
 * SPDX-License-Identifier: pkgconf
 *
 * Copyright (c) 2025 pkgconf authors (see AUTHORS).
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
#include "libpkgconf/iter.h"
#include "test-api.h"

static void
populate_list(pkgconf_list_t *list, int len, bool tail)
{
	int i = 0;
	pkgconf_node_t *node = NULL;
	char *string = NULL;

	for (i = 0; i < len; i ++)
	{
		node = (pkgconf_node_t *) calloc(1, sizeof(pkgconf_node_t));
		string = (char *) calloc(1, 24);
		snprintf(string, 24, "test string %d", i);
		if (tail)
			pkgconf_node_insert_tail(node, string, list);
		else
			pkgconf_node_insert(node, string, list);
	}
}

static void
free_node(pkgconf_node_t *node)
{
	free(node->data);
	free(node);
}

static void
empty_list(pkgconf_list_t *list)
{
	pkgconf_node_t *node = NULL;

	while (list->length > 0)
	{
		node = list->head;
		pkgconf_node_delete(list->head, list);
		free_node(node);
	}
}

static void
test_iter_add_and_find_head(void)
{
	pkgconf_list_t list = PKGCONF_LIST_INITIALIZER;
	pkgconf_node_t node;
	char *test_string = "test string";

	TEST_ASSERT_EQ(list.length, 0);

	memset(&node, 0x00, sizeof(pkgconf_node_t));

	pkgconf_node_insert(&node, test_string, &list);

	TEST_ASSERT_STRCMP_EQ(list.head->data, test_string);
	TEST_ASSERT_EQ(list.length, 1);
}

static void
test_iter_add_and_find_tail(void)
{
	pkgconf_list_t list = PKGCONF_LIST_INITIALIZER;
	pkgconf_node_t node;
	char *test_string = "test string";

	TEST_ASSERT_EQ(list.length, 0);

	memset(&node, 0x00, sizeof(pkgconf_node_t));

	pkgconf_node_insert_tail(&node, test_string, &list);

	TEST_ASSERT_STRCMP_EQ(list.tail->data, test_string);
	TEST_ASSERT_EQ(list.length, 1);
}

static void
test_iter_delete(void)
{
	pkgconf_list_t list = PKGCONF_LIST_INITIALIZER;
	pkgconf_node_t *node;

	populate_list(&list, 2, true);

	TEST_ASSERT_EQ(list.length, 2);
	node = list.head;

	pkgconf_node_delete(node, &list);

	TEST_ASSERT_EQ(list.length, 1);
	TEST_ASSERT_STRCMP_EQ(list.head->data, "test string 1");
	TEST_ASSERT_STRCMP_EQ(list.tail->data, "test string 1");

	free_node(node);

	node = list.head;

	pkgconf_node_delete(node, &list);
	TEST_ASSERT_EQ(list.length, 0);
	TEST_ASSERT_EQ(list.head, NULL);
	TEST_ASSERT_EQ(list.tail, NULL);

	free_node(node);

	populate_list(&list, 3, true);
	TEST_ASSERT_EQ(list.length, 3);

	node = list.head->next;

	pkgconf_node_delete(node, &list);
	TEST_ASSERT_EQ(list.length, 2);
	TEST_ASSERT_STRCMP_EQ(list.head->data, "test string 0");
	TEST_ASSERT_STRCMP_EQ(list.tail->data, "test string 2");

	free_node(node);

	node = list.head;
	pkgconf_node_delete(node, &list);
	free_node(node);
	TEST_ASSERT_EQ(list.length, 1);
	TEST_ASSERT_STRCMP_EQ(list.tail->data, "test string 2");

	node = list.head;
	pkgconf_node_delete(node, &list);
	free_node(node);
	TEST_ASSERT_EQ(list.length, 0);
}

static void
test_iter_splice(void)
{
	pkgconf_list_t list = PKGCONF_LIST_INITIALIZER;
	pkgconf_list_t list2 = PKGCONF_LIST_INITIALIZER;

	populate_list(&list, 3, false);

	pkgconf_list_splice(&list2, &list);
	TEST_ASSERT_EQ(list.length, 0);
	TEST_ASSERT_EQ(list2.length, 3);
	TEST_ASSERT_EQ(list.head, NULL);
	TEST_ASSERT_EQ(list.head, NULL);
	TEST_ASSERT_STRCMP_EQ(list2.head->data, "test string 2");
	TEST_ASSERT_STRCMP_EQ(list2.tail->data, "test string 0");

	empty_list(&list2);
}

int
main(int argc, char *argv[])
{
	(void) argc;
	const char *basename = pkgconf_path_find_basename(argv[0]);

	TEST_RUN(basename, test_iter_add_and_find_head);
	TEST_RUN(basename, test_iter_add_and_find_tail);
	TEST_RUN(basename, test_iter_delete);
	TEST_RUN(basename, test_iter_splice);

	return EXIT_SUCCESS;
}
