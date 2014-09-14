/*
 * main.c
 * main() routine, printer functions
 *
 * Copyright (c) 2011, 2012, 2013 pkgconf authors (see AUTHORS).
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

#define PKG_CFLAGS_ONLY_I		(1<<2)
#define PKG_CFLAGS_ONLY_OTHER		(1<<3)
#define PKG_CFLAGS			(PKG_CFLAGS_ONLY_I|PKG_CFLAGS_ONLY_OTHER)
#define PKG_LIBS_ONLY_LDPATH		(1<<5)
#define PKG_LIBS_ONLY_LIBNAME		(1<<6)
#define PKG_LIBS_ONLY_OTHER		(1<<7)
#define PKG_LIBS			(PKG_LIBS_ONLY_LDPATH|PKG_LIBS_ONLY_LIBNAME|PKG_LIBS_ONLY_OTHER)
#define PKG_MODVERSION			(1<<8)
#define PKG_REQUIRES			(1<<9)
#define PKG_REQUIRES_PRIVATE		(1<<10)
#define PKG_VARIABLES			(1<<11)
#define PKG_DIGRAPH			(1<<12)
#define PKG_KEEP_SYSTEM_CFLAGS		(1<<13)
#define PKG_KEEP_SYSTEM_LIBS		(1<<14)
#define PKG_VERSION			(1<<15)
#define PKG_ABOUT			(1<<16)
#define PKG_ENV_ONLY			(1<<17)
#define PKG_ERRORS_ON_STDOUT		(1<<18)
#define PKG_SILENCE_ERRORS		(1<<19)
#define PKG_IGNORE_CONFLICTS		(1<<20)
#define PKG_STATIC			(1<<21)
#define PKG_NO_UNINSTALLED		(1<<22)
#define PKG_UNINSTALLED			(1<<23)
#define PKG_LIST			(1<<24)
#define PKG_HELP			(1<<25)
#define PKG_PRINT_ERRORS		(1<<26)
#define PKG_SIMULATE			(1<<27)
#define PKG_NO_CACHE			(1<<28)

static unsigned int global_traverse_flags = PKGF_NONE;

static int want_flags;
static int maximum_traverse_depth = 2000;

static char *want_variable = NULL;
static char *sysroot_dir = NULL;

FILE *error_msgout = NULL;

static bool
fragment_has_system_dir(pkg_fragment_t *frag)
{
	int check_flags = 0;
	char *check_paths = NULL;
	char *save, *chunk;
	bool ret = false;

	switch (frag->type)
	{
	case 'L':
		check_flags = PKG_KEEP_SYSTEM_LIBS;
		check_paths = strdup(SYSTEM_LIBDIR);
		break;
	case 'I':
		check_flags = PKG_KEEP_SYSTEM_CFLAGS;
		check_paths = strdup(SYSTEM_INCLUDEDIR);
		break;
	default:
		return false;
	}

	for (chunk = strtok_r(check_paths, ":", &save); chunk != NULL; chunk = strtok_r(NULL, ":", &save))
	{
		if ((want_flags & check_flags) == 0 && !strcmp(chunk, frag->data))
		{
			ret = true;
			break;
		}
	}

	free(check_paths);

	return ret;
}

static void
print_fragment(pkg_fragment_t *frag)
{
	if (fragment_has_system_dir(frag))
		return;

	if (frag->type)
		printf("-%c%s ", frag->type, frag->data);
	else
		printf("%s ", frag->data);
}

static void
print_list_entry(const pkg_t *entry)
{
	if (entry->flags & PKG_PROPF_UNINSTALLED)
		return;

	printf("%-30s %s - %s\n", entry->id, entry->realname, entry->description);
}

static void
print_cflags(pkg_list_t *list)
{
	pkg_node_t *node;

	PKG_FOREACH_LIST_ENTRY(list->head, node)
	{
		pkg_fragment_t *frag = node->data;
		int got_flags = 0;

		if (frag->type == 'I')
			got_flags = PKG_CFLAGS_ONLY_I;
		else
			got_flags = PKG_CFLAGS_ONLY_OTHER;

		if ((want_flags & got_flags) == 0)
			continue;

		print_fragment(frag);
	}
}

static void
print_libs(pkg_list_t *list)
{
	pkg_node_t *node;

	PKG_FOREACH_LIST_ENTRY(list->head, node)
	{
		pkg_fragment_t *frag = node->data;
		int got_flags = 0;

		switch (frag->type)
		{
			case 'L': got_flags = PKG_LIBS_ONLY_LDPATH; break;
			case 'l': got_flags = PKG_LIBS_ONLY_LIBNAME; break;
			default: got_flags = PKG_LIBS_ONLY_OTHER; break;
		}

		if ((want_flags & got_flags) == 0)
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
print_variables(pkg_t *pkg, void *unused, unsigned int flags)
{
	pkg_node_t *node;
	(void) unused;
	(void) flags;

	PKG_FOREACH_LIST_ENTRY(pkg->vars.head, node)
	{
		pkg_tuple_t *tuple = node->data;

		printf("%s\n", tuple->key);
	}
}

static void
print_requires(pkg_t *pkg)
{
	pkg_node_t *node;

	PKG_FOREACH_LIST_ENTRY(pkg->requires.head, node)
	{
		pkg_dependency_t *dep = node->data;

		printf("%s", dep->package);

		if (dep->version != NULL)
			printf(" %s %s", pkg_get_comparator(dep), dep->version);

		printf("\n");
	}
}

static void
print_requires_private(pkg_t *pkg)
{
	pkg_node_t *node;

	PKG_FOREACH_LIST_ENTRY(pkg->requires_private.head, node)
	{
		pkg_dependency_t *dep = node->data;

		printf("%s", dep->package);

		if (dep->version != NULL)
			printf(" %s %s", pkg_get_comparator(dep), dep->version);

		printf("\n");
	}
}

static void
print_digraph_node(pkg_t *pkg, void *unused, unsigned int flags)
{
	pkg_node_t *node;
	(void) unused;
	(void) flags;

	printf("\"%s\" [fontname=Sans fontsize=8]\n", pkg->id);

	PKG_FOREACH_LIST_ENTRY(pkg->requires.head, node)
	{
		pkg_dependency_t *dep = node->data;

		printf("\"%s\" -- \"%s\" [fontname=Sans fontsize=8]\n", dep->package, pkg->id);
	}
}

static bool
apply_digraph(pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	int eflag;

	printf("graph deptree {\n");
	printf("edge [color=blue len=7.5 fontname=Sans fontsize=8]\n");
	printf("node [fontname=Sans fontsize=8]\n");

	eflag = pkg_traverse(world, print_digraph_node, unused, maxdepth, flags);

	if (eflag != PKG_ERRF_OK)
		return false;

	printf("}\n");
	return true;
}

static bool
apply_modversion(pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkg_traverse(world, print_modversion, unused, maxdepth, flags);

	if (eflag != PKG_ERRF_OK)
		return false;

	return true;
}

static bool
apply_variables(pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkg_traverse(world, print_variables, unused, maxdepth, flags);

	if (eflag != PKG_ERRF_OK)
		return false;

	return true;
}

typedef struct {
	const char *variable;
	char buf[PKG_BUFSIZE];
} var_request_t;

static void
print_variable(pkg_t *pkg, void *data, unsigned int flags)
{
	var_request_t *req = data;
	const char *var;
	(void) flags;

	var = pkg_tuple_find(&pkg->vars, req->variable);
	if (var != NULL)
	{
		if (*(req->buf) == '\0')
		{
			memset(req->buf, 0, sizeof(req->buf));

			if (*var == '/' && (flags & PKGF_MUNGE_SYSROOT_PREFIX) &&
			    (sysroot_dir != NULL && strncmp(var, sysroot_dir, strlen(sysroot_dir))))
				strlcat(req->buf, sysroot_dir, sizeof(req->buf));

			strlcat(req->buf, var, sizeof(req->buf));
			return;
		}

		strlcat(req->buf, " ", sizeof(req->buf));

		if (*var == '/' && (flags & PKGF_MUNGE_SYSROOT_PREFIX) &&
		    (sysroot_dir != NULL && strncmp(var, sysroot_dir, strlen(sysroot_dir))))
			strlcat(req->buf, sysroot_dir, sizeof(req->buf));

		strlcat(req->buf, var, sizeof(req->buf));
	}
}

static bool
apply_variable(pkg_t *world, void *variable, int maxdepth, unsigned int flags)
{
	int eflag;

	var_request_t req = {
		.variable = variable,
	};

	*req.buf = '\0';

	eflag = pkg_traverse(world, print_variable, &req, maxdepth, flags);
	if (eflag != PKG_ERRF_OK)
		return false;

	printf("%s\n", req.buf);
	return true;
}

static bool
apply_cflags(pkg_t *world, void *list_head, int maxdepth, unsigned int flags)
{
	pkg_list_t *list = list_head;
	int eflag;

	eflag = pkg_cflags(world, list, maxdepth, flags | PKGF_SEARCH_PRIVATE);
	if (eflag != PKG_ERRF_OK)
		return false;

	if (list->head == NULL)
		return true;

	print_cflags(list);

	pkg_fragment_free(list);

	return true;
}

static bool
apply_libs(pkg_t *world, void *list_head, int maxdepth, unsigned int flags)
{
	pkg_list_t *list = list_head;
	int eflag;

	eflag = pkg_libs(world, list, maxdepth, flags);
	if (eflag != PKG_ERRF_OK)
		return false;

	if (list->head == NULL)
		return true;

	print_libs(list);

	pkg_fragment_free(list);

	return true;
}

static bool
apply_requires(pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	pkg_node_t *iter;
	(void) unused;
	(void) maxdepth;

	PKG_FOREACH_LIST_ENTRY(world->requires.head, iter)
	{
		pkg_t *pkg;
		pkg_dependency_t *dep = iter->data;

		pkg = pkg_verify_dependency(dep, flags, NULL);
		print_requires(pkg);

		pkg_free(pkg);
	}

	return true;
}

static bool
apply_requires_private(pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	pkg_node_t *iter;
	(void) unused;
	(void) maxdepth;

	PKG_FOREACH_LIST_ENTRY(world->requires.head, iter)
	{
		pkg_t *pkg;
		pkg_dependency_t *dep = iter->data;

		pkg = pkg_verify_dependency(dep, flags | PKGF_SEARCH_PRIVATE, NULL);
		print_requires_private(pkg);

		pkg_free(pkg);
	}
	return true;
}

static void
check_uninstalled(pkg_t *pkg, void *data, unsigned int flags)
{
	int *retval = data;
	(void) flags;

	if (pkg->flags & PKG_PROPF_UNINSTALLED)
		*retval = EXIT_SUCCESS;
}

static bool
apply_uninstalled(pkg_t *world, void *data, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkg_traverse(world, check_uninstalled, data, maxdepth, flags);

	if (eflag != PKG_ERRF_OK)
		return false;

	return true;
}

static void
print_graph_node(pkg_t *pkg, void *data, unsigned int flags)
{
	pkg_node_t *n;

	(void) data;
	(void) flags;

	printf("node '%s' {\n", pkg->id);

	if (pkg->version != NULL)
		printf("    version = '%s';\n", pkg->version);

	PKG_FOREACH_LIST_ENTRY(pkg->requires.head, n)
	{
		pkg_dependency_t *dep = n->data;
		printf("    dependency '%s'", dep->package);
		if (dep->compare != PKG_ANY)
		{
			printf(" {\n");
			printf("        comparator = '%s';\n", pkg_get_comparator(dep));
			printf("        version = '%s';\n", dep->version);
			printf("    };\n");
		}
		else
			printf(";\n");
	}

	printf("};\n");
}

static bool
apply_simulate(pkg_t *world, void *data, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkg_traverse(world, print_graph_node, data, maxdepth, flags);

	if (eflag != PKG_ERRF_OK)
		return false;

	return true;
}

static void
version(void)
{
	printf("%s\n", PKG_PKGCONFIG_VERSION_EQUIV);
}

static void
about(void)
{
	printf("%s %s%s\n", PACKAGE_NAME, PACKAGE_VERSION, HAVE_STRICT_MODE ? " [strict]" : " [pkg-config " PKG_PKGCONFIG_VERSION_EQUIV " compatible]");
	printf("Copyright (c) 2011, 2012, 2013 pkgconf authors (see AUTHORS in documentation directory).\n\n");
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
	printf("  --about                           print pkgconf version and license to stdout\n");
	printf("  --version                         print supported pkg-config version to stdout\n");
	printf("  --atleast-pkgconfig-version       check whether or not pkgconf is compatible\n");
	printf("                                    with a specified pkg-config version\n");
	printf("  --errors-to-stdout                print all errors on stdout instead of stderr\n");
	printf("  --silence-errors                  explicitly be silent about errors\n");
	printf("  --list-all                        list all known packages\n");
	printf("  --simulate                        simulate walking the calculated dependency graph\n");
	printf("  --no-cache                        do not cache already seen packages when\n");
	printf("                                    walking the dependency graph\n");

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
	pkg_list_t pkgq = PKG_LIST_INITIALIZER;
	char *builddir;
	char *required_pkgconfig_version = NULL;
	char *required_exact_module_version = NULL;
	char *required_max_module_version = NULL;
	char *required_module_version = NULL;

	want_flags = 0;

	struct pkg_option options[] = {
		{ "version", no_argument, &want_flags, PKG_VERSION|PKG_PRINT_ERRORS, },
		{ "about", no_argument, &want_flags, PKG_ABOUT|PKG_PRINT_ERRORS, },
		{ "atleast-version", required_argument, NULL, 2, },
		{ "atleast-pkgconfig-version", required_argument, NULL, 3, },
		{ "libs", no_argument, &want_flags, PKG_LIBS|PKG_PRINT_ERRORS, },
		{ "cflags", no_argument, &want_flags, PKG_CFLAGS|PKG_PRINT_ERRORS, },
		{ "modversion", no_argument, &want_flags, PKG_MODVERSION|PKG_PRINT_ERRORS, },
		{ "variable", required_argument, NULL, 7, },
		{ "exists", no_argument, NULL, 8, },
		{ "print-errors", no_argument, &want_flags, PKG_PRINT_ERRORS, },
		{ "short-errors", no_argument, NULL, 10, },
		{ "maximum-traverse-depth", required_argument, NULL, 11, },
		{ "static", no_argument, &want_flags, PKG_STATIC, },
		{ "print-requires", no_argument, &want_flags, PKG_REQUIRES, },
		{ "print-variables", no_argument, &want_flags, PKG_VARIABLES|PKG_PRINT_ERRORS, },
		{ "digraph", no_argument, &want_flags, PKG_DIGRAPH, },
		{ "help", no_argument, &want_flags, PKG_HELP, },
		{ "env-only", no_argument, &want_flags, PKG_ENV_ONLY, },
		{ "print-requires-private", no_argument, &want_flags, PKG_REQUIRES_PRIVATE, },
		{ "cflags-only-I", no_argument, &want_flags, PKG_CFLAGS_ONLY_I|PKG_PRINT_ERRORS, },
		{ "cflags-only-other", no_argument, &want_flags, PKG_CFLAGS_ONLY_OTHER|PKG_PRINT_ERRORS, },
		{ "libs-only-L", no_argument, &want_flags, PKG_LIBS_ONLY_LDPATH|PKG_PRINT_ERRORS, },
		{ "libs-only-l", no_argument, &want_flags, PKG_LIBS_ONLY_LIBNAME|PKG_PRINT_ERRORS, },
		{ "libs-only-other", no_argument, &want_flags, PKG_LIBS_ONLY_OTHER|PKG_PRINT_ERRORS, },
		{ "uninstalled", no_argument, &want_flags, PKG_UNINSTALLED, },
		{ "no-uninstalled", no_argument, &want_flags, PKG_NO_UNINSTALLED, },
		{ "keep-system-cflags", no_argument, &want_flags, PKG_KEEP_SYSTEM_CFLAGS, },
		{ "keep-system-libs", no_argument, &want_flags, PKG_KEEP_SYSTEM_LIBS, },
		{ "define-variable", required_argument, NULL, 27, },
		{ "exact-version", required_argument, NULL, 28, },
		{ "max-version", required_argument, NULL, 29, },
		{ "ignore-conflicts", no_argument, &want_flags, PKG_IGNORE_CONFLICTS, },
		{ "errors-to-stdout", no_argument, &want_flags, PKG_ERRORS_ON_STDOUT, },
		{ "silence-errors", no_argument, &want_flags, PKG_SILENCE_ERRORS, },
		{ "list-all", no_argument, &want_flags, PKG_LIST|PKG_PRINT_ERRORS, },
		{ "simulate", no_argument, &want_flags, PKG_SIMULATE, },
		{ "no-cache", no_argument, &want_flags, PKG_NO_CACHE, },
		{ "debug", no_argument, &want_flags, 0, },
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

	if ((want_flags & PKG_PRINT_ERRORS) != PKG_PRINT_ERRORS)
		want_flags |= (PKG_SILENCE_ERRORS);

	if ((want_flags & PKG_SILENCE_ERRORS) == PKG_SILENCE_ERRORS && !getenv("PKG_CONFIG_DEBUG_SPEW"))
		want_flags |= (PKG_SILENCE_ERRORS);
	else
		want_flags &= ~(PKG_SILENCE_ERRORS);

	if ((want_flags & PKG_ABOUT) == PKG_ABOUT)
	{
		about();
		return EXIT_SUCCESS;
	}

	if ((want_flags & PKG_VERSION) == PKG_VERSION)
	{
		version();
		return EXIT_SUCCESS;
	}

	if ((want_flags & PKG_HELP) == PKG_HELP)
	{
		usage();
		return EXIT_SUCCESS;
	}

	error_msgout = stderr;
	if ((want_flags & PKG_ERRORS_ON_STDOUT) == PKG_ERRORS_ON_STDOUT)
		error_msgout = stdout;
	if ((want_flags & PKG_SILENCE_ERRORS) == PKG_SILENCE_ERRORS)
		error_msgout = fopen(PATH_DEV_NULL, "w");

	if ((want_flags & PKG_IGNORE_CONFLICTS) == PKG_IGNORE_CONFLICTS || getenv("PKG_CONFIG_IGNORE_CONFLICTS") != NULL)
		global_traverse_flags |= PKGF_SKIP_CONFLICTS;

	if ((want_flags & PKG_STATIC) == PKG_STATIC)
		global_traverse_flags |= (PKGF_SEARCH_PRIVATE | PKGF_MERGE_PRIVATE_FRAGMENTS);

	if ((want_flags & PKG_ENV_ONLY) == PKG_ENV_ONLY)
		global_traverse_flags |= PKGF_ENV_ONLY;

	if ((want_flags & PKG_NO_CACHE) == PKG_NO_CACHE)
		global_traverse_flags |= PKGF_NO_CACHE;

	if ((want_flags & PKG_NO_UNINSTALLED) == PKG_NO_UNINSTALLED || getenv("PKG_CONFIG_DISABLE_UNINSTALLED") != NULL)
		global_traverse_flags |= PKGF_NO_UNINSTALLED;

	if (getenv("PKG_CONFIG_ALLOW_SYSTEM_CFLAGS") != NULL)
		want_flags |= PKG_KEEP_SYSTEM_CFLAGS;

	if (getenv("PKG_CONFIG_ALLOW_SYSTEM_LIBS") != NULL)
		want_flags |= PKG_KEEP_SYSTEM_LIBS;

	if ((builddir = getenv("PKG_CONFIG_TOP_BUILD_DIR")) != NULL)
		pkg_tuple_add_global("pc_top_builddir", builddir);
	else
		pkg_tuple_add_global("pc_top_builddir", "$(top_builddir)");

	if ((sysroot_dir = getenv("PKG_CONFIG_SYSROOT_DIR")) != NULL)
	{
		pkg_tuple_add_global("pc_sysrootdir", sysroot_dir);
		global_traverse_flags |= PKGF_MUNGE_SYSROOT_PREFIX;
	}
	else
		pkg_tuple_add_global("pc_sysrootdir", "/");

	if (required_pkgconfig_version != NULL)
	{
		if (pkg_compare_version(PKG_PKGCONFIG_VERSION_EQUIV, required_pkgconfig_version) >= 0)
			return EXIT_SUCCESS;

		return EXIT_FAILURE;
	}

	if ((want_flags & PKG_LIST) == PKG_LIST)
	{
		pkg_scan_all(print_list_entry);
		return EXIT_SUCCESS;
	}

	if (required_module_version != NULL)
	{
		pkg_t *pkg;
		pkg_node_t *node;
		pkg_list_t deplist = PKG_LIST_INITIALIZER;

		while (argv[pkg_optind])
		{
			pkg_dependency_parse_str(&deplist, argv[pkg_optind]);
			pkg_optind++;
		}

		PKG_FOREACH_LIST_ENTRY(deplist.head, node)
		{
			pkg_dependency_t *pkgiter = node->data;

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
		pkg_node_t *node;
		pkg_list_t deplist = PKG_LIST_INITIALIZER;

		while (argv[pkg_optind])
		{
			pkg_dependency_parse_str(&deplist, argv[pkg_optind]);
			pkg_optind++;
		}

		PKG_FOREACH_LIST_ENTRY(deplist.head, node)
		{
			pkg_dependency_t *pkgiter = node->data;

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
		pkg_node_t *node;
		pkg_list_t deplist = PKG_LIST_INITIALIZER;

		while (argv[pkg_optind])
		{
			pkg_dependency_parse_str(&deplist, argv[pkg_optind]);
			pkg_optind++;
		}

		PKG_FOREACH_LIST_ENTRY(deplist.head, node)
		{
			pkg_dependency_t *pkgiter = node->data;

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
			pkg_queue_push(&pkgq, package);
			pkg_optind++;
		}
		else
		{
			char packagebuf[PKG_BUFSIZE];

			snprintf(packagebuf, sizeof packagebuf, "%s %s %s", package, argv[pkg_optind + 1], argv[pkg_optind + 2]);
			pkg_optind += 3;

			pkg_queue_push(&pkgq, packagebuf);
		}
	}

	if (pkgq.head == NULL)
	{
		fprintf(stderr, "Please specify at least one package name on the command line.\n");
		return EXIT_FAILURE;
	}

	ret = EXIT_SUCCESS;

	if ((want_flags & PKG_SIMULATE) == PKG_SIMULATE)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkg_queue_apply(&pkgq, apply_simulate, -1, global_traverse_flags | PKGF_SKIP_ERRORS, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if (!pkg_queue_validate(&pkgq, maximum_traverse_depth, global_traverse_flags))
	{
		ret = EXIT_FAILURE;
		goto out;
	}

	if ((want_flags & PKG_UNINSTALLED) == PKG_UNINSTALLED)
	{
		ret = EXIT_FAILURE;
		pkg_queue_apply(&pkgq, apply_uninstalled, maximum_traverse_depth, global_traverse_flags, &ret);
		goto out;
	}

	if ((want_flags & PKG_DIGRAPH) == PKG_DIGRAPH)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkg_queue_apply(&pkgq, apply_digraph, maximum_traverse_depth, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_MODVERSION) == PKG_MODVERSION)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkg_queue_apply(&pkgq, apply_modversion, 2, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_VARIABLES) == PKG_VARIABLES)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkg_queue_apply(&pkgq, apply_variables, 2, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if (want_variable)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkg_queue_apply(&pkgq, apply_variable, 2, global_traverse_flags | PKGF_SKIP_ROOT_VIRTUAL, want_variable))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_REQUIRES) == PKG_REQUIRES)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkg_queue_apply(&pkgq, apply_requires, maximum_traverse_depth, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_REQUIRES_PRIVATE) == PKG_REQUIRES_PRIVATE)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkg_queue_apply(&pkgq, apply_requires_private, maximum_traverse_depth, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_CFLAGS))
	{
		pkg_list_t frag_list = PKG_LIST_INITIALIZER;

		if (!pkg_queue_apply(&pkgq, apply_cflags, maximum_traverse_depth, global_traverse_flags, &frag_list))
		{
			ret = EXIT_FAILURE;
			goto out_println;
		}
	}

	if ((want_flags & PKG_LIBS))
	{
		pkg_list_t frag_list = PKG_LIST_INITIALIZER;

		if (!pkg_queue_apply(&pkgq, apply_libs, maximum_traverse_depth, global_traverse_flags, &frag_list))
		{
			ret = EXIT_FAILURE;
			goto out_println;
		}
	}

	pkg_queue_free(&pkgq);

out_println:
	if (want_flags & (PKG_CFLAGS|PKG_LIBS))
		printf(" \n");

out:
	pkg_tuple_free_global();
	pkg_cache_free();

	return ret;
}
