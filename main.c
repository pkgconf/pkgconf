/*
 * main.c
 * main() routine, printer functions
 *
 * Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016, 2017
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

#include <libpkgconf/libpkgconf.h>
#include "config.h"
#include "getopt_long.h"

#define PKG_CFLAGS_ONLY_I		(((uint64_t) 1) << 2)
#define PKG_CFLAGS_ONLY_OTHER		(((uint64_t) 1) << 3)
#define PKG_CFLAGS			(PKG_CFLAGS_ONLY_I|PKG_CFLAGS_ONLY_OTHER)
#define PKG_LIBS_ONLY_LDPATH		(((uint64_t) 1) << 5)
#define PKG_LIBS_ONLY_LIBNAME		(((uint64_t) 1) << 6)
#define PKG_LIBS_ONLY_OTHER		(((uint64_t) 1) << 7)
#define PKG_LIBS			(PKG_LIBS_ONLY_LDPATH|PKG_LIBS_ONLY_LIBNAME|PKG_LIBS_ONLY_OTHER)
#define PKG_MODVERSION			(((uint64_t) 1) << 8)
#define PKG_REQUIRES			(((uint64_t) 1) << 9)
#define PKG_REQUIRES_PRIVATE		(((uint64_t) 1) << 10)
#define PKG_VARIABLES			(((uint64_t) 1) << 11)
#define PKG_DIGRAPH			(((uint64_t) 1) << 12)
#define PKG_KEEP_SYSTEM_CFLAGS		(((uint64_t) 1) << 13)
#define PKG_KEEP_SYSTEM_LIBS		(((uint64_t) 1) << 14)
#define PKG_VERSION			(((uint64_t) 1) << 15)
#define PKG_ABOUT			(((uint64_t) 1) << 16)
#define PKG_ENV_ONLY			(((uint64_t) 1) << 17)
#define PKG_ERRORS_ON_STDOUT		(((uint64_t) 1) << 18)
#define PKG_SILENCE_ERRORS		(((uint64_t) 1) << 19)
#define PKG_IGNORE_CONFLICTS		(((uint64_t) 1) << 20)
#define PKG_STATIC			(((uint64_t) 1) << 21)
#define PKG_NO_UNINSTALLED		(((uint64_t) 1) << 22)
#define PKG_UNINSTALLED			(((uint64_t) 1) << 23)
#define PKG_LIST			(((uint64_t) 1) << 24)
#define PKG_HELP			(((uint64_t) 1) << 25)
#define PKG_PRINT_ERRORS		(((uint64_t) 1) << 26)
#define PKG_SIMULATE			(((uint64_t) 1) << 27)
#define PKG_NO_CACHE			(((uint64_t) 1) << 28)
#define PKG_PROVIDES			(((uint64_t) 1) << 29)
#define PKG_VALIDATE			(((uint64_t) 1) << 30)
#define PKG_LIST_PACKAGE_NAMES		(((uint64_t) 1) << 31)
#define PKG_NO_PROVIDES			(((uint64_t) 1) << 32)
#define PKG_PURE			(((uint64_t) 1) << 33)
#define PKG_PATH			(((uint64_t) 1) << 34)

static unsigned int global_traverse_flags = PKGCONF_PKG_PKGF_NONE;

static pkgconf_client_t pkg_client;

static uint64_t want_flags;
static int maximum_traverse_depth = 2000;

static char *want_variable = NULL;

FILE *error_msgout = NULL;
FILE *logfile_out = NULL;

static bool
error_handler(const char *msg, const pkgconf_client_t *client, const void *data)
{
	(void) client;
	(void) data;
	fprintf(error_msgout, "%s", msg);
	return true;
}

static bool
print_list_entry(const pkgconf_pkg_t *entry, void *data)
{
	(void) data;

	if (entry->flags & PKGCONF_PKG_PROPF_UNINSTALLED)
		return false;

	printf("%-30s %s - %s\n", entry->id, entry->realname, entry->description);

	return false;
}

static bool
print_package_entry(const pkgconf_pkg_t *entry, void *data)
{
	(void) data;

	if (entry->flags & PKGCONF_PKG_PROPF_UNINSTALLED)
		return false;

	printf("%s\n", entry->id);

	return false;
}

static bool
filter_cflags(const pkgconf_client_t *client, const pkgconf_fragment_t *frag, unsigned int flags)
{
	int got_flags = 0;
	(void) client;
	(void) flags;

	if (pkgconf_fragment_has_system_dir(client, frag))
		return false;

	if (frag->type == 'I')
		got_flags = PKG_CFLAGS_ONLY_I;
	else
		got_flags = PKG_CFLAGS_ONLY_OTHER;

	return (want_flags & got_flags) != 0;
}

static bool
filter_libs(const pkgconf_client_t *client, const pkgconf_fragment_t *frag, unsigned int flags)
{
	int got_flags = 0;
	(void) client;
	(void) flags;

	if (pkgconf_fragment_has_system_dir(client, frag))
		return false;

	switch (frag->type)
	{
		case 'L': got_flags = PKG_LIBS_ONLY_LDPATH; break;
		case 'l': got_flags = PKG_LIBS_ONLY_LIBNAME; break;
		default: got_flags = PKG_LIBS_ONLY_OTHER; break;
	}

	return (want_flags & got_flags) != 0;
}

static void
print_modversion(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *unused, unsigned int flags)
{
	(void) client;
	(void) unused;
	(void) flags;

	if (pkg->version != NULL)
		printf("%s\n", pkg->version);
}

static void
print_variables(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *unused, unsigned int flags)
{
	pkgconf_node_t *node;
	(void) client;
	(void) unused;
	(void) flags;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->vars.head, node)
	{
		pkgconf_tuple_t *tuple = node->data;

		printf("%s\n", tuple->key);
	}
}

static void
print_requires(pkgconf_pkg_t *pkg)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->requires.head, node)
	{
		pkgconf_dependency_t *dep = node->data;

		printf("%s", dep->package);

		if (dep->version != NULL)
			printf(" %s %s", pkgconf_pkg_get_comparator(dep), dep->version);

		printf("\n");
	}
}

static void
print_requires_private(pkgconf_pkg_t *pkg)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->requires_private.head, node)
	{
		pkgconf_dependency_t *dep = node->data;

		printf("%s", dep->package);

		if (dep->version != NULL)
			printf(" %s %s", pkgconf_pkg_get_comparator(dep), dep->version);

		printf("\n");
	}
}

static void
print_provides(pkgconf_pkg_t *pkg)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->provides.head, node)
	{
		pkgconf_dependency_t *dep = node->data;

		printf("%s", dep->package);

		if (dep->version != NULL)
			printf(" %s %s", pkgconf_pkg_get_comparator(dep), dep->version);

		printf("\n");
	}
}

static bool
apply_provides(pkgconf_client_t *client, pkgconf_pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	pkgconf_node_t *iter;
	(void) unused;
	(void) maxdepth;

	PKGCONF_FOREACH_LIST_ENTRY(world->requires.head, iter)
	{
		pkgconf_pkg_t *pkg;
		pkgconf_dependency_t *dep = iter->data;

		pkg = pkgconf_pkg_verify_dependency(client, dep, flags, NULL);
		print_provides(pkg);

		pkgconf_pkg_free(&pkg_client, pkg);
	}

	return true;
}

static void
print_digraph_node(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *unused, unsigned int flags)
{
	pkgconf_node_t *node;
	(void) client;
	(void) unused;
	(void) flags;

	printf("\"%s\" [fontname=Sans fontsize=8]\n", pkg->id);

	PKGCONF_FOREACH_LIST_ENTRY(pkg->requires.head, node)
	{
		pkgconf_dependency_t *dep = node->data;

		printf("\"%s\" -- \"%s\" [fontname=Sans fontsize=8]\n", dep->package, pkg->id);
	}
}

static bool
apply_digraph(pkgconf_client_t *client, pkgconf_pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	int eflag;

	printf("graph deptree {\n");
	printf("edge [color=blue len=7.5 fontname=Sans fontsize=8]\n");
	printf("node [fontname=Sans fontsize=8]\n");

	eflag = pkgconf_pkg_traverse(client, world, print_digraph_node, unused, maxdepth, flags);

	if (eflag != PKGCONF_PKG_ERRF_OK)
		return false;

	printf("}\n");
	return true;
}

static bool
apply_modversion(pkgconf_client_t *client, pkgconf_pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkgconf_pkg_traverse(client, world, print_modversion, unused, maxdepth, flags);

	if (eflag != PKGCONF_PKG_ERRF_OK)
		return false;

	return true;
}

static bool
apply_variables(pkgconf_client_t *client, pkgconf_pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkgconf_pkg_traverse(client, world, print_variables, unused, maxdepth, flags);

	if (eflag != PKGCONF_PKG_ERRF_OK)
		return false;

	return true;
}

static void
print_path(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data, unsigned int flags)
{
	(void) client;
	(void) data;
	(void) flags;

	printf("%s\n", pkg->filename);
}

static bool
apply_path(pkgconf_client_t *client, pkgconf_pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkgconf_pkg_traverse(client, world, print_path, unused, maxdepth, flags);

	if (eflag != PKGCONF_PKG_ERRF_OK)
		return false;

	return true;
}

typedef struct {
	const char *variable;
	char buf[PKGCONF_BUFSIZE];
} var_request_t;

static void
print_variable(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data, unsigned int flags)
{
	var_request_t *req = data;
	const char *var;
	(void) flags;

	var = pkgconf_tuple_find(client, &pkg->vars, req->variable);
	if (var != NULL)
	{
		if (*(req->buf) == '\0')
		{
			pkgconf_strlcpy(req->buf, var, sizeof(req->buf));
			return;
		}

		pkgconf_strlcat(req->buf, " ", sizeof(req->buf));
		pkgconf_strlcat(req->buf, var, sizeof(req->buf));
	}
}

static bool
apply_variable(pkgconf_client_t *client, pkgconf_pkg_t *world, void *variable, int maxdepth, unsigned int flags)
{
	int eflag;

	var_request_t req = {
		.variable = variable,
	};

	*req.buf = '\0';

	eflag = pkgconf_pkg_traverse(client, world, print_variable, &req, maxdepth, flags);
	if (eflag != PKGCONF_PKG_ERRF_OK)
		return false;

	printf("%s\n", req.buf);
	return true;
}

static bool
apply_cflags(pkgconf_client_t *client, pkgconf_pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	pkgconf_list_t unfiltered_list = PKGCONF_LIST_INITIALIZER;
	pkgconf_list_t filtered_list = PKGCONF_LIST_INITIALIZER;
	int eflag;
	char *render_buf;
	(void) unused;

	eflag = pkgconf_pkg_cflags(client, world, &unfiltered_list, maxdepth, flags | PKGCONF_PKG_PKGF_SEARCH_PRIVATE);
	if (eflag != PKGCONF_PKG_ERRF_OK)
		return false;

	pkgconf_fragment_filter(client, &filtered_list, &unfiltered_list, filter_cflags, flags);

	if (filtered_list.head == NULL)
		return true;

	render_buf = pkgconf_fragment_render(&filtered_list);
	printf("%s", render_buf);
	free(render_buf);

	pkgconf_fragment_free(&unfiltered_list);
	pkgconf_fragment_free(&filtered_list);

	return true;
}

static bool
apply_libs(pkgconf_client_t *client, pkgconf_pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	pkgconf_list_t unfiltered_list = PKGCONF_LIST_INITIALIZER;
	pkgconf_list_t filtered_list = PKGCONF_LIST_INITIALIZER;
	int eflag;
	char *render_buf;
	(void) unused;

	eflag = pkgconf_pkg_libs(client, world, &unfiltered_list, maxdepth, flags);
	if (eflag != PKGCONF_PKG_ERRF_OK)
		return false;

	pkgconf_fragment_filter(client, &filtered_list, &unfiltered_list, filter_libs, flags);

	if (filtered_list.head == NULL)
		return true;

	render_buf = pkgconf_fragment_render(&filtered_list);
	printf("%s", render_buf);
	free(render_buf);

	pkgconf_fragment_free(&unfiltered_list);
	pkgconf_fragment_free(&filtered_list);

	return true;
}

static bool
apply_requires(pkgconf_client_t *client, pkgconf_pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	pkgconf_node_t *iter;
	(void) unused;
	(void) maxdepth;

	PKGCONF_FOREACH_LIST_ENTRY(world->requires.head, iter)
	{
		pkgconf_pkg_t *pkg;
		pkgconf_dependency_t *dep = iter->data;

		pkg = pkgconf_pkg_verify_dependency(client, dep, flags, NULL);
		print_requires(pkg);

		pkgconf_pkg_free(&pkg_client, pkg);
	}

	return true;
}

static bool
apply_requires_private(pkgconf_client_t *client, pkgconf_pkg_t *world, void *unused, int maxdepth, unsigned int flags)
{
	pkgconf_node_t *iter;
	(void) unused;
	(void) maxdepth;

	PKGCONF_FOREACH_LIST_ENTRY(world->requires.head, iter)
	{
		pkgconf_pkg_t *pkg;
		pkgconf_dependency_t *dep = iter->data;

		pkg = pkgconf_pkg_verify_dependency(client, dep, flags | PKGCONF_PKG_PKGF_SEARCH_PRIVATE, NULL);
		print_requires_private(pkg);

		pkgconf_pkg_free(&pkg_client, pkg);
	}
	return true;
}

static void
check_uninstalled(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data, unsigned int flags)
{
	int *retval = data;
	(void) client;
	(void) flags;

	if (pkg->flags & PKGCONF_PKG_PROPF_UNINSTALLED)
		*retval = EXIT_SUCCESS;
}

static bool
apply_uninstalled(pkgconf_client_t *client, pkgconf_pkg_t *world, void *data, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkgconf_pkg_traverse(client, world, check_uninstalled, data, maxdepth, flags);

	if (eflag != PKGCONF_PKG_ERRF_OK)
		return false;

	return true;
}

static void
print_graph_node(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data, unsigned int flags)
{
	pkgconf_node_t *n;

	(void) client;
	(void) data;
	(void) flags;

	printf("node '%s' {\n", pkg->id);

	if (pkg->version != NULL)
		printf("    version = '%s';\n", pkg->version);

	PKGCONF_FOREACH_LIST_ENTRY(pkg->requires.head, n)
	{
		pkgconf_dependency_t *dep = n->data;
		printf("    dependency '%s'", dep->package);
		if (dep->compare != PKGCONF_CMP_ANY)
		{
			printf(" {\n");
			printf("        comparator = '%s';\n", pkgconf_pkg_get_comparator(dep));
			printf("        version = '%s';\n", dep->version);
			printf("    };\n");
		}
		else
			printf(";\n");
	}

	printf("};\n");
}

static bool
apply_simulate(pkgconf_client_t *client, pkgconf_pkg_t *world, void *data, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkgconf_pkg_traverse(client, world, print_graph_node, data, maxdepth, flags);

	if (eflag != PKGCONF_PKG_ERRF_OK)
		return false;

	return true;
}

static void
version(void)
{
	printf("%s\n", PACKAGE_VERSION);
}

static void
about(void)
{
	printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	printf("Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016, 2017\n");
	printf("    pkgconf authors (see AUTHORS in documentation directory).\n\n");
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
	printf("  --list-package-names              list all known package names\n");
	printf("  --simulate                        simulate walking the calculated dependency graph\n");
	printf("  --no-cache                        do not cache already seen packages when\n");
	printf("                                    walking the dependency graph\n");
	printf("  --log-file=filename               write an audit log to a specified file\n");
	printf("  --with-path=path                  adds a directory to the search path\n");

	printf("\nchecking specific pkg-config database entries:\n\n");

	printf("  --atleast-version                 require a specific version of a module\n");
	printf("  --exact-version                   require an exact version of a module\n");
	printf("  --max-version                     require a maximum version of a module\n");
	printf("  --exists                          check whether or not a module exists\n");
	printf("  --uninstalled                     check whether or not an uninstalled module will be used\n");
	printf("  --no-uninstalled                  never use uninstalled modules when satisfying dependencies\n");
	printf("  --no-provides                     do not use 'provides' rules to resolve dependencies\n");
	printf("  --maximum-traverse-depth          maximum allowed depth for dependency graph\n");
	printf("  --static                          be more aggressive when computing dependency graph\n");
	printf("                                    (for static linking)\n");
	printf("  --pure                            optimize a static dependency graph as if it were a normal\n");
	printf("                                    dependency graph\n");
	printf("  --env-only                        look only for package entries in PKG_CONFIG_PATH\n");
	printf("  --ignore-conflicts                ignore 'conflicts' rules in modules\n");
	printf("  --validate                        validate specific .pc files for correctness\n");

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
	printf("  --print-provides                  print provided dependencies to stdout\n");
	printf("  --print-variables                 print all known variables in module to stdout\n");
	printf("  --digraph                         print entire dependency graph in graphviz 'dot' format\n");
	printf("  --keep-system-cflags              keep -I%s entries in cflags output\n", SYSTEM_INCLUDEDIR);
	printf("  --keep-system-libs                keep -L%s entries in libs output\n", SYSTEM_LIBDIR);
	printf("  --path                            show the exact filenames for any matching .pc files\n");

	printf("\nreport bugs to <%s>.\n", PACKAGE_BUGREPORT);
}

int
main(int argc, char *argv[])
{
	int ret;
	pkgconf_list_t pkgq = PKGCONF_LIST_INITIALIZER;
	char *builddir;
	char *sysroot_dir;
	char *required_pkgconfig_version = NULL;
	char *required_exact_module_version = NULL;
	char *required_max_module_version = NULL;
	char *required_module_version = NULL;
	char *logfile_arg = NULL;

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
		{ "pure", no_argument, &want_flags, PKG_PURE, },
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
		{ "list-package-names", no_argument, &want_flags, PKG_LIST_PACKAGE_NAMES|PKG_PRINT_ERRORS, },
		{ "simulate", no_argument, &want_flags, PKG_SIMULATE, },
		{ "no-cache", no_argument, &want_flags, PKG_NO_CACHE, },
		{ "print-provides", no_argument, &want_flags, PKG_PROVIDES, },
		{ "no-provides", no_argument, &want_flags, PKG_NO_PROVIDES, },
		{ "debug", no_argument, &want_flags, 0, },
		{ "validate", no_argument, NULL, 0 },
		{ "log-file", required_argument, NULL, 40 },
		{ "path", no_argument, &want_flags, PKG_PATH },
		{ "with-path", required_argument, NULL, 42 },
		{ NULL, 0, NULL, 0 }
	};

	pkgconf_client_init(&pkg_client, error_handler, NULL);

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
			pkgconf_tuple_define_global(&pkg_client, pkg_optarg);
			break;
		case 28:
			required_exact_module_version = pkg_optarg;
			break;
		case 29:
			required_max_module_version = pkg_optarg;
			break;
		case 40:
			logfile_arg = pkg_optarg;
			break;
		case 42:
			pkgconf_path_add(pkg_optarg, &pkg_client.dir_list, true);
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
		if (argc > 2)
		{
			fprintf(stderr, "%s: --version specified with other options or module names, assuming --modversion.\n", argv[0]);

			want_flags &= ~PKG_VERSION;
			want_flags |= PKG_MODVERSION;
		}
		else
		{
			version();
			return EXIT_SUCCESS;
		}
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
		global_traverse_flags |= PKGCONF_PKG_PKGF_SKIP_CONFLICTS;

	if ((want_flags & PKG_STATIC) == PKG_STATIC)
		global_traverse_flags |= (PKGCONF_PKG_PKGF_SEARCH_PRIVATE | PKGCONF_PKG_PKGF_MERGE_PRIVATE_FRAGMENTS);

	/* if --static and --pure are both specified, then disable merge-back.
	 * this allows for a --static which searches private modules, but has the same fragment behaviour as if
	 * --static were disabled.  see <https://github.com/pkgconf/pkgconf/issues/83> for rationale.
	 */
	if ((want_flags & PKG_PURE) == PKG_PURE || getenv("PKG_CONFIG_PURE_DEPGRAPH") != NULL)
		global_traverse_flags &= ~PKGCONF_PKG_PKGF_MERGE_PRIVATE_FRAGMENTS;

	if ((want_flags & PKG_ENV_ONLY) == PKG_ENV_ONLY)
		global_traverse_flags |= PKGCONF_PKG_PKGF_ENV_ONLY;

	if ((want_flags & PKG_NO_CACHE) == PKG_NO_CACHE)
		global_traverse_flags |= PKGCONF_PKG_PKGF_NO_CACHE;

	if ((want_flags & PKG_NO_UNINSTALLED) == PKG_NO_UNINSTALLED || getenv("PKG_CONFIG_DISABLE_UNINSTALLED") != NULL)
		global_traverse_flags |= PKGCONF_PKG_PKGF_NO_UNINSTALLED;

	if ((want_flags & PKG_NO_PROVIDES) == PKG_NO_PROVIDES)
		global_traverse_flags |= PKGCONF_PKG_PKGF_SKIP_PROVIDES;

	if (getenv("PKG_CONFIG_ALLOW_SYSTEM_CFLAGS") != NULL)
		want_flags |= PKG_KEEP_SYSTEM_CFLAGS;

	if (getenv("PKG_CONFIG_ALLOW_SYSTEM_LIBS") != NULL)
		want_flags |= PKG_KEEP_SYSTEM_LIBS;

	if ((builddir = getenv("PKG_CONFIG_TOP_BUILD_DIR")) != NULL)
		pkgconf_client_set_buildroot_dir(&pkg_client, builddir);

	if ((sysroot_dir = getenv("PKG_CONFIG_SYSROOT_DIR")) != NULL)
		pkgconf_client_set_sysroot_dir(&pkg_client, sysroot_dir);

	/* at this point, global_traverse_flags should be set, so build the dir list */
	pkgconf_pkg_dir_list_build(&pkg_client, global_traverse_flags);

	if (required_pkgconfig_version != NULL)
	{
		if (pkgconf_compare_version(PACKAGE_VERSION, required_pkgconfig_version) >= 0)
			return EXIT_SUCCESS;

		return EXIT_FAILURE;
	}

	if ((want_flags & PKG_LIST) == PKG_LIST)
	{
		pkgconf_scan_all(&pkg_client, NULL, print_list_entry);
		return EXIT_SUCCESS;
	}

	if ((want_flags & PKG_LIST_PACKAGE_NAMES) == PKG_LIST_PACKAGE_NAMES)
	{
		pkgconf_scan_all(&pkg_client, NULL, print_package_entry);
		return EXIT_SUCCESS;
	}

	if (logfile_arg == NULL)
		logfile_arg = getenv("PKG_CONFIG_LOG");

	if (logfile_arg != NULL)
	{
		logfile_out = fopen(logfile_arg, "w");
		pkgconf_audit_set_log(&pkg_client, logfile_out);
	}

	if (required_module_version != NULL)
	{
		pkgconf_pkg_t *pkg;
		pkgconf_node_t *node;
		pkgconf_list_t deplist = PKGCONF_LIST_INITIALIZER;

		while (argv[pkg_optind])
		{
			pkgconf_dependency_parse_str(&deplist, argv[pkg_optind]);
			pkg_optind++;
		}

		PKGCONF_FOREACH_LIST_ENTRY(deplist.head, node)
		{
			pkgconf_dependency_t *pkgiter = node->data;

			pkg = pkgconf_pkg_find(&pkg_client, pkgiter->package, global_traverse_flags);
			if (pkg == NULL)
				return EXIT_FAILURE;

			if (pkgconf_compare_version(pkg->version, required_module_version) >= 0)
				return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	if (required_exact_module_version != NULL)
	{
		pkgconf_pkg_t *pkg;
		pkgconf_node_t *node;
		pkgconf_list_t deplist = PKGCONF_LIST_INITIALIZER;

		while (argv[pkg_optind])
		{
			pkgconf_dependency_parse_str(&deplist, argv[pkg_optind]);
			pkg_optind++;
		}

		PKGCONF_FOREACH_LIST_ENTRY(deplist.head, node)
		{
			pkgconf_dependency_t *pkgiter = node->data;

			pkg = pkgconf_pkg_find(&pkg_client, pkgiter->package, global_traverse_flags);
			if (pkg == NULL)
				return EXIT_FAILURE;

			if (pkgconf_compare_version(pkg->version, required_exact_module_version) == 0)
				return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	if (required_max_module_version != NULL)
	{
		pkgconf_pkg_t *pkg;
		pkgconf_node_t *node;
		pkgconf_list_t deplist = PKGCONF_LIST_INITIALIZER;

		while (argv[pkg_optind])
		{
			pkgconf_dependency_parse_str(&deplist, argv[pkg_optind]);
			pkg_optind++;
		}

		PKGCONF_FOREACH_LIST_ENTRY(deplist.head, node)
		{
			pkgconf_dependency_t *pkgiter = node->data;

			pkg = pkgconf_pkg_find(&pkg_client, pkgiter->package, global_traverse_flags);
			if (pkg == NULL)
				return EXIT_FAILURE;

			if (pkgconf_compare_version(pkg->version, required_max_module_version) <= 0)
				return EXIT_SUCCESS;
		}

		return EXIT_FAILURE;
	}

	while (1)
	{
		const char *package = argv[pkg_optind];

		if (package == NULL)
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
		return EXIT_FAILURE;
	}

	ret = EXIT_SUCCESS;

	if ((want_flags & PKG_SIMULATE) == PKG_SIMULATE)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_simulate, -1, global_traverse_flags | PKGCONF_PKG_PKGF_SKIP_ERRORS, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if (!pkgconf_queue_validate(&pkg_client, &pkgq, maximum_traverse_depth, global_traverse_flags))
	{
		ret = EXIT_FAILURE;
		goto out;
	}

	if ((want_flags & PKG_VALIDATE) == PKG_VALIDATE)
		return 0;

	if ((want_flags & PKG_UNINSTALLED) == PKG_UNINSTALLED)
	{
		ret = EXIT_FAILURE;
		pkgconf_queue_apply(&pkg_client, &pkgq, apply_uninstalled, maximum_traverse_depth, global_traverse_flags, &ret);
		goto out;
	}

	if ((want_flags & PKG_PROVIDES) == PKG_PROVIDES)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_provides, maximum_traverse_depth, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_DIGRAPH) == PKG_DIGRAPH)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_digraph, maximum_traverse_depth, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_MODVERSION) == PKG_MODVERSION)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_modversion, 2, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_PATH) == PKG_PATH)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_path, 2, global_traverse_flags | PKGCONF_PKG_PKGF_SKIP_ROOT_VIRTUAL, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_VARIABLES) == PKG_VARIABLES)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_variables, 2, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if (want_variable)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_variable, 2, global_traverse_flags | PKGCONF_PKG_PKGF_SKIP_ROOT_VIRTUAL, want_variable))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_REQUIRES) == PKG_REQUIRES)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_requires, maximum_traverse_depth, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_REQUIRES_PRIVATE) == PKG_REQUIRES_PRIVATE)
	{
		want_flags &= ~(PKG_CFLAGS|PKG_LIBS);

		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_requires_private, maximum_traverse_depth, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out;
		}
	}

	if ((want_flags & PKG_CFLAGS))
	{
		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_cflags, maximum_traverse_depth, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out_println;
		}
	}

	if ((want_flags & PKG_LIBS))
	{
		if (!pkgconf_queue_apply(&pkg_client, &pkgq, apply_libs, maximum_traverse_depth, global_traverse_flags, NULL))
		{
			ret = EXIT_FAILURE;
			goto out_println;
		}
	}

	pkgconf_queue_free(&pkgq);

out_println:
	if (want_flags & (PKG_CFLAGS|PKG_LIBS))
		printf(" \n");

out:
	pkgconf_client_deinit(&pkg_client);

	if (logfile_out != NULL)
		fclose(logfile_out);

	return ret;
}
