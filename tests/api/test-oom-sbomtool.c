/*
 * test-oom-sbomtool.c
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
#include "cyclonedx/structure.h"
#include "cyclonedx/component.h"
#include "cyclonedx/dependencies.h"
#include "util.h"

static pkgconf_client_t *g_client;

static void
oom_setup(void)
{
	g_client = test_client_new();
	// Mirror cli/spdxtool/main.c: the constructors read these globals
	sbomtool_util_set_uri_root(g_client, "https://example.com/space name");
	sbomtool_util_set_uri_separator_colon(g_client, false);
	sbomtool_util_set_sbom_version(g_client, "1.7");
	sbomtool_util_set_sbom_license(g_client, "CC0-1.0");
}

/*
 * ==============================================
 * util / url
 * ==============================================
 */

static void
test_oom_util_id_string(void)
{
	char *id_str = NULL;
	OOM_TEST_PTR(id_str, sbomtool_util_get_sbom_id_string(g_client, "test", "test1"), free(id_str));
}

static void
test_oom_util_id_int(void)
{
	char *id_str = NULL;
	OOM_TEST_PTR(id_str, sbomtool_util_get_sbom_id_int(g_client, "test"), free(id_str));
}

/*
 * =========================
 * Check structure functions
 * =========================
 */
static void
test_oom_cyclonedx_structure_new(void)
{
	pkgconfcli_serialize_value_t *s;
	OOM_TEST_PTR(s, sbomtool_cyclonedx_structure_new(g_client, 1), pkgconfcli_serialize_value_free(s));
}

static void
test_oom_cyclonedx_structure_add_metadata(void)
{
	pkgconfcli_serialize_value_t *sbom_doc = sbomtool_cyclonedx_structure_new(g_client, 1);
	OOM_TEST_BOOL(sbomtool_cyclonedx_structure_add_metadata(g_client, sbom_doc, NULL, "test_agent_name", "https://example.com", true));
	pkgconfcli_serialize_value_free(sbom_doc);
}

/*
 * =========================
 * Check component functions
 * =========================
 */
static void
test_oom_cyclonedx_component_new(void)
{
	pkgconfcli_serialize_value_t *s;
	OOM_TEST_PTR(s, sbomtool_cyclonedx_component_new(g_client, CYCLONEX_COMPONENT_APPLICATION, "test", "1.0.0", true), pkgconfcli_serialize_value_free(s));
}

static void
test_oom_cyclonedx_license(void)
{
	pkgconfcli_serialize_array_t *test_a = pkgconfcli_serialize_array_new();

	OOM_TEST_BOOL(sbomtool_cyclondex_component_add_license(g_client, test_a, "BSD-2-Clause"));

	pkgconfcli_serialize_array_free(test_a);
}

static void
test_oom_cyclonedx_author(void)
{
	pkgconfcli_serialize_array_t *test_a = pkgconfcli_serialize_array_new();

	OOM_TEST_BOOL(sbomtool_cyclondex_component_add_author(g_client, test_a, "test", "test@test.com", "+123123123"));

	pkgconfcli_serialize_array_free(test_a);
}

/*
 * ============================
 * Check dependencies functions
 * ============================
 */
static void
test_oom_cyclonedx_dependencies(void)
{
	pkgconfcli_serialize_value_t *s;
	pkgconfcli_serialize_array_t *test_a = pkgconfcli_serialize_array_new();

	OOM_TEST_PTR(s, sbomtool_cyclonedx_dependencies_new(g_client, "test", test_a), pkgconfcli_serialize_value_free(s));

	pkgconfcli_serialize_array_free(test_a);
}


int
main(int argc, const char **argv)
{
	(void) argc;
	const char *basename = pkgconf_path_find_basename(argv[0]);

	oom_setup();

	TEST_RUN(basename, test_oom_util_id_string);
	TEST_RUN(basename, test_oom_util_id_int);
	TEST_RUN(basename, test_oom_cyclonedx_structure_new);
	TEST_RUN(basename, test_oom_cyclonedx_structure_add_metadata);
	TEST_RUN(basename, test_oom_cyclonedx_component_new);
	TEST_RUN(basename, test_oom_cyclonedx_license);
	TEST_RUN(basename, test_oom_cyclonedx_author);
	TEST_RUN(basename, test_oom_cyclonedx_dependencies);

	pkgconf_client_free(g_client);
	return EXIT_SUCCESS;
}
