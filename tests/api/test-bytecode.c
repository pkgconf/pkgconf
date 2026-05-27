/*
 * test-bytecode.c
 * Tests for the public libpkgconf bytecode API:
 * pkgconf_bytecode_compile and pkgconf_bytecode_eval_str.
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

/*
 * Build a variable list with the given key/value pairs. Caller frees
 * with pkgconf_variable_list_free().
 *
 * The key/value pairs are stored by compiling `value` as bytecode and
 * stashing the result on a pkgconf_variable_t inside the list, which
 * is how the parser builds variable scopes for real .pc files.
 */
static void
seed_variable(pkgconf_list_t *vars, const char *key, const char *value)
{
	pkgconf_variable_t *v = pkgconf_variable_get_or_create(vars, key);
	TEST_ASSERT_NONNULL(v);

	pkgconf_buffer_reset(&v->bcbuf);
	pkgconf_bytecode_compile(&v->bcbuf, value);
	pkgconf_bytecode_from_buffer(&v->bc, &v->bcbuf);
}

static void
test_eval_plain_text(void)
{
	pkgconf_client_t *client = test_client_new();
	pkgconf_list_t vars = PKGCONF_LIST_INITIALIZER;
	bool saw_sysroot = false;

	// A string with no variable references should round-trip unchanged.
	char *out = pkgconf_bytecode_eval_str(client, &vars, "plain text value", &saw_sysroot);

	TEST_ASSERT_NONNULL(out);
	TEST_STRCMP(out, "plain text value");
	TEST_ASSERT_FALSE(saw_sysroot);

	free(out);
	pkgconf_variable_list_free(&vars);
	pkgconf_client_free(client);
}

static void
test_eval_variable_substitution(void)
{
	pkgconf_client_t *client = test_client_new();
	pkgconf_list_t vars = PKGCONF_LIST_INITIALIZER;
	bool saw_sysroot = false;

	seed_variable(&vars, "prefix", "/opt/foo");

	char *out = pkgconf_bytecode_eval_str(client, &vars, "${prefix}/lib", &saw_sysroot);

	TEST_ASSERT_NONNULL(out);
	TEST_STRCMP(out, "/opt/foo/lib");
	TEST_ASSERT_FALSE(saw_sysroot);

	free(out);
	pkgconf_variable_list_free(&vars);
	pkgconf_client_free(client);
}

static void
test_eval_nested_variables(void)
{
	pkgconf_client_t *client = test_client_new();
	pkgconf_list_t vars = PKGCONF_LIST_INITIALIZER;
	bool saw_sysroot = false;

	// Recursive expansion: libdir references prefix.
	seed_variable(&vars, "prefix", "/usr/local");
	seed_variable(&vars, "libdir", "${prefix}/lib");

	char *out = pkgconf_bytecode_eval_str(client, &vars,
		"-L${libdir}", &saw_sysroot);

	TEST_ASSERT_NONNULL(out);
	TEST_STRCMP(out, "-L/usr/local/lib");

	free(out);
	pkgconf_variable_list_free(&vars);
	pkgconf_client_free(client);
}

static void
test_eval_undefined_variable(void)
{
	pkgconf_client_t *client = test_client_new();
	pkgconf_list_t vars = PKGCONF_LIST_INITIALIZER;
	bool saw_sysroot = false;

	// Referencing an undefined variable should produce empty substitution, not failure
	char *out = pkgconf_bytecode_eval_str(client, &vars, "prefix=${nonexistent}/end", &saw_sysroot);

	TEST_ASSERT_NONNULL(out);
	TEST_STRCMP(out, "prefix=/end");

	free(out);
	pkgconf_variable_list_free(&vars);
	pkgconf_client_free(client);
}

static void
test_eval_multiple_variables(void)
{
	pkgconf_client_t *client = test_client_new();
	pkgconf_list_t vars = PKGCONF_LIST_INITIALIZER;
	bool saw_sysroot = false;

	seed_variable(&vars, "prefix", "/usr");
	seed_variable(&vars, "exec_prefix", "/usr/local");

	char *out = pkgconf_bytecode_eval_str(client, &vars, "-I${prefix}/include -L${exec_prefix}/lib", &saw_sysroot);

	TEST_ASSERT_NONNULL(out);
	TEST_STRCMP(out, "-I/usr/include -L/usr/local/lib");

	free(out);
	pkgconf_variable_list_free(&vars);
	pkgconf_client_free(client);
}

static void
test_eval_empty_input(void)
{
	pkgconf_client_t *client = test_client_new();
	pkgconf_list_t vars = PKGCONF_LIST_INITIALIZER;
	bool saw_sysroot = false;

	char *out = pkgconf_bytecode_eval_str(client, &vars,
		"", &saw_sysroot);

	// An empty input may evaluate to either NULL or an empty string
	if (out != NULL)
	{
		TEST_STRCMP(out, "");
		free(out);
	}

	pkgconf_variable_list_free(&vars);
	pkgconf_client_free(client);
}

static void
test_eval_sysroot_detection(void)
{
	pkgconf_client_t *client = test_client_new();
	pkgconf_list_t vars = PKGCONF_LIST_INITIALIZER;
	bool saw_sysroot = false;

	pkgconf_client_set_sysroot_dir(client, "/sysroot");

	seed_variable(&vars, "pc_sysrootdir", "/sysroot");
	seed_variable(&vars, "includedir", "${pc_sysrootdir}/usr/include");

	char *out = pkgconf_bytecode_eval_str(client, &vars, "${includedir}", &saw_sysroot);

	TEST_ASSERT_NONNULL(out);
	TEST_ASSERT_TRUE(saw_sysroot);

	free(out);
	pkgconf_variable_list_free(&vars);
	pkgconf_client_free(client);
}

static void
test_compile_produces_nonempty_buffer(void)
{
	pkgconf_buffer_t bc = PKGCONF_BUFFER_INITIALIZER;

	pkgconf_bytecode_compile(&bc, "hello ${world}");

	// The compiled bytecode buffer should be non-empty
	TEST_NE(pkgconf_buffer_len(&bc), 0);

	pkgconf_buffer_finalize(&bc);
}

static void
test_compile_eval_roundtrip(void)
{
	pkgconf_client_t *client = test_client_new();
	pkgconf_list_t vars = PKGCONF_LIST_INITIALIZER;
	bool saw_sysroot = false;

	/* Compile directly, then evaluate via the bc field on a variable.
	 * This is what the parser does internally for every .pc field. */
	seed_variable(&vars, "name", "world");

	pkgconf_variable_t *v = pkgconf_variable_get_or_create(&vars, "greeting");
	TEST_ASSERT_NONNULL(v);
	pkgconf_buffer_reset(&v->bcbuf);
	pkgconf_bytecode_compile(&v->bcbuf, "hello ${name}");
	pkgconf_bytecode_from_buffer(&v->bc, &v->bcbuf);

	char *out = pkgconf_variable_eval_str(client, &vars, v, &saw_sysroot);
	TEST_ASSERT_NONNULL(out);
	TEST_STRCMP(out, "hello world");

	free(out);
	pkgconf_variable_list_free(&vars);
	pkgconf_client_free(client);
}

int
main(int argc, char *argv[])
{
	(void) argc;
	const char *name = test_progname(argv[0]);

	TEST_RUN(name, test_compile_produces_nonempty_buffer);
	TEST_RUN(name, test_eval_plain_text);
	TEST_RUN(name, test_eval_empty_input);
	TEST_RUN(name, test_eval_variable_substitution);
	TEST_RUN(name, test_eval_nested_variables);
	TEST_RUN(name, test_eval_multiple_variables);
	TEST_RUN(name, test_eval_undefined_variable);
	TEST_RUN(name, test_eval_sysroot_detection);
	TEST_RUN(name, test_compile_eval_roundtrip);

	return EXIT_SUCCESS;
}
