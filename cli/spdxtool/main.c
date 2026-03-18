/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2025 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include "libpkgconf/config.h"
#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>
#include "getopt_long.h"
#include "util.h"
#include "core.h"
#include "software.h"
#include "serialize.h"
#include "simplelicensing.h"

#define PKG_VERSION			(((uint64_t) 1) << 1)
#define PKG_ABOUT			(((uint64_t) 1) << 2)
#define PKG_HELP			(((uint64_t) 1) << 3)

static const char *spdx_version = "3.0.1";
static const char *bom_license = "CC0-1.0";
static const char *xsd_any_uri_default_base = "https://github.com/pkgconf/pkgconf";
static int maximum_traverse_depth = 2000;

static pkgconf_client_t pkg_client;
static uint64_t want_flags;
static size_t maximum_package_count = 0;
// static int maximum_traverse_depth = 2000;
static FILE *error_msgout = NULL;
static FILE *sbom_out = NULL;

static const char *
environ_lookup_handler(const pkgconf_client_t *client, const char *key)
{
	(void) client;

	return getenv(key);
}

static bool
error_handler(const char *msg, const pkgconf_client_t *client, void *data)
{
	(void) client;
	(void) data;
	fprintf(error_msgout, "%s", msg);
	return true;
}

static void
write_jsonld_end(pkgconf_client_t *client, pkgconf_pkg_t *world, pkgconf_buffer_t *buffer)
{
	(void) client;
	(void) world;

	spdxtool_serialize_array_end(buffer, 1, false);
	spdxtool_serialize_obj_end(buffer, 0, false);
}

static void
write_jsonld_header(pkgconf_client_t *client, pkgconf_pkg_t *world, pkgconf_buffer_t *buffer)
{
	(void) client;
	(void) world;

	spdxtool_serialize_obj_start(buffer, 0);
	spdxtool_serialize_parm_and_string(buffer, "@context", "https://spdx.org/rdf/3.0.1/spdx-context.jsonld", 1, true);

	spdxtool_serialize_parm_and_char(buffer, "@graph", '[', 1, false);
}

// NOTE: this function is passed to pkgconf_pkg_traverse
static void
generate_spdx_package(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *ptr)
{
	spdxtool_core_spdx_document_t *document = (spdxtool_core_spdx_document_t *)ptr;
	pkgconf_node_t *node = NULL;
	char *package_spdx = NULL, *spdx_id_string = NULL;
	char spdx_id_name[1024];

	if (pkg->flags & PKGCONF_PKG_PROPF_VIRTUAL)
		return;

	spdx_id_string = spdxtool_util_get_spdx_id_string(client, "software_Sbom", pkg->id);
	if(!spdx_id_string)
	{
		pkgconf_error(client, "generate_spdx_package: Couldn't create spdx_id_string");
		return;
	}

	spdxtool_software_sbom_t *sbom = spdxtool_software_sbom_new(client, spdx_id_string, document->creation_info, "build");
	if(!sbom)
	{
		pkgconf_error(client, "generate_spdx_package: Couldn't create sbom struct");
		return;
	}
	sbom->spdx_document = document;
	sbom->rootElement = pkg;

	free(spdx_id_string);
	spdx_id_string = NULL;

	package_spdx = spdxtool_util_get_spdx_id_string(client, "Package", pkg->id);
	if(!package_spdx)
	{
		pkgconf_error(client, "generate_spdx_package: Couldn't create spdx id string");
		return;
	}

	pkgconf_tuple_add(client, &pkg->vars, "spdxId", package_spdx, false, 0);
	free(package_spdx);
	package_spdx = NULL;

	pkgconf_tuple_add(client, &pkg->vars, "creationInfo", document->creation_info, false, 0);
	pkgconf_tuple_add(client, &pkg->vars, "agent", document->agent, false, 0);

	if (pkg->license != NULL)
	{
		// TODO: replace with pkgconf_buffer_t
		snprintf(spdx_id_name, sizeof(spdx_id_name), "%s/hasDeclaredLicense", pkg->id);
		package_spdx = spdxtool_util_get_spdx_id_string(client, "Relationship", spdx_id_name);
		if(!package_spdx)
		{
			pkgconf_error(client, "generate_spdx_package: Couldn't create spdx id string");
			return;
		}

		pkgconf_tuple_add(client, &pkg->vars, "hasDeclaredLicense", package_spdx, false, 0);

		free(package_spdx);
		package_spdx = NULL;

		// TODO: replace with pkgconf_buffer_t
		snprintf(spdx_id_name, sizeof(spdx_id_name), "%s/hasConcludedLicense", pkg->id);
		package_spdx = spdxtool_util_get_spdx_id_string(client, "Relationship", spdx_id_name);
		if(!package_spdx)
		{
			pkgconf_error(client, "generate_spdx_package: Couldn't create spdx id string");
			return;
		}

		pkgconf_tuple_add(client, &pkg->vars, "hasConcludedLicense", package_spdx, false, 0);

		free(package_spdx);
		package_spdx = NULL;

		spdxtool_core_spdx_document_add_license(client, document, pkg->license);
	}

	node = calloc(1, sizeof(pkgconf_node_t));
	if(!node)
	{
		pkgconf_error(client, "Memory exhausted!");
		return;
	}

	pkgconf_node_insert_tail(node, sbom, &document->rootElement);
}

static bool
generate_spdx(pkgconf_client_t *client, pkgconf_pkg_t *world, const char *creation_time, const char *creation_id, const char *agent_name)
{
	int eflag;
	pkgconf_buffer_t buffer = PKGCONF_BUFFER_INITIALIZER;

	const char *agent_name_string = agent_name ? agent_name : "Default";
	const char *creation_id_string = creation_id ? creation_id : "_:creationinfo_1";
	const char *creation_time_string = creation_time ? creation_time : NULL;

	spdxtool_core_agent_t *agent = spdxtool_core_agent_new(client, creation_id_string, agent_name_string);
	if(!agent)
	{
		pkgconf_error(client, "Could not create agent struct");
		return false;
	}

	spdxtool_core_creation_info_t *creation = spdxtool_core_creation_info_new(&pkg_client, agent->spdx_id, creation_id_string, creation_time_string);
	if(!creation)
	{
		pkgconf_error(client, "Could not create creation info struct");
		spdxtool_core_agent_free(agent);
		return false;
	}

	char *spdx_id_int = spdxtool_util_get_spdx_id_int(&pkg_client, "spdxDocument");
	spdxtool_core_spdx_document_t *document = spdxtool_core_spdx_document_new(&pkg_client, spdx_id_int, creation_id_string, agent->spdx_id);
	free(spdx_id_int);
	if(!document)
	{
		pkgconf_error(client, "Could not create document");
		spdxtool_core_spdx_document_free(document);
		spdxtool_core_agent_free(agent);
		return false;
	}

	eflag = pkgconf_pkg_traverse(client, world, generate_spdx_package, document, maximum_traverse_depth, 0);
	if (eflag != PKGCONF_PKG_ERRF_OK)
	{
		spdxtool_core_spdx_document_free(document);
		spdxtool_core_agent_free(agent);
		return false;
	}

	write_jsonld_header(client, world, &buffer);

	spdxtool_core_agent_serialize(client, &buffer, agent, true);
	spdxtool_core_creation_info_serialize(client, &buffer, creation, true);

	spdxtool_simplelicensing_licenseExpression_serialize(client, &buffer, document, true);

	spdxtool_core_spdx_document_serialize(client, &buffer, document, false);

	write_jsonld_end(client, world, &buffer);

	spdxtool_core_spdx_document_free(document);
	spdxtool_core_agent_free(agent);
	spdxtool_core_creation_info_free(creation);

	fprintf(sbom_out, "%s\n", pkgconf_buffer_str(&buffer));

	pkgconf_buffer_finalize(&buffer);
	return true;
}

static int
version(void)
{
	printf("spdxtool %s\n", PACKAGE_VERSION);
	return EXIT_SUCCESS;
}

static int
about(void)
{
	printf("spdxtool (%s %s)\n\n", PACKAGE_NAME, PACKAGE_VERSION);
	printf("SPDX-License-Identifier: BSD-2-Clause\n\n");
	printf("Copyright (c) 2025 The FreeBSD Foundation\n\n");
	printf("Portions of this software were developed by\n");
	printf("Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from\n");
	printf("the FreeBSD Foundation\n\n");
	printf("Report bugs at <%s>.\n", PACKAGE_BUGREPORT);
	return EXIT_SUCCESS;
}

static int
usage(void)
{
	printf("usage: spdxtool [modules]\n");

	printf("\nOptions:\n");

	printf("  --agent-name                      Set agent name [default: 'Default']\n");
	printf("  --creation-time                   Use string as creation time (Should be in ISO8601 format) [default: current time]\n");
	printf("  --creation-id                     Use string as creation id [default: '_:creationinfo_1']\n");
	printf("  --help                            this message\n");
	printf("  --about                           print bomtool version and license to stdout\n");
	printf("  --version                         print bomtool version to stdout\n");
	printf("  --output FILE                     output SBOM data to file\n");

	return EXIT_SUCCESS;
}

int
main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	pkgconf_list_t pkgq = PKGCONF_LIST_INITIALIZER;
	unsigned int want_client_flags = PKGCONF_PKG_PKGF_SEARCH_PRIVATE;
	pkgconf_cross_personality_t *personality = pkgconf_cross_personality_default();
	char *creation_time = NULL;
	char *creation_id = NULL;
	char *agent_name = NULL;
	char world_id[] = "virtual:world";
	char world_realname[] = "virtual world package";
	pkgconf_pkg_t world =
	{
		.id = world_id,
		.realname = world_realname,
		.flags = PKGCONF_PKG_PROPF_STATIC | PKGCONF_PKG_PROPF_VIRTUAL,
	};

	error_msgout = stderr;
	sbom_out = stdout;

	struct pkg_option options[] =
	{
		{ "agent-name", required_argument, NULL, 100, },
		{ "creation-time", required_argument, NULL, 101, },
		{ "creation-id", required_argument, NULL, 102, },
		{ "version", no_argument, &want_flags, PKG_VERSION, },
		{ "about", no_argument, &want_flags, PKG_ABOUT, },
		{ "help", no_argument, &want_flags, PKG_HELP, },
		{ "output", required_argument, NULL, 103, },
		{ NULL, 0, NULL, 0 }
	};

	while ((ret = pkg_getopt_long_only(argc, argv, "", options, NULL)) != -1)
	{
		switch (ret)
		{
		case 100:
			agent_name = pkg_optarg;
			break;
		case 101:
			creation_time = pkg_optarg;
			break;
		case 102:
			creation_id = pkg_optarg;
			break;
		case 103:
			sbom_out = fopen(pkg_optarg, "w");
			if (sbom_out == NULL)
			{
				fprintf(stderr, "unable to open %s: %s\n", pkg_optarg, strerror(errno));
				return EXIT_FAILURE;
			}
			break;
		case '?':
		case ':':
			return EXIT_FAILURE;
		default:
			break;
		}
	}

	pkgconf_client_init(&pkg_client, error_handler, NULL, personality, NULL, environ_lookup_handler);

	/* we have determined what features we want most likely.  in some cases, we override later. */
	pkgconf_client_set_flags(&pkg_client, want_client_flags);

	/* at this point, want_client_flags should be set, so build the dir list */
	pkgconf_client_dir_list_build(&pkg_client, personality);


	if ((want_flags & PKG_ABOUT) == PKG_ABOUT)
		return about();

	if ((want_flags & PKG_VERSION) == PKG_VERSION)
		return version();

	if ((want_flags & PKG_HELP) == PKG_HELP)
		return usage();

	while (1)
	{
		const char *package = argv[pkg_optind];

		if (package == NULL)
			break;

		/* check if there is a limit to the number of packages allowed to be included, if so and we have hit
		 * the limit, stop adding packages to the queue.
		 */
		if (maximum_package_count > 0 && pkgq.length > maximum_package_count)
			break;

		while (isspace((unsigned char)package[0]))
			package++;

		/* skip empty packages */
		if (package[0] == '\0')
		{
			pkg_optind++;
			continue;
		}

		if (argv[pkg_optind + 1] == NULL || !PKGCONF_IS_OPERATOR_CHAR(*(argv[pkg_optind + 1])))
		{
			pkgconf_queue_push(&pkgq, package);
			pkg_optind++;
		}
		else
		{
			char packagebuf[PKGCONF_BUFSIZE];

			snprintf(packagebuf, sizeof packagebuf, "%s %s %s", package, argv[pkg_optind + 1], argv[pkg_optind + 2]);
			pkg_optind += 3;

			pkgconf_queue_push(&pkgq, packagebuf);
		}
	}

	if (!pkgconf_queue_solve(&pkg_client, &pkgq, &world, maximum_traverse_depth))
	{
		ret = EXIT_FAILURE;
		goto out;
	}

	spdxtool_util_set_uri_root(&pkg_client, xsd_any_uri_default_base);
	spdxtool_util_set_spdx_license(&pkg_client, bom_license);
	spdxtool_util_set_spdx_version(&pkg_client, spdx_version);

	if (!generate_spdx(&pkg_client, &world, creation_time, creation_id, agent_name))
	{
		ret = EXIT_FAILURE;
		goto out;
	}

	ret = EXIT_SUCCESS;

out:
	pkgconf_solution_free(&pkg_client, &world);
	pkgconf_queue_free(&pkgq);
	pkgconf_cross_personality_deinit(personality);
	pkgconf_client_deinit(&pkg_client);

	return ret;
}
