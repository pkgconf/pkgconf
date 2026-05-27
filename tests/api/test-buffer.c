/*
 * test-buffer.c
 * Tests for libpkgconf buffer API.
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
test_buffer_empty(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	TEST_EQ(pkgconf_buffer_len(&buf), 0);
	TEST_ASSERT_NULL(pkgconf_buffer_str(&buf));
	TEST_STRCMP(pkgconf_buffer_str_or_empty(&buf), "");
	TEST_EQ(pkgconf_buffer_lastc(&buf), '\0');

	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_append(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	TEST_ASSERT_TRUE(pkgconf_buffer_append(&buf, "hello"));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "hello");
	TEST_EQ(pkgconf_buffer_len(&buf), 5);

	TEST_ASSERT_TRUE(pkgconf_buffer_append(&buf, " world"));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "hello world");
	TEST_EQ(pkgconf_buffer_len(&buf), 11);

	TEST_EQ(pkgconf_buffer_lastc(&buf), 'd');

	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_append_slice(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	TEST_ASSERT_TRUE(pkgconf_buffer_append_slice(&buf, "abcdefgh", 3));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "abc");

	TEST_ASSERT_TRUE(pkgconf_buffer_append_slice(&buf, "xyz", 0));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "abc");

	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_append_fmt(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	TEST_ASSERT_TRUE(pkgconf_buffer_append_fmt(&buf, "%s=%d", "x", 42));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "x=42");

	TEST_ASSERT_TRUE(pkgconf_buffer_append_fmt(&buf, " %s", "ok"));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "x=42 ok");

	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_prepend(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_buffer_append(&buf, "world");
	TEST_ASSERT_TRUE(pkgconf_buffer_prepend(&buf, "hello "));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "hello world");

	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_push_byte(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	TEST_ASSERT_TRUE(pkgconf_buffer_push_byte(&buf, 'a'));
	TEST_ASSERT_TRUE(pkgconf_buffer_push_byte(&buf, 'b'));
	TEST_ASSERT_TRUE(pkgconf_buffer_push_byte(&buf, 'c'));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "abc");
	TEST_EQ(pkgconf_buffer_len(&buf), 3);

	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_trim_byte(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_buffer_append(&buf, "hello\n");
	TEST_ASSERT_TRUE(pkgconf_buffer_trim_byte(&buf));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "hello");

	pkgconf_buffer_reset(&buf);
	pkgconf_buffer_push_byte(&buf, 'x');
	TEST_ASSERT_TRUE(pkgconf_buffer_trim_byte(&buf));
	TEST_EQ(pkgconf_buffer_len(&buf), 0);
	TEST_ASSERT_FALSE(pkgconf_buffer_trim_byte(&buf));

	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_reset(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_buffer_append(&buf, "some content");
	TEST_NE(pkgconf_buffer_len(&buf), 0);

	pkgconf_buffer_reset(&buf);
	TEST_EQ(pkgconf_buffer_len(&buf), 0);
	TEST_ASSERT_NULL(pkgconf_buffer_str(&buf));

	TEST_ASSERT_TRUE(pkgconf_buffer_append(&buf, "fresh"));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "fresh");

	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_freeze(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_buffer_append(&buf, "frozen");
	char *out = pkgconf_buffer_freeze(&buf);
	TEST_ASSERT_NONNULL(out);
	TEST_STRCMP(out, "frozen");
	TEST_EQ(pkgconf_buffer_len(&buf), 0);

	TEST_ASSERT_NULL(pkgconf_buffer_freeze(&buf));

	free(out);
	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_copy(void)
{
	pkgconf_buffer_t src = PKGCONF_BUFFER_INITIALIZER;
	pkgconf_buffer_t dst = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_buffer_append(&src, "original");
	pkgconf_buffer_append(&dst, "to be replaced");
	TEST_ASSERT_TRUE(pkgconf_buffer_copy(&src, &dst));
	TEST_STRCMP(pkgconf_buffer_str(&dst), "original");
	TEST_STRCMP(pkgconf_buffer_str(&src), "original"); // src is unchanged

	pkgconf_buffer_finalize(&src);
	pkgconf_buffer_finalize(&dst);
}

static void
test_buffer_join(void)
{
	pkgconf_buffer_t buf = PKGCONF_BUFFER_INITIALIZER;

	TEST_ASSERT_TRUE(pkgconf_buffer_join(&buf, '/', "usr", "local", "lib", NULL));
	TEST_STRCMP(pkgconf_buffer_str(&buf), "usr/local/lib");

	pkgconf_buffer_finalize(&buf);
}

static void
test_buffer_contains(void)
{
	pkgconf_buffer_t hay = PKGCONF_BUFFER_INITIALIZER;
	pkgconf_buffer_t needle = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_buffer_append(&hay, "the quick brown fox");

	pkgconf_buffer_append(&needle, "quick");
	TEST_ASSERT_TRUE(pkgconf_buffer_contains(&hay, &needle));

	pkgconf_buffer_reset(&needle);
	pkgconf_buffer_append(&needle, "slow");
	TEST_ASSERT_FALSE(pkgconf_buffer_contains(&hay, &needle));

	TEST_ASSERT_TRUE(pkgconf_buffer_contains_byte(&hay, 'q'));
	TEST_ASSERT_FALSE(pkgconf_buffer_contains_byte(&hay, 'z'));

	pkgconf_buffer_finalize(&hay);
	pkgconf_buffer_finalize(&needle);
}

static void
test_buffer_match(void)
{
	pkgconf_buffer_t a = PKGCONF_BUFFER_INITIALIZER;
	pkgconf_buffer_t b = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_buffer_append(&a, "identical");
	pkgconf_buffer_append(&b, "identical");
	TEST_ASSERT_TRUE(pkgconf_buffer_match(&a, &b));

	pkgconf_buffer_reset(&b);
	pkgconf_buffer_append(&b, "different");
	TEST_ASSERT_FALSE(pkgconf_buffer_match(&a, &b));

	pkgconf_buffer_finalize(&a);
	pkgconf_buffer_finalize(&b);
}

static void
test_buffer_has_prefix(void)
{
	pkgconf_buffer_t hay = PKGCONF_BUFFER_INITIALIZER;
	pkgconf_buffer_t pre = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_buffer_append(&hay, "/usr/local/lib");

	pkgconf_buffer_append(&pre, "/usr");
	TEST_ASSERT_TRUE(pkgconf_buffer_has_prefix(&hay, &pre));

	pkgconf_buffer_reset(&pre);
	pkgconf_buffer_append(&pre, "/opt");
	TEST_ASSERT_FALSE(pkgconf_buffer_has_prefix(&hay, &pre));

	pkgconf_buffer_finalize(&hay);
	pkgconf_buffer_finalize(&pre);
}

static void
test_buffer_subst(void)
{
	pkgconf_buffer_t src = PKGCONF_BUFFER_INITIALIZER;
	pkgconf_buffer_t dst = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_buffer_append(&src, "prefix=${PREFIX}/share");
	TEST_ASSERT_TRUE(pkgconf_buffer_subst(&dst, &src, "${PREFIX}", "/opt/foo"));
	TEST_STRCMP(pkgconf_buffer_str(&dst), "prefix=/opt/foo/share");

	pkgconf_buffer_finalize(&src);
	pkgconf_buffer_finalize(&dst);
}

static void
test_buffer_escape(void)
{
	pkgconf_buffer_t src = PKGCONF_BUFFER_INITIALIZER;
	pkgconf_buffer_t dst = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_span_t spans[] = {
		{ ' ', ' ' },
		{ '\t', '\t' },
	};

	pkgconf_buffer_append(&src, "a b\tc");
	TEST_ASSERT_TRUE(pkgconf_buffer_escape(&dst, &src, spans, PKGCONF_ARRAY_SIZE(spans)));
	TEST_STRCMP(pkgconf_buffer_str(&dst), "a\\ b\\\tc");

	pkgconf_buffer_finalize(&src);
	pkgconf_buffer_finalize(&dst);
}

static void
test_str_eq_slice(void)
{
	TEST_ASSERT_TRUE(pkgconf_str_eq_slice("hello", "hello", 5));
	TEST_ASSERT_FALSE(pkgconf_str_eq_slice("hello!", "hello", 5));
	TEST_ASSERT_FALSE(pkgconf_str_eq_slice("hello", "world", 5));
	TEST_ASSERT_FALSE(pkgconf_str_eq_slice(NULL, "hello", 5));
}

static void
test_span_contains(void)
{
	pkgconf_span_t spans[] = {
		{ '0', '9' },
		{ 'a', 'z' },
	};

	TEST_ASSERT_TRUE(pkgconf_span_contains('5', spans, PKGCONF_ARRAY_SIZE(spans)));
	TEST_ASSERT_TRUE(pkgconf_span_contains('m', spans, PKGCONF_ARRAY_SIZE(spans)));
	TEST_ASSERT_FALSE(pkgconf_span_contains('A', spans, PKGCONF_ARRAY_SIZE(spans)));
	TEST_ASSERT_FALSE(pkgconf_span_contains('!', spans, PKGCONF_ARRAY_SIZE(spans)));
}

int
main(int argc, const char **argv)
{
	(void) argc;

	const char *basename = test_progname(argv[0]);

	TEST_RUN(basename, test_buffer_empty);
	TEST_RUN(basename, test_buffer_append);
	TEST_RUN(basename, test_buffer_append_slice);
	TEST_RUN(basename, test_buffer_append_fmt);
	TEST_RUN(basename, test_buffer_prepend);
	TEST_RUN(basename, test_buffer_push_byte);
	TEST_RUN(basename, test_buffer_trim_byte);
	TEST_RUN(basename, test_buffer_reset);
	TEST_RUN(basename, test_buffer_freeze);
	TEST_RUN(basename, test_buffer_copy);
	TEST_RUN(basename, test_buffer_join);
	TEST_RUN(basename, test_buffer_contains);
	TEST_RUN(basename, test_buffer_match);
	TEST_RUN(basename, test_buffer_has_prefix);
	TEST_RUN(basename, test_buffer_subst);
	TEST_RUN(basename, test_buffer_escape);
	TEST_RUN(basename, test_str_eq_slice);
	TEST_RUN(basename, test_span_contains);

	return 0;
}
