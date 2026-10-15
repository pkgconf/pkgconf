/*
 * test-iter.c
 * Test inline node ja list API.
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
#include "test-api.h"

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
	pkgconf_node_t node;
	pkgconf_node_t node2;
	pkgconf_node_t node3;
	char *test_string = "test string";
	char *test_string_2 = "test string 2";
	char *test_string_3 = "test string 3";

	memset(&node, 0x00, sizeof(pkgconf_node_t));

	pkgconf_node_insert(&node, test_string, &list);
	pkgconf_node_insert(&node2, test_string_2, &list);

	TEST_ASSERT_EQ(list.length, 2);

	pkgconf_node_delete(&node, &list);

	TEST_ASSERT_EQ(list.length, 1);
	TEST_ASSERT_STRCMP_EQ(list.head->data, test_string_2);
	TEST_ASSERT_STRCMP_EQ(list.tail->data, test_string_2);

	pkgconf_node_delete(&node2, &list);
	TEST_ASSERT_EQ(list.length, 0);
	TEST_ASSERT_EQ(list.head, NULL);
	TEST_ASSERT_EQ(list.tail, NULL);

	pkgconf_node_insert_tail(&node, test_string, &list);
	pkgconf_node_insert_tail(&node2, test_string_2, &list);
	pkgconf_node_insert_tail(&node3, test_string_3, &list);
	TEST_ASSERT_EQ(list.length, 3);

	pkgconf_node_delete(&node2, &list);
	TEST_ASSERT_EQ(list.length, 2);
	TEST_ASSERT_STRCMP_EQ(list.head->data, test_string);
	TEST_ASSERT_STRCMP_EQ(list.tail->data, test_string_3);
}

static void
test_iter_splice(void)
{
	pkgconf_list_t list = PKGCONF_LIST_INITIALIZER;
	pkgconf_list_t list2 = PKGCONF_LIST_INITIALIZER;
	pkgconf_node_t node;
	pkgconf_node_t node2;
	pkgconf_node_t node3;
	char *test_string = "test string";
	char *test_string_2 = "test string 2";
	char *test_string_3 = "test string 3";

	memset(&node, 0x00, sizeof(pkgconf_node_t));

	pkgconf_node_insert(&node, test_string, &list);
	pkgconf_node_insert(&node2, test_string_2, &list);
	pkgconf_node_insert(&node3, test_string_3, &list);

	pkgconf_list_splice(&list2, &list);
	TEST_ASSERT_EQ(list.length, 0);
	TEST_ASSERT_EQ(list2.length, 3);
	TEST_ASSERT_EQ(list.head, NULL);
	TEST_ASSERT_EQ(list.head, NULL);
	TEST_ASSERT_STRCMP_EQ(list2.head->data, test_string_3);
	TEST_ASSERT_STRCMP_EQ(list2.tail->data, test_string);
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
