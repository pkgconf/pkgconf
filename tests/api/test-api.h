/*
 * test-api.h
 * test API macros and functions.
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

#ifndef TEST_API_H
#define TEST_API_H

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_FAIL_(fmt, ...)	\
	do {	\
		fprintf(stderr, "FAIL: %s:%d: " fmt "\n",	\
			__FILE__, __LINE__, __VA_ARGS__);	\
		exit(EXIT_FAILURE);	\
	} while (0)

#define TEST_ASSERT_TRUE(expr)	\
	do {	\
		if (!(expr))	\
			TEST_FAIL_("TEST_ASSERT_TRUE(%s) was false", #expr);	\
	} while (0)

#define TEST_ASSERT_FALSE(expr)	\
	do {	\
		if ((expr))	\
			TEST_FAIL_("TEST_ASSERT_FALSE(%s) was true", #expr);	\
	} while (0)

#define TEST_ASSERT_NULL(expr)	\
	do {	\
		const void *_v = (const void *)(expr);	\
		if (_v != NULL)	\
			TEST_FAIL_("TEST_ASSERT_NULL(%s) was %p", #expr, _v);	\
	} while (0)

#define TEST_ASSERT_NONNULL(expr)	\
	do {	\
		if ((expr) == NULL)	\
			TEST_FAIL_("TEST_ASSERT_NONNULL(%s) was NULL", #expr);	\
	} while (0)

#define TEST_EQ(a, b)	\
	do {	\
		long long _a = (long long)(a);	\
		long long _b = (long long)(b);	\
		if (_a != _b)	\
			TEST_FAIL_("TEST_EQ(%s, %s): %lld != %lld",	\
				#a, #b, _a, _b);	\
	} while (0)

#define TEST_NE(a, b)	\
	do {	\
		long long _a = (long long)(a);	\
		long long _b = (long long)(b);	\
		if (_a == _b)	\
			TEST_FAIL_("TEST_NE(%s, %s): both %lld",	\
				#a, #b, _a);	\
	} while (0)

#define TEST_STRCMP(actual, expected)	\
	do {	\
		const char *_a = (actual);	\
		const char *_e = (expected);	\
		if (_a == NULL || _e == NULL || strcmp(_a, _e) != 0)	\
			TEST_FAIL_("TEST_STRCMP(%s, %s): [%s] != [%s]",	\
				#actual, #expected,	\
				_a ? _a : "(null)", _e ? _e : "(null)");	\
	} while (0)

#define TEST_STRCASECMP(actual, expected)	\
	do {	\
		const char *_a = (actual);	\
		const char *_e = (expected);	\
		if (_a == NULL || _e == NULL || strcasecmp(_a, _e) != 0)	\
			TEST_FAIL_("TEST_STRCASECMP(%s, %s): [%s] !~ [%s]",	\
				#actual, #expected,	\
				_a ? _a : "(null)", _e ? _e : "(null)");	\
	} while (0)

#define TEST_STRNCMP(actual, expected, n)	\
	do {	\
		const char *_a = (actual);	\
		const char *_e = (expected);	\
		size_t _n = (size_t)(n);	\
		if (_a == NULL || _e == NULL || strncmp(_a, _e, _n) != 0)	\
			TEST_FAIL_("TEST_STRNCMP(%s, %s, %zu): [%s] != [%s]",	\
				#actual, #expected, _n,	\
				_a ? _a : "(null)", _e ? _e : "(null)");	\
	} while (0)

#define TEST_RUN(name, fn)	\
	do {	\
		fprintf(stderr, "%s -> %s:", name, #fn);	\
		fn();	\
		fprintf(stderr, " PASS\n");	\
	} while (0)

static inline const char *
test_progname(const char *argv0)
{
	const char *p;

	if (argv0 == NULL || *argv0 == '\0')
		return "test";

	p = strrchr(argv0, '/');
	if (p != NULL)
		argv0 = p + 1;

#ifdef _WIN32
	p = strrchr(argv0, '\\');
	if (p != NULL)
		argv0 = p + 1;
#endif

	return argv0;
}

static inline pkgconf_client_t *
test_client_new(void)
{
	pkgconf_cross_personality_t *pers = pkgconf_cross_personality_default();
	return pkgconf_client_new(NULL, NULL, pers, NULL, NULL);
}

#endif // TEST_API_H
