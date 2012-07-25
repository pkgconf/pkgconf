/*
 * main.c
 * main() routine, printer functions
 *
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include "config.h"
#include "pkg.h"
#include "bsdstubs.h"

#define WANT_CFLAGS_ONLY_I		(19)
#define WANT_CFLAGS_ONLY_OTHER		(20)

#define WANT_LIBS_ONLY_LDPATH		(21)
#define WANT_LIBS_ONLY_LIBNAME		(22)
#define WANT_LIBS_ONLY_OTHER		(23)

static unsigned int global_traverse_flags = PKGF_NONE;

static int want_help = 0;
static int want_version = 0;
static int want_cflags = 0;
static int want_libs = 0;
static int want_modversion = 0;
static int want_static = 0;
static int want_requires = 0;
static int want_requires_private = 0;
static int want_variables = 0;
static int want_digraph = 0;
static int want_env_only = 0;
static int want_uninstalled = 0;
static int want_no_uninstalled = 0;
static int want_keep_system_cflags = 0;
static int want_keep_system_libs = 0;
static int want_ignore_conflicts = 0;
static int maximum_traverse_depth = -1;

static char *required_pkgconfig_version = NULL;
static char *required_exact_module_version = NULL;
static char *required_max_module_version = NULL;
static char *required_module_version = NULL;
static char *want_variable = NULL;
static char *sysroot_dir = NULL;

FILE *error_msgout = NULL;

static bool
fragment_has_system_dir(pkg_fragment_t *frag)
{
	switch (frag->type)
	{
	case 'L':
		if (!want_keep_system_libs && !strcasecmp(SYSTEM_LIBDIR, frag->data))
			return true;
	case 'I':
		if (!want_keep_system_cflags && !strcasecmp(SYSTEM_INCLUDEDIR, frag->data))
			return true;
	default:
		break;
	}

	return false;
}

static inline const char *
print_sysroot_dir(pkg_fragment_t *frag)
{
	if (sysroot_dir == NULL)
		return "";
	else switch (frag->type)
	{
	case 'L':
	case 'I':
		return sysroot_dir;
	default:
		break;
	}

	return "";
}

static void
print_fragment(pkg_fragment_t *frag)
{
	if (fragment_has_system_dir(frag))
		return;

	if (frag->type)
		printf("-%c%s%s ", frag->type, print_sysroot_dir(frag), frag->data);
	else
		printf("%s ", frag->data);
}

static void
print_cflags(pkg_fragment_t *list)
{
	pkg_fragment_t *frag;

	PKG_FOREACH_LIST_ENTRY(list, frag)
	{
		if (want_cflags == WANT_CFLAGS_ONLY_I && frag->type != 'I')
			continue;
		else if (want_cflags == WANT_CFLAGS_ONLY_OTHER && frag->type == 'I')
			continue;

		print_fragment(frag);
	}
}

static void
print_libs(pkg_fragment_t *list)
{
	pkg_fragment_t *frag;

	PKG_FOREACH_LIST_ENTRY(list, frag)
	{
		if (want_libs == WANT_LIBS_ONLY_LDPATH && frag->type != 'L')
			continue;
		else if (want_libs == WANT_LIBS_ONLY_LIBNAME && frag->type != 'l')
			continue;
		else if (want_libs == WANT_LIBS_ONLY_OTHER && (frag->type == 'l' || frag->type == 'L'))
			continue;

		print_fragment(frag);
	}
}

static void
print_modversion(pkg_t *pkg, void *unused, unsigned int flags)
{
	(void) unused;
	(void) flags;

	if (pkg->version != NULL)
		printf("%s\n", pkg->version);
}

static void
print_variable(pkg_t *pkg, void *unused, unsigned int flags)
{
	const char *var;
	(void) unused;
	(void) flags;

	var = pkg_tuple_find(pkg->vars, want_variable);
	if (var != NULL)
		printf("%s", var);
}

static void
print_variables(pkg_t *pkg, void *unused, unsigned int flags)
{
	pkg_tuple_t *node;
	(void) unused;
	(void) flags;

	PKG_FOREACH_LIST_ENTRY(pkg->vars, node)
		printf("%s\n", node->key);
}

static void
print_requires(pkg_t *pkg)
{
	pkg_dependency_t *node;

	PKG_FOREACH_LIST_ENTRY(pkg->requires, node)
	{
		printf("%s", node->package);

		if (node->version != NULL)
			printf(" %s %s", pkg_get_comparator(node), node->version);

		printf("\n");
	}
}

static void
print_requires_private(pkg_t *pkg)
{
	pkg_dependency_t *node;

	PKG_FOREACH_LIST_ENTRY(pkg->requires_private, node)
	{
		printf("%s", node->package);

		if (node->version != NULL)
			printf(" %s %s", pkg_get_comparator(node), node->version);

		printf("\n");
	}
}

static void
print_digraph_node(pkg_t *pkg, void *unused, unsigned int flags)
{
	pkg_dependency_t *node;
	(void) unused;
	(void) flags;

	printf("\"%s\" [fontname=Sans fontsize=8]\n", pkg->id);

	PKG_FOREACH_LIST_ENTRY(pkg->requires, node)
	{
		printf("\"%s\" -- \"%s\" [fontname=Sans fontsize=8]\n", node->package, pkg->id);
	}
}

static void
check_uninstalled(pkg_t *pkg, void *data, unsigned int flags)
{
	int *retval = data;
	(void) flags;

	if (pkg->uninstalled)
		*retval = EXIT_SUCCESS;
}

typedef struct pkg_queue_ {
	struct pkg_queue_ *prev, *next;
	char *package;
} pkg_queue_t;

static pkg_queue_t *
pkg_queue_push(pkg_queue_t *parent, const char *package)
{
	pkg_queue_t *pkgq = calloc(sizeof(pkg_queue_t), 1);

	pkgq->package = strdup(package);
	pkgq->prev = parent;
	if (pkgq->prev != NULL)
		pkgq->prev->next = pkgq;

	return pkgq;
}

int
pkg_queue_walk(pkg_queue_t *head)
{
	int retval = EXIT_SUCCESS;
	int wanted_something = 0;
	pkg_queue_t *pkgq, *next_pkgq;
	pkg_t world = (pkg_t){
		.id = "world",
		.realname = "virtual",
		.flags = PKG_PROPF_VIRTUAL,
	};

	/* if maximum_traverse_depth is one, then we will not traverse deeper
	 * than our virtual package.
	 */
	if (!maximum_traverse_depth)
		maximum_traverse_depth = -1;
	else if (maximum_traverse_depth > 0)
		maximum_traverse_depth++;

	PKG_FOREACH_LIST_ENTRY_SAFE(head, next_pkgq, pkgq)
	{
		pkg_dependency_t *pkgdep;

		pkgdep = pkg_dependency_parse(&world, pkgq->package);
		if (pkgdep != NULL)
			world.requires = pkg_dependency_append(world.requires, pkgdep);
		else
			retval = EXIT_FAILURE;

		free(pkgq->package);
		free(pkgq);
	}

	/* we couldn't build the entire depgraph, so bail. */
	if (retval != EXIT_SUCCESS)
		goto out;

	/* we should verify that the graph is complete before attempting to compute cflags etc. */
	if (pkg_verify_graph(&world, maximum_traverse_depth, global_traverse_flags) != PKG_ERRF_OK)
	{
		retval = EXIT_FAILURE;
		goto out;
	}

	if (want_uninstalled)
	{
		retval = 1;

		wanted_something = 0;
		pkg_traverse(&world, check_uninstalled, &retval, maximum_traverse_depth, global_traverse_flags);

		goto out;
	}

	if (want_digraph)
	{
		printf("graph deptree {\n");
		printf("edge [color=blue len=7.5 fontname=Sans fontsize=8]\n");
		printf("node [fontname=Sans fontsize=8]\n");

		pkg_traverse(&world, print_digraph_node, NULL, maximum_traverse_depth, global_traverse_flags);

		printf("}\n");

		goto out;
	}

	if (want_modversion)
	{
		wanted_something = 0;
		want_cflags = 0;
		want_libs = 0;

		pkg_traverse(&world, print_modversion, NULL, 2, global_traverse_flags);

		goto out;
	}

	if (want_variables)
	{
		wanted_something = 0;
		want_cflags = 0;
		want_libs = 0;

		pkg_traverse(&world, print_variables, NULL, 2, global_traverse_flags);

		goto out;
	}

	if (want_requires)
	{
		pkg_dependency_t *iter;

		wanted_something = 0;
		want_cflags = 0;
		want_libs = 0;

		PKG_FOREACH_LIST_ENTRY(world.requires, iter)
		{
			pkg_t *pkg;

			pkg = pkg_verify_dependency(iter, global_traverse_flags, NULL);
			print_requires(pkg);

			pkg_free(pkg);
		}

		goto out;
	}

	if (want_requires_private)
	{
		pkg_dependency_t *iter;

		wanted_something = 0;
		want_cflags = 0;
		want_libs = 0;

		PKG_FOREACH_LIST_ENTRY(world.requires, iter)
		{
			pkg_t *pkg;

			pkg = pkg_verify_dependency(iter, global_traverse_flags | PKGF_SEARCH_PRIVATE, NULL);
			print_requires_private(pkg);

			pkg_free(pkg);
		}

		goto out;
	}

	if (want_cflags)
	{
		pkg_fragment_t *list;

		wanted_something++;
		list = pkg_cflags(&world, maximum_traverse_depth, global_traverse_flags | PKGF_SEARCH_PRIVATE);
		print_cflags(list);

		pkg_fragment_free(list);
	}

	if (want_libs)
	{
		pkg_fragment_t *list;

		wanted_something++;
		list = pkg_libs(&world, maximum_traverse_depth, global_traverse_flags);
		print_libs(list);

		pkg_fragment_free(list);
	}

	if (want_variable)
	{
		wanted_something++;
		pkg_traverse(&world, print_variable, NULL, 2, global_traverse_flags | PKGF_SKIP_ROOT_VIRTUAL);
	}

	if (wanted_something)
		printf("\n");

out:
	pkg_free(&world);

	return retval;
}

static void
version(void)
{
	printf("%s %s%s\n", PACKAGE_NAME, PACKAGE_VERSION, HAVE_STRICT_MODE ? " [strict]" : " [pkg-config compatible]");
	printf("Copyright (c) 2011 - 2012 William Pitcock <nenolod@dereferenced.org>.\n\n");
	printf("Permission to use, copy, modify, and/or distribute this software for any\n");
	printf("purpose with or without fee is hereby granted, provided that the above\n");
	printf("copyright notice and this permission notice appear in all copies.\n\n");
	printf("This software is provided 'as is' and without any warranty, express or\n");
	printf("implied.  In no event shall the authors be liable for any damages arising\n");
	printf("from the use of this software.\n\n");
	printf("Report bugs at <%s>.\n", PACKAGE_BUGREPORT);
}

static void
usage(void)
{
	printf("usage: %s [OPTIONS] [LIBRARIES]\n", PACKAGE_NAME);

	printf("\nbasic options:\n\n");

	printf("  --help                            this message\n");
	printf("  --version                         print pkgconf version to stdout\n");
	printf("  --atleast-pkgconfig-version       check whether or not pkgconf is compatible\n");
	printf("                                    with a specified pkg-config version\n");
	printf("  --errors-to-stdout                print all errors on stdout instead of stderr\n");
	printf("  --silence-errors                  explicitly be silent about errors\n");

	printf("\nchecking specific pkg-config database entries:\n\n");

	printf("  --atleast-version                 require a specific version of a module\n");
	printf("  --exact-version                   require an exact version of a module\n");
	printf("  --max-version                     require a maximum version of a module\n");
	printf("  --exists                          check whether or not a module exists\n");
	printf("  --uninstalled                     check whether or not an uninstalled module will be used\n");
	printf("  --no-uninstalled                  never use uninstalled modules when satisfying dependencies\n");
	printf("  --maximum-traverse-depth          maximum allowed depth for dependency graph\n");
	printf("  --static                          be more aggressive when computing dependency graph\n");
	printf("                                    (for static linking)\n");
	printf("  --env-only                        look only for package entries in PKG_CONFIG_PATH\n");
	printf("  --ignore-conflicts                ignore 'conflicts' rules in modules\n");

	printf("\nquerying specific pkg-config database fields:\n\n");

	printf("  --define-variable=varname=value   define variable 'varname' as 'value'\n");
	printf("  --variable=varname                print specified variable entry to stdout\n");
	printf("  --cflags                          print required CFLAGS to stdout\n");
	printf("  --cflags-only-I                   print required include-dir CFLAGS to stdout\n");
	printf("  --cflags-only-other               print required non-include-dir CFLAGS to stdout\n");
	printf("  --libs                            print required linker flags to stdout\n");
	printf("  --libs-only-L                     print required LDPATH linker flags to stdout\n");
	printf("  --libs-only-l                     print required LIBNAME linker flags to stdout\n");
	printf("  --libs-only-other                 print required other linker flags to stdout\n");
	printf("  --print-requires                  print required dependency frameworks to stdout\n");
	printf("  --print-requires-private          print required dependency frameworks for static\n");
	printf("                                    linking to stdout\n");
	printf("  --print-variables                 print all known variables in module to stdout\n");
	printf("  --digraph                         print entire dependency graph in graphviz 'dot' format\n");
	printf("  --keep-system-cflags              keep -I%s entries in cflags output\n", SYSTEM_INCLUDEDIR);
	printf("  --keep-system-libs                keep -L%s entries in libs output\n", SYSTEM_LIBDIR);

	printf("\nreport bugs to <%s>.\n", PACKAGE_BUGREPORT);
}

int
main(int argc, char *argv[])
{
	int ret;
	pkg_queue_t *pkgq = NULL;
	pkg_queue_t *pkgq_head = NULL;
	char *builddir;
	int want_errors_on_stdout = 0;
	int want_silence_errors = 0;
	int want_libs_L = 0;
	int want_libs_l = 0;
	int want_libs_other = 0;

	struct pkg_option options[] = {
		{ "version", no_argument, &want_version, 1, },
		{ "atleast-version", required_argument, NULL, 2, },
		{ "atleast-pkgconfig-version", required_argument, NULL, 3, },
		{ "libs", no_argument, &want_libs, 4, },
		{ "cflags", no_argument, &want_cflags, 5, },
		{ "modversion", no_argument, &want_modversion, 6, },
		{ "variable", required_argument, NULL, 7, },
		{ "exists", no_argument, NULL, 8, },
		{ "print-errors", no_argument, NULL, 9, },
		{ "short-errors", no_argument, NULL, 10, },
		{ "maximum-traverse-depth", required_argument, NULL, 11, },
		{ "static", no_argument, &want_static, 12, },
		{ "print-requires", no_argument, &want_requires, 13, },
		{ "print-variables", no_argument, &want_variables, 14, },
		{ "digraph", no_argument, &want_digraph, 15, },
		{ "help", no_argument, &want_help, 16, },
		{ "env-only", no_argument, &want_env_only, 17, },
		{ "print-requires-private", no_argument, &want_requires_private, 18, },
		{ "cflags-only-I", no_argument, &want_cflags, WANT_CFLAGS_ONLY_I, },
		{ "cflags-only-other", no_argument, &want_cflags, WANT_CFLAGS_ONLY_OTHER, },
		{ "libs-only-L", no_argument, &want_libs_L, WANT_LIBS_ONLY_LDPATH, },
		{ "libs-only-l", no_argument, &want_libs_l, WANT_LIBS_ONLY_LIBNAME, },
		{ "libs-only-other", no_argument, &want_libs_other, WANT_LIBS_ONLY_OTHER, },
		{ "uninstalled", no_argument, &want_uninstalled, 24, },
		{ "no-uninstalled", no_argument, &want_no_uninstalled, 25, },
		{ "keep-system-cflags", no_argument, &want_keep_system_cflags, 26, },
		{ "keep-system-libs", no_argument, &want_keep_system_libs, 26, },
		{ "define-variable", required_argument, NULL, 27, },
		{ "exact-version", required_argument, NULL, 28, },
		{ "max-version", required_argument, NULL, 29, },
		{ "ignore-conflicts", no_argument, &want_ignore_conflicts, 30, },
		{ "errors-to-stdout", no_argument, &want_errors_on_stdout, 31, },
		{ "silence-errors", no_argument, &want_silence_errors, 32, },
		{ NULL, 0, NULL, 0 }
	};

	while ((ret = pkg_getopt_long_only(argc, argv, "", options, NULL)) != -1)
	{
		switch (ret)
		{
		case 2:
			required_module_version = pkg_optarg;
			break;
		case 3:
			required_pkgconfig_version = pkg_optarg;
			break;
		case 7:
			want_variable = pkg_optarg;
			break;
		case 11:
			maximum_traverse_depth = atoi(pkg_optarg);
			break;
		case 27:
			pkg_tuple_define_global(pkg_optarg);
			break;
		case 28:
			required_exact_module_version = pkg_optarg;
			break;
		case 29:
			required_max_module_version = pkg_optarg;
			break;
		case '?':
		case ':':
			return EXIT_FAILURE;
			break;
		default:
			break;
		}
	}

	if (want_libs_l)
		want_libs = want_libs_l;
	else if (want_libs_L)
		want_libs = want_libs_L;
	else if (want_libs_other)
		want_libs = want_libs_other;

	if (want_version)
	{
		version();
		return EXIT_SUCCESS;
	}

	if (want_help)
	{
		usage();
		return EXIT_SUCCESS;
	}

	error_msgout = stderr;
	if (want_errors_on_stdout)
		error_msgout = stdout;
	if (want_silence_errors)
		error_msgout = fopen(PATH_DEV_NULL, "w");

	if (want_ignore_conflicts || getenv("PKG_CONFIG_IGNORE_CONFLICTS") != NULL)
		global_traverse_flags |= PKGF_SKIP_CONFLICTS;

	if (want_static)
		global_traverse_flags |= (PKGF_SEARCH_PRIVATE | PKGF_MERGE_PRIVATE_FRAGMENTS);

	if (want_env_only)
		global_traverse_flags |= PKGF_ENV_ONLY;

	if (want_no_uninstalled || getenv("PKG_CONFIG_DISABLE_UNINSTALLED") != NULL)
		global_traverse_flags |= PKGF_NO_UNINSTALLED;

	if (getenv("PKG_CONFIG_ALLOW_SYSTEM_CFLAGS") != NULL)
		want_keep_system_cflags = 1;

	if (getenv("PKG_CONFIG_ALLOW_SYSTEM_LIBS") != NULL)
		want_keep_system_libs = 1;

	if ((builddir = getenv("PKG_CONFIG_TOP_BUILD_DIR")) != NULL)
		pkg_tuple_add_global("pc_top_builddir", builddir);
	else
		pkg_tuple_add_global("pc_top_builddir", "$(top_builddir)");

	if ((sysroot_dir = getenv("PKG_CONFIG_SYSROOT_DIR")) != NULL)
		pkg_tuple_add_global("pc_sysrootdir", sysroot_dir);
	else
		pkg_tuple_add_global("pc_sysrootdir", "/");

	if (required_pkgconfig_version != NULL)
	{
		if (pkg_compare_version(PKG_PKGCONFIG_VERSION_EQUIV, required_pkgconfig_version) >= 0)
			return EXIT_SUCCESS;

		return EXIT_FAILURE;
	}

	if (required_module_version != NULL)
	{
		pkg_t *pkg;
		pkg_dependency_t *pkghead = NULL, *pkgiter = NULL;

		while (argv[pkg_optind])
		{
			pkghead = pkg_dependency_parse_str(pkghead, argv[pkg_optind]);
			pkg_optind++;
		}

		PKG_FOREACH_LIST_ENTRY(pkghead, pkgiter)
		{
			pkg = pkg_find(pkgiter->package, global_traverse_flags);
			if (pkg == NULL)
				return EXIT_FAILURE;

			if (pkg_compare_version(pkg->version, required_module_version) >= 0)
				return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	if (required_exact_module_version != NULL)
	{
		pkg_t *pkg;
		pkg_dependency_t *pkghead = NULL, *pkgiter = NULL;

		while (argv[pkg_optind])
		{
			pkghead = pkg_dependency_parse_str(pkghead, argv[pkg_optind]);
			pkg_optind++;
		}

		PKG_FOREACH_LIST_ENTRY(pkghead, pkgiter)
		{
			pkg = pkg_find(pkgiter->package, global_traverse_flags);
			if (pkg == NULL)
				return EXIT_FAILURE;

			if (pkg_compare_version(pkg->version, required_exact_module_version) == 0)
				return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	if (required_max_module_version != NULL)
	{
		pkg_t *pkg;
		pkg_dependency_t *pkghead = NULL, *pkgiter = NULL;

		while (argv[pkg_optind])
		{
			pkghead = pkg_dependency_parse_str(pkghead, argv[pkg_optind]);
			pkg_optind++;
		}

		PKG_FOREACH_LIST_ENTRY(pkghead, pkgiter)
		{
			pkg = pkg_find(pkgiter->package, global_traverse_flags);
			if (pkg == NULL)
				return EXIT_FAILURE;

			if (pkg_compare_version(pkg->version, required_max_module_version) <= 0)
				return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	while (1)
	{
		const char *package = argv[pkg_optind];

		if (package == NULL)
			break;

		while (isspace(package[0]))
			package++;

		/* skip empty packages */
		if (package[0] == '\0') {
			pkg_optind++;
			continue;
		}

		if (argv[pkg_optind + 1] == NULL || !PKG_OPERATOR_CHAR(*(argv[pkg_optind + 1])))
		{
			pkgq = pkg_queue_push(pkgq, package);

			if (pkgq_head == NULL)
				pkgq_head = pkgq;

			pkg_optind++;
		}
		else
		{
			char packagebuf[PKG_BUFSIZE];

			snprintf(packagebuf, sizeof packagebuf, "%s %s %s", package, argv[pkg_optind + 1], argv[pkg_optind + 2]);
			pkg_optind += 3;

			pkgq = pkg_queue_push(pkgq, packagebuf);

			if (pkgq_head == NULL)
				pkgq_head = pkgq;
		}
	}

	if (pkgq_head == NULL)
	{
		fprintf(error_msgout, "Please specify at least one package name on the command line.\n");
		return EXIT_FAILURE;
	}

	ret = pkg_queue_walk(pkgq_head);

	pkg_tuple_free_global();
	return ret;
}
