/*
 * bomtool/main.c
 * main() routine, printer functions
 *
 * Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
 *     pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include "libpkgconf/config.h"
#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>
#include "getopt_long.h"

#define PKG_VERSION			(((uint64_t) 1) << 1)
#define PKG_ABOUT			(((uint64_t) 1) << 2)
#define PKG_HELP			(((uint64_t) 1) << 3)

static pkgconf_client_t pkg_client;
static uint64_t want_flags;
static size_t maximum_package_count = 0;
static int maximum_traverse_depth = 2000;
FILE *error_msgout = NULL;

static bool
error_handler(const char *msg, const pkgconf_client_t *client, void *data)
{
	(void) client;
	(void) data;
	fprintf(error_msgout, "%s", msg);
	return true;
}

static bool
generate_sbom_from_world(pkgconf_client_t *client, pkgconf_pkg_t *world)
{
	return true;
}

static int
version(void)
{
	printf("bomtool %s\n", PACKAGE_VERSION);
	return EXIT_SUCCESS;
}

static int
about(void)
{
	printf("bomtool (%s %s)\n", PACKAGE_NAME, PACKAGE_VERSION);
	printf("Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021\n");
	printf("    pkgconf authors (see AUTHORS in documentation directory).\n\n");
	printf("Permission to use, copy, modify, and/or distribute this software for any\n");
	printf("purpose with or without fee is hereby granted, provided that the above\n");
	printf("copyright notice and this permission notice appear in all copies.\n\n");
	printf("This software is provided 'as is' and without any warranty, express or\n");
	printf("implied.  In no event shall the authors be liable for any damages arising\n");
	printf("from the use of this software.\n\n");
	printf("Report bugs at <%s>.\n", PACKAGE_BUGREPORT);
	return EXIT_SUCCESS;
}

static int
usage(void)
{
	printf("usage: bomtool [--flags] [modules]\n");

	printf("\nbasic options:\n\n");

	printf("  --help                            this message\n");
	printf("  --about                           print bomtool version and license to stdout\n");
	printf("  --version                         print bomtool version to stdout\n");

	return EXIT_SUCCESS;
}

int
main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	pkgconf_list_t pkgq = PKGCONF_LIST_INITIALIZER;
	pkgconf_list_t dir_list = PKGCONF_LIST_INITIALIZER;
	unsigned int want_client_flags = PKGCONF_PKG_PKGF_NONE;
	pkgconf_cross_personality_t *personality = pkgconf_cross_personality_default();
	pkgconf_pkg_t world = {
		.id = "virtual:world",
		.realname = "virtual world package",
		.flags = PKGCONF_PKG_PROPF_STATIC | PKGCONF_PKG_PROPF_VIRTUAL,
	};

	error_msgout = stderr;

	struct pkg_option options[] = {
		{ "version", no_argument, &want_flags, PKG_VERSION, },
		{ "about", no_argument, &want_flags, PKG_ABOUT, },
		{ "help", no_argument, &want_flags, PKG_HELP, },
		{ NULL, 0, NULL, 0 }
	};

	while ((ret = pkg_getopt_long_only(argc, argv, "", options, NULL)) != -1)
	{
		switch (ret)
		{
		case '?':
		case ':':
			return EXIT_FAILURE;
		default:
			break;
		}
	}

	pkgconf_client_init(&pkg_client, error_handler, NULL, personality);

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

		while (isspace((unsigned int)package[0]))
			package++;

		/* skip empty packages */
		if (package[0] == '\0') {
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

	if (pkgq.head == NULL)
	{
		fprintf(stderr, "Please specify at least one package name on the command line.\n");
		ret = EXIT_FAILURE;
	}

	ret = EXIT_SUCCESS;

	if (!pkgconf_queue_solve(&pkg_client, &pkgq, &world, maximum_traverse_depth))
	{
		ret = EXIT_FAILURE;
		goto out;
	}

	if (!generate_sbom_from_world(&pkg_client, &world))
	{
		ret = EXIT_FAILURE;
		goto out;
	}

out:
	pkgconf_queue_free(&pkgq);
	pkgconf_cross_personality_deinit(personality);
	pkgconf_client_deinit(&pkg_client);

	return ret;
}
