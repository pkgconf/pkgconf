/*
 * test-oom.c
 * Allocation-failure (OOM) tests for spdxtool, using the oom.h injection
 * harness.  For each target, a failure is injected at every successive
 * allocation; the target must report the error gracefully (NULL) and, under
 * ASAN, leak nothing on the error path.
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
#include "oom.h"
#include "serialize.h"

static void
test_oom_serialize_object_add(void)
{
	pkgconfcli_serialize_object_list_t *o = NULL;
	pkgconfcli_serialize_value_t *s = NULL;

	OOM_TEST_ALLOC_PTR(o, pkgconfcli_serialize_object_list_new(), s, pkgconfcli_serialize_object_add_take(o, "hello", pkgconfcli_serialize_value_string("hello world")), pkgconfcli_serialize_object_list_free(o));
	OOM_TEST_ALLOC_PTR(o, pkgconfcli_serialize_object_list_new(), s, pkgconfcli_serialize_object_add_string(o, "hello", "hello world"), pkgconfcli_serialize_object_list_free(o));
	OOM_TEST_ALLOC_PTR(o, pkgconfcli_serialize_object_list_new(), s, pkgconfcli_serialize_object_add_object(o, "hello", pkgconfcli_serialize_object_list_new()), pkgconfcli_serialize_object_list_free(o));
	OOM_TEST_ALLOC_PTR(o, pkgconfcli_serialize_object_list_new(), s, pkgconfcli_serialize_object_add_array(o, "hello", pkgconfcli_serialize_array_new()), pkgconfcli_serialize_object_list_free(o));
}

static void
test_oom_serialize_array_add(void)
{
	pkgconfcli_serialize_array_t *o = NULL;
	pkgconfcli_serialize_value_t *s = NULL;

	OOM_TEST_ALLOC_PTR(o, pkgconfcli_serialize_array_new(), s, pkgconfcli_serialize_array_add_take(o, pkgconfcli_serialize_value_string("hello world")), pkgconfcli_serialize_array_free(o));
	OOM_TEST_ALLOC_PTR(o, pkgconfcli_serialize_array_new(), s, pkgconfcli_serialize_array_add_string(o, "hello world"), pkgconfcli_serialize_array_free(o));
	OOM_TEST_ALLOC_PTR(o, pkgconfcli_serialize_array_new(), s, pkgconfcli_serialize_array_add_object(o, pkgconfcli_serialize_object_list_new()), pkgconfcli_serialize_array_free(o));
	OOM_TEST_ALLOC_PTR(o, pkgconfcli_serialize_array_new(), s, pkgconfcli_serialize_array_add_array(o, pkgconfcli_serialize_array_new()), pkgconfcli_serialize_array_free(o));
}


/*
 * ======================
 * serializer value model
 * ======================
 */

static void
test_oom_serialize_constructors(void)
{
	pkgconfcli_serialize_object_list_t *o;
	OOM_TEST_PTR(o, pkgconfcli_serialize_object_list_new(), pkgconfcli_serialize_object_list_free(o));

	pkgconfcli_serialize_array_t *a;
	OOM_TEST_PTR(a, pkgconfcli_serialize_array_new(), pkgconfcli_serialize_array_free(a));

	pkgconfcli_serialize_value_t *v;
	OOM_TEST_PTR(v, pkgconfcli_serialize_value_string("hello"), pkgconfcli_serialize_value_free(v));
	OOM_TEST_PTR(v, pkgconfcli_serialize_value_int(7), pkgconfcli_serialize_value_free(v));
	OOM_TEST_PTR(v, pkgconfcli_serialize_value_bool(true), pkgconfcli_serialize_value_free(v));
	OOM_TEST_PTR(v, pkgconfcli_serialize_value_null(), pkgconfcli_serialize_value_free(v));
}

static void
test_oom_serialize_to_buffer(void)
{
	pkgconfcli_serialize_object_list_t *object = pkgconfcli_serialize_object_list_new();
	TEST_ASSERT_NONNULL(object);
	TEST_ASSERT_NONNULL(pkgconfcli_serialize_object_add_string(object, "\"key", "value"));

	pkgconfcli_serialize_value_t root = {
		.type = PKGCONFCLI_SERIALIZE_TYPE_OBJECT,
		.value = { .o = object },
	};

	for (unsigned long n = 1; ; n++)
	{
		pkgconf_buffer_t buffer = PKGCONF_BUFFER_INITIALIZER;

		alloc_inject_arm(n);
		bool ok = pkgconfcli_serialize_value_to_buf(&buffer, &root, 0);
		bool fired = alloc_inject_fired();
		alloc_inject_disarm();
		pkgconf_buffer_finalize(&buffer);

		if (!fired)
		{
			TEST_ASSERT_TRUE(ok);
			break;
		}

		TEST_ASSERT_FALSE(ok);
	}

	pkgconfcli_serialize_object_list_free(object);
}

int
main(int argc, const char **argv)
{
	(void) argc;
	const char *basename = pkgconf_path_find_basename(argv[0]);

	TEST_RUN(basename, test_oom_serialize_constructors);
	TEST_RUN(basename, test_oom_serialize_object_add);
	TEST_RUN(basename, test_oom_serialize_array_add);
	TEST_RUN(basename, test_oom_serialize_to_buffer);

	// pkgconf_client_free(g_client);
	return EXIT_SUCCESS;
}
