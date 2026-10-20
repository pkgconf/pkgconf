/*
 * test-serialize.c
 * Tests for spdxtool's JSON serialization value model.
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
#include "serialize.h"

// Render a value to a freshly-allocated C string (caller frees)
static char *
render(pkgconfcli_serialize_value_t *value)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;
	TEST_ASSERT_TRUE(pkgconfcli_serialize_value_to_buf(&buf, value, 0));
	char *s = strdup(pkgconf_buffer_str_or_empty(&buf));
	pkgconf_buffer_finalize(&buf);
	return s;
}

// Render a container without taking ownership of it
static char *
render_object(pkgconfcli_serialize_object_list_t *o)
{
	pkgconfcli_serialize_value_t wrap = { .type = PKGCONFCLI_SERIALIZE_TYPE_OBJECT, .value = { .o = o } };
	return render(&wrap);
}

static char *
render_array(pkgconfcli_serialize_array_t *a)
{
	pkgconfcli_serialize_value_t wrap = { .type = PKGCONFCLI_SERIALIZE_TYPE_ARRAY, .value = { .a = a } };
	return render(&wrap);
}

// Scalar value types: string / int / bool / null
static void
test_serialize_value_string(void)
{
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_string("hi");
	TEST_ASSERT_NONNULL(v);
	char *s = render(v);
	TEST_ASSERT_STRCMP_EQ(s, "\"hi\"");
	free(s);
	pkgconfcli_serialize_value_free(v);
}

static void
test_serialize_test_value_none(void)
{
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(NULL,PKGCONFCLI_SERIALIZE_TYPE_STRING));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(NULL,PKGCONFCLI_SERIALIZE_TYPE_INT));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(NULL,PKGCONFCLI_SERIALIZE_TYPE_BOOL));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(NULL,PKGCONFCLI_SERIALIZE_TYPE_ARRAY));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(NULL,PKGCONFCLI_SERIALIZE_TYPE_OBJECT));
}


static void
test_serialize_test_value_string(void)
{
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_string("test");
	TEST_ASSERT_TRUE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_STRING));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_INT));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_BOOL));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_ARRAY));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_OBJECT));

	pkgconfcli_serialize_value_free(v);
}

static void
test_serialize_value_int(void)
{
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_int(123);
	TEST_ASSERT_NONNULL(v);
	char *s = render(v);
	TEST_ASSERT_STRCMP_EQ(s, "123");
	free(s);
	pkgconfcli_serialize_value_free(v);
}

static void
test_serialize_test_value_int(void)
{
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_int(123);
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_STRING));
	TEST_ASSERT_TRUE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_INT));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_BOOL));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_ARRAY));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_OBJECT));

	pkgconfcli_serialize_value_free(v);
}


static void
test_serialize_value_bool(void)
{
	pkgconfcli_serialize_value_t *t = pkgconfcli_serialize_value_bool(true);
	pkgconfcli_serialize_value_t *f = pkgconfcli_serialize_value_bool(false);
	char *st = render(t);
	char *sf = render(f);
	TEST_ASSERT_STRCMP_EQ(st, "true");
	TEST_ASSERT_STRCMP_EQ(sf, "false");
	free(st);
	free(sf);
	pkgconfcli_serialize_value_free(t);
	pkgconfcli_serialize_value_free(f);
}

static void
test_serialize_test_value_bool(void)
{
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_bool(true);
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_STRING));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_INT));
	TEST_ASSERT_TRUE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_BOOL));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_ARRAY));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_OBJECT));

	pkgconfcli_serialize_value_free(v);
}

static void
test_serialize_value_null(void)
{
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_null();
	TEST_ASSERT_NONNULL(v);
	char *s = render(v);
	TEST_ASSERT_STRCMP_EQ(s, "null");
	free(s);
	pkgconfcli_serialize_value_free(v);
}

static void
test_serialize_test_value_array(void)
{
	pkgconfcli_serialize_array_t *a = pkgconfcli_serialize_array_new();
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_array(a);

	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_STRING));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_INT));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_BOOL));
	TEST_ASSERT_TRUE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_ARRAY));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_OBJECT));

	pkgconfcli_serialize_value_free(v);
}

static void
test_serialize_test_value_object(void)
{
	pkgconfcli_serialize_object_list_t *o = pkgconfcli_serialize_object_list_new();
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_object(o);

	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_STRING));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_INT));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_BOOL));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_ARRAY));
	TEST_ASSERT_TRUE(pkgconfcli_serialize_is_value(v,PKGCONFCLI_SERIALIZE_TYPE_OBJECT));

	pkgconfcli_serialize_value_free(v);
}

// All JSON string escape sequences, including control characters
static void
test_serialize_escape_sequences(void)
{
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_string("\"\\\b\f\n\r\t\x01" "Z");
	char *s = render(v);

	TEST_ASSERT_STRSTR(s, "\\\"");     // quote
	TEST_ASSERT_STRSTR(s, "\\\\");     // backslash
	TEST_ASSERT_STRSTR(s, "\\b");      // backspace
	TEST_ASSERT_STRSTR(s, "\\f");      // form feed
	TEST_ASSERT_STRSTR(s, "\\n");      // newline
	TEST_ASSERT_STRSTR(s, "\\r");      // carriage return
	TEST_ASSERT_STRSTR(s, "\\t");      // tab
	TEST_ASSERT_STRSTR(s, "\\u0001");  // other ctrl
	TEST_ASSERT_STRSTR(s, "Z");        // printable passthrough

	free(s);
	pkgconfcli_serialize_value_free(v);
}

// Mixed-type object exercises the int/bool/null add helpers
static void
test_serialize_object_mixed_types(void)
{
	pkgconfcli_serialize_object_list_t *o = pkgconfcli_serialize_object_list_new();
	TEST_ASSERT_NONNULL(o);

	TEST_ASSERT_NONNULL(pkgconfcli_serialize_object_add_string(o, "s", "x"));
	TEST_ASSERT_NONNULL(pkgconfcli_serialize_object_add_int(o, "i", 42));
	TEST_ASSERT_NONNULL(pkgconfcli_serialize_object_add_bool(o, "b", true));
	TEST_ASSERT_NONNULL(pkgconfcli_serialize_object_add_null(o, "n"));

	char *s = render_object(o);
	TEST_ASSERT_STRSTR(s, "\"s\": \"x\"");
	TEST_ASSERT_STRSTR(s, "\"i\": 42");
	TEST_ASSERT_STRSTR(s, "\"b\": true");
	TEST_ASSERT_STRSTR(s, "\"n\": null");

	free(s);
	pkgconfcli_serialize_object_list_free(o);
}

static void
test_serialize_object_key_escaping(void)
{
	pkgconfcli_serialize_object_list_t *o = pkgconfcli_serialize_object_list_new();
	TEST_ASSERT_NONNULL(o);
	TEST_ASSERT_NONNULL(pkgconfcli_serialize_object_add_string(o, "\"\\\n\x01", "x"));

	char *s = render_object(o);
	TEST_ASSERT_STRCMP_EQ(s, "{\n    \"\\\"\\\\\\n\\u0001\": \"x\"\n}");

	free(s);
	pkgconfcli_serialize_object_list_free(o);
}

// Mixed-type array exercises the int/bool/null array helpers
static void
test_serialize_array_mixed_types(void)
{
	pkgconfcli_serialize_array_t *a = pkgconfcli_serialize_array_new();
	TEST_ASSERT_NONNULL(a);

	TEST_ASSERT_NONNULL(pkgconfcli_serialize_array_add_int(a, 7));
	TEST_ASSERT_NONNULL(pkgconfcli_serialize_array_add_bool(a, false));
	TEST_ASSERT_NONNULL(pkgconfcli_serialize_array_add_null(a));

	char *s = render_array(a);
	TEST_ASSERT_STRSTR(s, "7");
	TEST_ASSERT_STRSTR(s, "false");
	TEST_ASSERT_STRSTR(s, "null");

	free(s);
	pkgconfcli_serialize_array_free(a);
}

// Defensive NULL handling that the CLI path never reaches
static void
test_serialize_null_guards(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;
	pkgconfcli_serialize_value_t *v = pkgconfcli_serialize_value_null();

	// value_to_buf rejects NULL buffer or NULL value
	TEST_ASSERT_FALSE(pkgconfcli_serialize_value_to_buf(NULL, v, 0));
	TEST_ASSERT_FALSE(pkgconfcli_serialize_value_to_buf(&buf, NULL, 0));
	pkgconf_buffer_finalize(&buf);

	// value_string(NULL) yields NULL
	TEST_ASSERT_NULL(pkgconfcli_serialize_value_string(NULL));

	// add_take with a NULL container frees the value and returns NULL
	TEST_ASSERT_NULL(pkgconfcli_serialize_object_add_take(NULL, "k", v));
	TEST_ASSERT_NULL(pkgconfcli_serialize_array_add_take(NULL, pkgconfcli_serialize_value_null()));

	// free routines are NULL-safe
	pkgconfcli_serialize_value_free(NULL);
	pkgconfcli_serialize_object_free(NULL);
	pkgconfcli_serialize_object_list_free(NULL);
	pkgconfcli_serialize_array_free(NULL);
}

static void
test_serialize_value_find_by_key(void)
{
	pkgconfcli_serialize_object_list_t *o = pkgconfcli_serialize_object_list_new();
	pkgconfcli_serialize_array_t *a = pkgconfcli_serialize_array_new();
	pkgconfcli_serialize_value_t *vo = pkgconfcli_serialize_value_object(o);
	pkgconfcli_serialize_value_t *va = pkgconfcli_serialize_value_array(a);
	pkgconfcli_serialize_value_t *v = NULL;
	char test_key[12];
	int i = 0;

	TEST_ASSERT_NULL(pkgconfcli_serialize_value_object_find(NULL, "test"));
	TEST_ASSERT_NULL(pkgconfcli_serialize_value_object_find(vo, NULL));
	TEST_ASSERT_NULL(pkgconfcli_serialize_value_object_find(va, "test"));

	for(i = 0; i <= 24; i ++)
	{
		snprintf(test_key, 10, "test%05d", i);
		pkgconfcli_serialize_object_add_take(o, test_key, pkgconfcli_serialize_value_string(test_key));
	}

	TEST_ASSERT_NULL(pkgconfcli_serialize_value_object_find(vo, "test00025"));

	for(i = 24; i > 0; i --)
	{
		memset(test_key, 0x00, 12);
		snprintf(test_key, 10, "test%05d", i);
		v = pkgconfcli_serialize_value_object_find(vo, test_key);

        TEST_ASSERT_NONNULL(v);
		TEST_ASSERT_EQ(v->type,PKGCONFCLI_SERIALIZE_TYPE_STRING);
		TEST_ASSERT_STRCASECMP_EQ(v->value.s, test_key);
	}

	pkgconfcli_serialize_value_free(va);
	pkgconfcli_serialize_value_free(vo);
}


int
main(int argc, const char **argv)
{
	(void) argc;
	const char *basename = pkgconf_path_find_basename(argv[0]);

	TEST_RUN(basename, test_serialize_test_value_none);
	TEST_RUN(basename, test_serialize_value_string);
	TEST_RUN(basename, test_serialize_test_value_string);
	TEST_RUN(basename, test_serialize_value_int);
	TEST_RUN(basename, test_serialize_test_value_int);
	TEST_RUN(basename, test_serialize_value_bool);
	TEST_RUN(basename, test_serialize_test_value_bool);
	TEST_RUN(basename, test_serialize_value_null);
	TEST_RUN(basename, test_serialize_test_value_array);
	TEST_RUN(basename, test_serialize_test_value_object);
	TEST_RUN(basename, test_serialize_escape_sequences);
	TEST_RUN(basename, test_serialize_object_mixed_types);
	TEST_RUN(basename, test_serialize_object_key_escaping);
	TEST_RUN(basename, test_serialize_array_mixed_types);
	TEST_RUN(basename, test_serialize_null_guards);
	TEST_RUN(basename, test_serialize_value_find_by_key);

	return EXIT_SUCCESS;
}
