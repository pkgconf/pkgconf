/*
 * test-runner.c
 * test harness
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

#include <libpkgconf/libpkgconf.h>
#include <libpkgconf/stdinc.h>
#include <cli/core.h>
#include <cli/getopt_long.h>

static char *test_fixtures_dir = NULL;

typedef enum test_match_strategy_ {
	MATCH_EXACT = 0,
	MATCH_PARTIAL,
} pkgconf_test_match_strategy_t;

typedef struct test_case_ {
	char *name;

	pkgconf_list_t search_path;
	pkgconf_buffer_t query;

	pkgconf_buffer_t expected_stdout;
	pkgconf_test_match_strategy_t match_stdout;

	pkgconf_buffer_t expected_stderr;
	pkgconf_test_match_strategy_t match_stderr;

	int exitcode;
	uint64_t wanted_flags;

	pkgconf_list_t env_vars;
} pkgconf_test_case_t;

typedef struct test_state_ {
	pkgconf_cli_state_t cli_state;
	const pkgconf_test_case_t *testcase;
} pkgconf_test_state_t;

typedef struct test_environ_ {
	pkgconf_node_t node;
	char *key;
	char *value;
} pkgconf_test_environ_t;

typedef struct test_output_ {
	pkgconf_output_t output;

	pkgconf_buffer_t o_stdout;
	pkgconf_buffer_t o_stderr;
} pkgconf_test_output_t;

void
test_environment_push(pkgconf_test_case_t *testcase, const char *key, const char *value)
{
	pkgconf_test_environ_t *environ = calloc(1, sizeof(*environ));
	if (environ == NULL)
		return;

	environ->key = strdup(key);
	environ->value = strdup(value);
	pkgconf_node_insert_tail(&environ->node, environ, &testcase->env_vars);
}

void
test_environment_free(pkgconf_list_t *env_list)
{
	pkgconf_node_t *n, *tn;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(env_list->head, tn, n)
	{
		pkgconf_test_environ_t *environ = n->data;

		pkgconf_node_delete(&environ->node, env_list);

		free(environ->key);
		free(environ->value);
		free(environ);
	}
}

static const char *
environ_lookup_handler(const pkgconf_client_t *client, const char *key)
{
	pkgconf_test_state_t *state = client->client_data;
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(state->testcase->env_vars.head, node)
	{
		pkgconf_test_environ_t *environ = node->data;

		if (!strcmp(key, environ->key))
			return environ->value;
	}

	return NULL;
}

static bool
error_handler(const char *msg, const pkgconf_client_t *client, void *data)
{
	(void) data;
	pkgconf_output_fmt(client->output, PKGCONF_OUTPUT_STDERR, "%s", msg);
	return true;
}

static bool
write_handler(pkgconf_output_t *output, pkgconf_output_stream_t stream, const pkgconf_buffer_t *buffer)
{
	pkgconf_test_output_t *out = (pkgconf_test_output_t *) output;
	pkgconf_buffer_t *dest = stream == PKGCONF_OUTPUT_STDERR ? &out->o_stderr : &out->o_stdout;

	pkgconf_buffer_append(dest, pkgconf_buffer_str(buffer));
	return true;
}

static pkgconf_output_t *
test_output(void)
{
	static pkgconf_test_output_t output = {
		.output.write = write_handler,
	};

	return &output.output;
}

static void
test_output_reset(pkgconf_test_output_t *out)
{
	pkgconf_buffer_reset(&out->o_stdout);
	pkgconf_buffer_reset(&out->o_stderr);
}

typedef void (*test_keyword_func_t)(pkgconf_test_case_t *testcase, const char *keyword, const char *warnprefix, const ptrdiff_t offset, char *value);

typedef struct test_keyword_pair_ {
	const char *keyword;
	const test_keyword_func_t func;
	const ptrdiff_t offset;
} pkgconf_test_keyword_pair_t;

static int
test_keyword_pair_cmp(const void *key, const void *ptr)
{
	const pkgconf_test_keyword_pair_t *pair = ptr;
	return strcasecmp(key, pair->keyword);
}

static void
test_keyword_set_int(pkgconf_test_case_t *testcase, const char *keyword, const char *warnprefix, const ptrdiff_t offset, char *value)
{
	(void) keyword;
	(void) warnprefix;

	int *dest = (int *)((char *) testcase + offset);
	*dest = atoi(value);
}

static void
test_keyword_set_buffer(pkgconf_test_case_t *testcase, const char *keyword, const char *warnprefix, const ptrdiff_t offset, char *value)
{
	(void) keyword;
	(void) warnprefix;

	pkgconf_buffer_t *dest = (pkgconf_buffer_t *)((char *) testcase + offset);
	pkgconf_buffer_append(dest, value);
}

typedef struct test_flag_pair_ {
	const char *name;
	uint64_t flag;
} pkgconf_test_flag_pair_t;

static int
test_flag_pair_cmp(const void *key, const void *ptr)
{
	const pkgconf_test_flag_pair_t *pair = ptr;
	return strcasecmp(key, pair->name);
}

static const pkgconf_test_flag_pair_t test_flag_pairs[] = {
	{"cflags",		PKG_CFLAGS},
	{"cflags-only-i",	PKG_CFLAGS_ONLY_I},
	{"cflags-only-other",	PKG_CFLAGS_ONLY_OTHER},
	{"debug",		PKG_DEBUG},
	{"define-prefix",	PKG_DEFINE_PREFIX},
	{"digraph",		PKG_DIGRAPH},
	{"dont-define-prefix",	PKG_DONT_DEFINE_PREFIX},
	{"dont-relocate-paths",	PKG_DONT_RELOCATE_PATHS},
	{"dump-license",	PKG_DUMP_LICENSE},
	{"dump-license-file",	PKG_DUMP_LICENSE_FILE},
	{"dump-personality",	PKG_DUMP_PERSONALITY},
	{"dump-source",		PKG_DUMP_SOURCE},
	{"env-only",		PKG_ENV_ONLY},
	{"errors-on-stdout",	PKG_ERRORS_ON_STDOUT},
	{"exists",		PKG_EXISTS},
	{"exists-cflags",	PKG_EXISTS_CFLAGS},
	{"fragment-tree",	PKG_FRAGMENT_TREE},
	{"keep-system-cflags",	PKG_KEEP_SYSTEM_CFLAGS},
	{"keep-system-libs",	PKG_KEEP_SYSTEM_LIBS},
	{"ignore-conflicts",	PKG_IGNORE_CONFLICTS},
	{"internal-cflags",	PKG_INTERNAL_CFLAGS},
	{"libs",		PKG_LIBS},
	{"libs-only-ldpath",	PKG_LIBS_ONLY_LDPATH},
	{"libs-only-libname",	PKG_LIBS_ONLY_LIBNAME},
	{"libs-only-other",	PKG_LIBS_ONLY_OTHER},
	{"list",		PKG_LIST},
	{"list-package-names",	PKG_LIST_PACKAGE_NAMES},
	{"modversion",		PKG_MODVERSION},
	{"msvc-syntax",		PKG_MSVC_SYNTAX},
	{"newlines",		PKG_NEWLINES},
	{"no-cache",		PKG_NO_CACHE},
	{"no-provides",		PKG_NO_PROVIDES},
	{"no-uninstalled",	PKG_NO_UNINSTALLED},
	{"path",		PKG_PATH},
	{"print-errors",	PKG_PRINT_ERRORS},
	{"provides",		PKG_PROVIDES},
	{"pure",		PKG_PURE},
	{"requires",		PKG_REQUIRES},
	{"requires-private",	PKG_REQUIRES_PRIVATE},
	{"shared",		PKG_SHARED},
	{"short-errors",	PKG_SHORT_ERRORS},
	{"silence-errors",	PKG_SILENCE_ERRORS},
	{"simulate",		PKG_SIMULATE},
	{"solution",		PKG_SOLUTION},
	{"static",		PKG_STATIC},
	{"uninstalled",		PKG_UNINSTALLED},
	{"validate",		PKG_VALIDATE},
	{"variables",		PKG_VARIABLES},
};

static void
test_keyword_set_wanted_flags(pkgconf_test_case_t *testcase, const char *keyword, const char *warnprefix, const ptrdiff_t offset, char *value)
{
	int i;
	int flagcount;
	char **flags = NULL;

	(void) keyword;
	(void) warnprefix;
	(void) offset;

	pkgconf_argv_split(value, &flagcount, &flags);

	for (i = 0; i < flagcount; i++)
	{
		const char *flag = flags[i];
		const pkgconf_test_flag_pair_t *pair = bsearch(flag,
			test_flag_pairs, PKGCONF_ARRAY_SIZE(test_flag_pairs),
			sizeof(*pair), test_flag_pair_cmp);

		if (pair == NULL)
			continue;

		testcase->wanted_flags |= pair->flag;
	}

	pkgconf_argv_free(flags);
}

static size_t
prefixed_path_split(const char *text, pkgconf_list_t *dirlist, const char *prefix)
{
	size_t count = 0;
	char *workbuf, *p, *iter;

	if (text == NULL)
		return 0;

	iter = workbuf = strdup(text);
	while ((p = strtok(iter, PKG_CONFIG_PATH_SEP_S)) != NULL)
	{
		pkgconf_buffer_t pathbuf = PKGCONF_BUFFER_INITIALIZER;

		pkgconf_buffer_append(&pathbuf, prefix);
		pkgconf_buffer_append(&pathbuf, "/");
		pkgconf_buffer_append(&pathbuf, p);
		pkgconf_path_add(pkgconf_buffer_str(&pathbuf), dirlist, false);
		pkgconf_buffer_finalize(&pathbuf);

		count++, iter = NULL;
	}
	free(workbuf);

	return count;
}

static void
test_keyword_set_path_list(pkgconf_test_case_t *testcase, const char *keyword, const char *warnprefix, const ptrdiff_t offset, char *value)
{
	(void) keyword;
	(void) warnprefix;

	pkgconf_list_t *dest = (pkgconf_list_t *)((char *) testcase + offset);
	prefixed_path_split(value, dest, test_fixtures_dir);
}

static void
test_keyword_set_match_strategy(pkgconf_test_case_t *testcase, const char *keyword, const char *warnprefix, const ptrdiff_t offset, char *value)
{
	(void) keyword;
	(void) warnprefix;

	pkgconf_test_match_strategy_t *dest = (pkgconf_test_match_strategy_t *)((char *) testcase + offset);

	if (!strcasecmp(value, "partial"))
		*dest = MATCH_PARTIAL;
}

static void
test_keyword_set_environment(pkgconf_test_case_t *testcase, const char *keyword, const char *warnprefix, const ptrdiff_t offset, char *value)
{
	(void) keyword;
	(void) offset;

	char *eq = strchr(value, '=');
	if (eq == NULL)
	{
		fprintf(stderr, "%s: malformed Environment entry: %s\n", warnprefix, value);
		return;
	}

	*eq++ = '\0';
	test_environment_push(testcase, value, eq);
}

static const pkgconf_test_keyword_pair_t test_keyword_pairs[] = {
	{"Environment", test_keyword_set_environment, offsetof(pkgconf_test_case_t, env_vars)},
	{"ExpectedExitCode", test_keyword_set_int, offsetof(pkgconf_test_case_t, exitcode)},
	{"ExpectedStderr", test_keyword_set_buffer, offsetof(pkgconf_test_case_t, expected_stderr)},
	{"ExpectedStdout", test_keyword_set_buffer, offsetof(pkgconf_test_case_t, expected_stdout)},
	{"MatchStderr", test_keyword_set_match_strategy, offsetof(pkgconf_test_case_t, match_stderr)},
	{"MatchStdout", test_keyword_set_match_strategy, offsetof(pkgconf_test_case_t, match_stdout)},
	{"PackageSearchPath", test_keyword_set_path_list, offsetof(pkgconf_test_case_t, search_path)},
	{"Query", test_keyword_set_buffer, offsetof(pkgconf_test_case_t, query)},
	{"WantedFlags", test_keyword_set_wanted_flags, offsetof(pkgconf_test_case_t, wanted_flags)},
};

static void
test_keyword_set(pkgconf_test_case_t *testcase, const char *warnprefix, const char *keyword, char *value)
{
	const pkgconf_test_keyword_pair_t *pair = bsearch(keyword,
		test_keyword_pairs, PKGCONF_ARRAY_SIZE(test_keyword_pairs),
		sizeof(*pair), test_keyword_pair_cmp);

	if (pair == NULL || pair->func == NULL)
		return;

	pair->func(testcase, warnprefix, keyword, pair->offset, value);
}

static const pkgconf_parser_operand_func_t test_parser_ops[256] = {
	[':'] = (pkgconf_parser_operand_func_t) test_keyword_set,
};

static void
test_parser_warn(void *p, const char *fmt, ...)
{
	va_list va;

	(void) p;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

pkgconf_test_case_t *
load_test_case(char *testfile)
{
	FILE *testf = fopen(testfile, "r");
	if (testf == NULL)
		return NULL;

	pkgconf_test_case_t *out = calloc(1, sizeof(*out));
	if (out == NULL)
		goto cleanup;

	out->name = strdup(basename(testfile));
	pkgconf_parser_parse(testf, out, test_parser_ops, test_parser_warn, testfile);

cleanup:
	fclose(testf);
	return out;
}

/* we use a custom personality to ensure the tests are fully hermetic */
pkgconf_cross_personality_t *
personality_for_test(const pkgconf_test_case_t *testcase)
{
	pkgconf_cross_personality_t *pers = calloc(1, sizeof(*pers));
	if (pers == NULL)
		return NULL;

	pers->name = strdup("test");
	pkgconf_path_copy_list(&pers->dir_list, &testcase->search_path);
	pkgconf_path_add("/test/sysroot/include", &pers->filter_includedirs, false);
	pkgconf_path_add("/test/sysroot/lib", &pers->filter_libdirs, false);

	return pers;
}

static bool
report_failure(pkgconf_test_match_strategy_t match, const pkgconf_buffer_t *expected, const pkgconf_buffer_t *actual, const char *buffername, const char *testname)
{
	printf("FAIL: %s\n", testname);
	fprintf(stderr,
		"================================================================================\n"
		"%s did not%s match:\n"
		"  expected: [%s]\n"
		"  actual: [%s]\n"
		"================================================================================\n",
		buffername, match == MATCH_PARTIAL ? " partially" : "",
		pkgconf_buffer_str_or_empty(expected),
		pkgconf_buffer_str_or_empty(actual));

	return false;
}

bool
test_match_buffer(pkgconf_test_match_strategy_t match, const pkgconf_buffer_t *expected, const pkgconf_buffer_t *actual, const char *buffername, const char *testname)
{
	if (!pkgconf_buffer_len(expected))
		return true;

	if (!pkgconf_buffer_len(actual))
		return report_failure(match, expected, actual, buffername, testname);

	if (match == MATCH_PARTIAL)
		return pkgconf_buffer_contains(actual, expected) ? true : report_failure(match, expected, actual, buffername, testname);

	return pkgconf_buffer_match(actual, expected) ? true : report_failure(match, expected, actual, buffername, testname);
}

bool
run_test_case(const pkgconf_test_case_t *testcase)
{
	pkgconf_cross_personality_t *personality = personality_for_test(testcase);
	pkgconf_test_state_t state = {
		.cli_state.want_flags = testcase->wanted_flags,
		.testcase = testcase,
	};

	pkgconf_test_output_t *out = (pkgconf_test_output_t *) test_output();

	pkgconf_client_init(&state.cli_state.pkg_client, error_handler, NULL, personality, &state, environ_lookup_handler);
	pkgconf_client_set_output(&state.cli_state.pkg_client, &out->output);

	pkgconf_buffer_t arg_buf = PKGCONF_BUFFER_INITIALIZER;
	int test_argc = 0;
	char **test_argv = NULL;

	pkgconf_buffer_append_fmt(&arg_buf, "pkgconf %s", pkgconf_buffer_str_or_empty(&testcase->query));
	pkgconf_argv_split(pkgconf_buffer_str(&arg_buf), &test_argc, &test_argv);
	pkgconf_buffer_finalize(&arg_buf);

	int ret = pkgconf_cli_run(&state.cli_state, test_argc, test_argv, 1);
	pkgconf_argv_free(test_argv);

	if (pkgconf_buffer_len(&out->o_stdout))
		pkgconf_buffer_trim_byte(&out->o_stdout);

	if (pkgconf_buffer_len(&out->o_stderr))
		pkgconf_buffer_trim_byte(&out->o_stderr);

	if (!test_match_buffer(testcase->match_stdout, &testcase->expected_stdout, &out->o_stdout, "stdout", testcase->name))
		return false;

	if (!test_match_buffer(testcase->match_stderr, &testcase->expected_stderr, &out->o_stderr, "stderr", testcase->name))
		return false;

	test_output_reset(out);

	if (ret != testcase->exitcode)
	{
		fprintf(stderr, "exitcode %d does not match expected %d\n", ret, testcase->exitcode);
		return false;
	}

	printf("PASS: %s\n", testcase->name);

	return true;
}

void
free_test_case(pkgconf_test_case_t *testcase)
{
	test_environment_free(&testcase->env_vars);
	pkgconf_path_free(&testcase->search_path);

	pkgconf_buffer_finalize(&testcase->expected_stderr);
	pkgconf_buffer_finalize(&testcase->expected_stdout);
	pkgconf_buffer_finalize(&testcase->query);

	free(testcase->name);
	free(testcase);
}

bool
process_test_case(char *testcase_file)
{
	pkgconf_test_case_t *testcase = load_test_case(testcase_file);
	bool ret;

	if (testcase == NULL)
	{
		fprintf(stderr, "test %s failed to load\n", testcase_file);
		return false;
	}

	ret = run_test_case(testcase);
	free_test_case(testcase);

	return ret;
}

static inline bool
str_has_suffix(const char *str, const char *suffix)
{
	size_t str_len = strlen(str);
	size_t suf_len = strlen(suffix);

	if (str_len < suf_len)
		return false;

	return !strncasecmp(str + str_len - suf_len, suffix, suf_len);
}

bool
process_test_directory(char *dirpath)
{
	bool ret = true;
	DIR *dir = opendir(dirpath);
	if (dir == NULL)
	{
		fprintf(stderr, "failed to open test directory %s\n", dirpath);
		return false;
	}

	struct dirent *dirent;

	for (dirent = readdir(dir); dirent != NULL; dirent = readdir(dir))
	{
		pkgconf_buffer_t pathbuf = PKGCONF_BUFFER_INITIALIZER;

		pkgconf_buffer_append(&pathbuf, dirpath);
		pkgconf_buffer_append(&pathbuf, "/");
		pkgconf_buffer_append(&pathbuf, dirent->d_name);

		char *pathstr = pkgconf_buffer_freeze(&pathbuf);
		if (pathstr == NULL)
			continue;

		if (!str_has_suffix(pathstr, ".test"))
		{
			free(pathstr);
			continue;
		}

		ret = process_test_case(pathstr);
		free(pathstr);

		if (!ret)
			break;
	}

	closedir(dir);
	return ret;
}

void
usage(void)
{
	fprintf(stderr, "usage: test-runner --test-fixtures <path-to-fixtures> <path-to-tests>\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int ret;

	struct pkg_option options[] = {
		{"test-fixtures", required_argument, NULL, 1},
		{NULL, 0, NULL, 0},
	};

	while ((ret = pkg_getopt_long_only(argc, argv, "", options, NULL)) != -1)
	{
		switch (ret)
		{
		case 1:
			test_fixtures_dir = pkg_optarg;
			break;
		}
	}

	if (test_fixtures_dir == NULL)
		usage();

	if (argc < 2)
		usage();

	return process_test_directory(argv[pkg_optind]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
