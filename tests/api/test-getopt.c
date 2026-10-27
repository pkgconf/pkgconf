/*
 * test-serialize.c
 * Tests for imported Net/OpenBSD getopt functions
 *
 * SPDX-License-Identifier: pkgconf
 *
 * Copyright (c) 2026 pkgconf authors (see AUTHORS).
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
#include "getopt_long.h"

static void
test_getopt_long_test_without_param(void)
{
	struct pkg_option options[] = {
		{ "version", no_argument, NULL, 5, },
		{ "test", no_argument, NULL, 6, },
		{ NULL, 0, NULL, 0 }
	};

	char *argv[] = {"test", "--test", "--version", "--notavailable", "-a"};

	TEST_ASSERT_EQ(pkg_getopt_long_only(5, argv, "", options, NULL), 6);
	TEST_ASSERT_EQ(pkg_getopt_long_only(5, argv, "", options, NULL), 5);
	TEST_ASSERT_EQ(pkg_getopt_long_only(5, argv, "", options, NULL), 63);
	// This is actually not correct
	TEST_ASSERT_EQ(pkg_getopt_long_only(5, argv, "ab:", options, NULL), 97);
	TEST_ASSERT_EQ(pkg_getopt_long_only(5, argv, "", options, NULL), -1);

	pkg_optind = 1;
	TEST_ASSERT_EQ(pkg_getopt_long(5, argv, "ab:", options, NULL), 6);
	TEST_ASSERT_EQ(pkg_getopt_long(5, argv, "ab:", options, NULL), 5);
	TEST_ASSERT_EQ(pkg_getopt_long(5, argv, "ab:", options, NULL), 63);
	// This is actually not correct..
	TEST_ASSERT_EQ(pkg_getopt_long(5, argv, "ab:", options, NULL), 97);
	TEST_ASSERT_EQ(pkg_getopt_long(5, argv, "ab:", options, NULL), -1);

}

static void
test_getopt_long_test_with_param(void)
{
	struct pkg_option options[] = {
		{ "pa", no_argument, NULL, 5, },
		{ "pb", required_argument, NULL, 6, },
		{ NULL, 0, NULL, 0 }
	};

	char *argv[] = {"test", "--pa", "--pb=test", "-pb", "another", "-a"};

	pkg_optind = 1;

	TEST_ASSERT_EQ(pkg_getopt_long_only(6, argv, "", options, NULL), 5);
	TEST_ASSERT_EQ(pkg_getopt_long_only(6, argv, "", options, NULL), 6);
	TEST_ASSERT_STRCMP_EQ(pkg_optarg, "test");
	TEST_ASSERT_EQ(pkg_getopt_long_only(6, argv, "", options, NULL), 6);
	TEST_ASSERT_STRCMP_EQ(pkg_optarg, "another");
	TEST_ASSERT_EQ(pkg_getopt_long_only(6, argv, "", options, NULL), 63);
	TEST_ASSERT_EQ(pkg_getopt_long_only(6, argv, "", options, NULL), -1);

	pkg_optind = 1;

	TEST_ASSERT_EQ(pkg_getopt_long(6, argv, "ab:", options, NULL), 5);
	TEST_ASSERT_EQ(pkg_getopt_long(6, argv, "ab:", options, NULL), 6);
	TEST_ASSERT_STRCMP_EQ(pkg_optarg, "test");
	TEST_ASSERT_EQ(pkg_getopt_long(6, argv, "ab:", options, NULL), 63);
	TEST_ASSERT_NULL(pkg_optarg);
	TEST_ASSERT_EQ(pkg_getopt_long(6, argv, "ab:", options, NULL), 98);
	TEST_ASSERT_STRCMP_EQ(pkg_optarg, "another");
	TEST_ASSERT_EQ(pkg_getopt_long(6, argv, "ab:", options, NULL), 97);
	TEST_ASSERT_EQ(pkg_getopt_long(6, argv, "ab:", options, NULL), -1);

}

static void
test_getopt_test_with_param(void)
{
	char *argv[] = {"test", "-a", "-b", "test", "-b", "another", "-b=one", "-b param"};

	pkg_optind = 1;

	TEST_ASSERT_EQ(pkg_getopt(8, argv, "ab:"), 97);
	TEST_ASSERT_EQ(pkg_getopt(8, argv, "ab:"), 98);
	TEST_ASSERT_STRCMP_EQ(pkg_optarg, "test");
	TEST_ASSERT_EQ(pkg_getopt(8, argv, "ab:"), 98);
	TEST_ASSERT_STRCMP_EQ(pkg_optarg, "another");
	TEST_ASSERT_EQ(pkg_getopt(8, argv, "ab:"), 98);
	// These are not valid but parsed
	TEST_ASSERT_STRCMP_EQ(pkg_optarg, "=one");
	TEST_ASSERT_EQ(pkg_getopt(8, argv, "ab:"), 98);
	TEST_ASSERT_STRCMP_EQ(pkg_optarg, " param");
	TEST_ASSERT_EQ(pkg_getopt(8, argv, "ab:"), -1);
}


int
main(int argc, const char **argv)
{
	(void) argc;
	const char *basename = pkgconf_path_find_basename(argv[0]);

	TEST_RUN(basename, test_getopt_long_test_without_param);
	TEST_RUN(basename, test_getopt_long_test_with_param);
	TEST_RUN(basename, test_getopt_test_with_param);

	return EXIT_SUCCESS;
}
