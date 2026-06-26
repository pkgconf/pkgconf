/*
 * spdxtool-fuzzer.c
 * SPDX SBOM serializer fuzzing harness
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

#include "core.h"
#include "software.h"
#include "serialize.h"
#include "simplelicensing.h"
#include "util.h"

#include "alloc-inject.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

/* bound the number of injection rounds per input to keep executions cheap */
#define ALLOC_FAIL_MAX 4096

/* the fuzzer input is split on NUL bytes into up to this many .pc files, named
 * a.pc, b.pc, ... so that Requires/Conflicts/Provides between them resolve, and
 * the resulting graph is serialized to an SPDX SBOM.
 */
#define UNIVERSE_MAX 4
#define SOLVE_MAXDEPTH 10

static const char universe_names[UNIVERSE_MAX] = { 'a', 'b', 'c', 'd' };

static const char *
environ_lookup_handler(const pkgconf_client_t *client, const char *key)
{
	(void) client;
	(void) key;

	return NULL;
}

static int
write_all(int fd, const uint8_t *data, size_t size)
{
	while (size > 0)
	{
		ssize_t n = write(fd, data, size);

		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		}

		data += n;
		size -= n;
	}

	return 0;
}

static void
write_pkg(const char *dir, char name, const uint8_t *data, size_t size)
{
	char path[PKGCONF_ITEM_SIZE];
	int fd;

	snprintf(path, sizeof path, "%s/%c.pc", dir, name);

	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd < 0)
		return;

	write_all(fd, data, size);
	close(fd);
}

static void
write_universe(const char *dir, const uint8_t *data, size_t size)
{
	size_t start = 0, idx = 0;

	for (size_t i = 0; i < size && idx < UNIVERSE_MAX; i++)
	{
		if (data[i] == 0x00)
		{
			write_pkg(dir, universe_names[idx++], data + start, i - start);
			start = i + 1;
		}
	}

	if (idx < UNIVERSE_MAX)
		write_pkg(dir, universe_names[idx], data + start, size - start);
}

static void
cleanup_universe(const char *dir)
{
	char path[PKGCONF_ITEM_SIZE];

	for (size_t i = 0; i < UNIVERSE_MAX; i++)
	{
		snprintf(path, sizeof path, "%s/%c.pc", dir, universe_names[i]);
		unlink(path);
	}

	rmdir(dir);
}

/*
 * Mirror of cli/spdxtool/main.c generate_spdx_package(): registers a resolved
 * package as an SBOM rootElement on the document, populating the per-package
 * tuples the serializers later read back.  Kept in sync with main.c by hand.
 */
static void
generate_spdx_package(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *ptr)
{
	spdxtool_core_spdx_document_t *document = (spdxtool_core_spdx_document_t *)ptr;
	pkgconf_node_t *node = NULL;
	spdxtool_software_sbom_t *sbom = NULL;
	char *package_spdx = NULL;
	char *spdx_id_string = NULL;
	char sep = spdxtool_util_get_uri_separator(client);

	if (pkg->flags & PKGCONF_PKG_PROPF_VIRTUAL)
		return;

	spdx_id_string = spdxtool_util_get_spdx_id_string(client, "software_Sbom", pkg->id);
	if (!spdx_id_string)
		goto err;

	sbom = spdxtool_software_sbom_new(client, spdx_id_string, document->creation_info, "build");
	free(spdx_id_string);
	spdx_id_string = NULL;
	if (!sbom)
		goto err;

	sbom->spdx_document = document;
	sbom->rootElement = pkg;

	package_spdx = spdxtool_util_get_spdx_id_string(client, "Package", pkg->id);
	if (!package_spdx)
		goto err;

	pkgconf_tuple_add(client, &pkg->vars, "spdxId", package_spdx, false, 0);
	free(package_spdx);
	package_spdx = NULL;

	pkgconf_tuple_add(client, &pkg->vars, "creationInfo", document->creation_info, false, 0);
	pkgconf_tuple_add(client, &pkg->vars, "agent", document->agent, false, 0);

	if (pkg->maintainer != NULL)
	{
		const char *supplier = spdxtool_core_spdx_document_add_maintainer(client, document, pkg->maintainer);
		if (!supplier)
			goto err;

		pkgconf_tuple_add(client, &pkg->vars, "suppliedBy", supplier, false, 0);
	}

	if (pkg->license.head != NULL)
	{
		pkgconf_buffer_t spdx_id_buf = PKGCONF_BUFFER_INITIALIZER;

		pkgconf_buffer_append_fmt(&spdx_id_buf, "%s%chasDeclaredLicense", pkg->id, sep);
		char *spdx_id_name = pkgconf_buffer_freeze(&spdx_id_buf);
		if (!spdx_id_name)
			goto err;

		package_spdx = spdxtool_util_get_spdx_id_string(client, "Relationship", spdx_id_name);
		free(spdx_id_name);
		if (!package_spdx)
			goto err;

		pkgconf_tuple_add(client, &pkg->vars, "hasDeclaredLicense", package_spdx, false, 0);
		free(package_spdx);
		package_spdx = NULL;

		pkgconf_buffer_t concluded_buf = PKGCONF_BUFFER_INITIALIZER;
		pkgconf_buffer_append_fmt(&concluded_buf, "%s%chasConcludedLicense", pkg->id, sep);
		spdx_id_name = pkgconf_buffer_freeze(&concluded_buf);
		if (!spdx_id_name)
			goto err;

		package_spdx = spdxtool_util_get_spdx_id_string(client, "Relationship", spdx_id_name);
		free(spdx_id_name);
		if (!package_spdx)
			goto err;

		pkgconf_tuple_add(client, &pkg->vars, "hasConcludedLicense", package_spdx, false, 0);
		free(package_spdx);
		package_spdx = NULL;

		PKGCONF_FOREACH_LIST_ENTRY(pkg->license.head, node)
		{
			const pkgconf_license_t *license = node->data;
			if (license->type == PKGCONF_LICENSE_EXPRESSION)
			{
				if (!spdxtool_core_spdx_document_add_license(client, document, license->data))
					goto err;
			}
		}
	}

	node = calloc(1, sizeof(pkgconf_node_t));
	if (!node)
		goto err;

	pkgconf_node_insert_tail(node, sbom, &document->rootElement);
	return;

err:
	free(package_spdx);
	free(spdx_id_string);
	spdxtool_software_sbom_free(sbom);
}

/*
 * Mirror of cli/spdxtool/main.c generate_spdx(): solve a freshly-written
 * universe rooted at "a", then drive the full SBOM serialization pipeline.
 */
static void
run_spdx(const pkgconf_cross_personality_t *pers, const char *dir)
{
	pkgconf_client_t *client = pkgconf_client_new(NULL, NULL, pers, NULL, environ_lookup_handler);
	if (client == NULL)
		return;

	pkgconf_client_set_flags(client, PKGCONF_PKG_PKGF_SEARCH_PRIVATE);
	pkgconf_path_add(dir, &client->dir_list, false);

	/* the serializers read these back via the client's global tuples */
	spdxtool_util_set_uri_root(client, "https://example.com/test");
	spdxtool_util_set_uri_separator_colon(client, false);
	spdxtool_util_set_spdx_license(client, "CC0-1.0");
	spdxtool_util_set_spdx_version(client, "3.0.1");

	pkgconf_pkg_t world = {
		.id = "virtual:world",
		.realname = "virtual world package",
		.flags = PKGCONF_PKG_PROPF_STATIC | PKGCONF_PKG_PROPF_VIRTUAL,
	};
	pkgconf_list_t pkgq = PKGCONF_LIST_INITIALIZER;

	pkgconf_queue_push(&pkgq, "a");

	if (pkgconf_queue_solve(client, &pkgq, &world, SOLVE_MAXDEPTH))
	{
		spdxtool_core_agent_t *agent = spdxtool_core_agent_new(client, "_:creationinfo_1", "Default");

		if (agent != NULL)
		{
			/* fixed timestamp keeps the harness deterministic */
			spdxtool_core_creation_info_t *creation =
				spdxtool_core_creation_info_new(client, agent->spdx_id, "_:creationinfo_1", "2020-01-01T00:00:00Z");

			if (creation != NULL)
			{
				char *spdx_id_int = spdxtool_util_get_spdx_id_int(client, "spdxDocument");
				spdxtool_core_spdx_document_t *document =
					spdxtool_core_spdx_document_new(client, spdx_id_int, "_:creationinfo_1", agent->spdx_id);
				free(spdx_id_int);

				if (document != NULL)
				{
					int eflag = pkgconf_pkg_traverse(client, &world, generate_spdx_package, document, SOLVE_MAXDEPTH, 0);

					if (eflag == PKGCONF_PKG_ERRF_OK)
					{
						spdxtool_serialize_value_t *root = spdxtool_serialize_sbom(client, agent, creation, document);

						if (root != NULL)
						{
							pkgconf_buffer_t buffer = PKGCONF_BUFFER_INITIALIZER;

							spdxtool_serialize_value_to_buf(&buffer, root, 0);
							pkgconf_buffer_finalize(&buffer);
							spdxtool_serialize_value_free(root);
						}
					}

					spdxtool_core_spdx_document_free(document);
				}

				spdxtool_core_creation_info_free(creation);
			}

			spdxtool_core_agent_free(agent);
		}
	}

	pkgconf_solution_free(client, &world);
	pkgconf_queue_free(&pkgq);
	pkgconf_client_free(client);
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size == 0)
		return 0;

	char dir[] = "/tmp/pkgconf-fuzz-spdx-XXXXXX";
	if (mkdtemp(dir) == NULL)
		return 0;

	write_universe(dir, data, size);

	pkgconf_cross_personality_t *pers = pkgconf_cross_personality_default();

	/* baseline run with all allocations succeeding */
	run_spdx(pers, dir);

	/* then fail each allocation site reachable by this input, one at a time */
	for (unsigned long i = 1; i <= ALLOC_FAIL_MAX; i++)
	{
		alloc_inject_arm(i);
		run_spdx(pers, dir);
		alloc_inject_disarm();

		if (!alloc_inject_fired())
			break;
	}

	pkgconf_cross_personality_deinit(pers);
	cleanup_universe(dir);

	return 0;
}
