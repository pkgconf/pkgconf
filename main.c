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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <libgen.h>

#include "pkg.h"

#ifdef HAVE_GETOPT_LONG
# include <getopt.h>
#else
# include "bsdstubs.h"
#endif

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
static int maximum_traverse_depth = -1;

static char *required_pkgconfig_version = NULL;
static char *required_module_version = NULL;
static char *want_variable = NULL;

static bool
fragment_has_system_dir(pkg_fragment_t *frag)
{
	switch (frag->type)
	{
	case 'L':
		if (!strcasecmp(LIBDIR, frag->data))
			return true;
	case 'I':
		if (!strcasecmp(INCLUDEDIR, frag->data))
			return true;
	default:
		break;
	}

	return false;
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
collect_cflags(pkg_t *pkg, void *data)
{
	pkg_fragment_t **list = data;
	pkg_fragment_t *frag;

	foreach_list_entry(pkg->cflags, frag)
		*list = pkg_fragment_copy(*list, frag);
}

static void
print_cflags(pkg_fragment_t *list)
{
	pkg_fragment_t *frag;

	foreach_list_entry(list, frag)
	{
		if (want_cflags == WANT_CFLAGS_ONLY_I && frag->type != 'I')
			continue;
		else if (want_cflags == WANT_CFLAGS_ONLY_OTHER && frag->type == 'I')
			continue;

		print_fragment(frag);
	}
}

static void
collect_libs(pkg_t *pkg, void *data)
{
	pkg_fragment_t **list = data;
	pkg_fragment_t *frag;

	foreach_list_entry(pkg->libs, frag)
		*list = pkg_fragment_copy(*list, frag);

	if (want_static)
	{
		foreach_list_entry(pkg->libs_private, frag)
			*list = pkg_fragment_copy(*list, frag);
	}
}

static void
print_libs(pkg_fragment_t *list)
{
	pkg_fragment_t *frag;

	foreach_list_entry(list, frag)
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
print_modversion(pkg_t *pkg, void *unused)
{
	(void) unused;

	if (pkg->version != NULL)
		printf("%s\n", pkg->version);
}

static void
print_variable(pkg_t *pkg, void *unused)
{
	const char *variable_tmpl;
	(void) unused;

	variable_tmpl = tuple_find(pkg->vars, want_variable);
	if (variable_tmpl != NULL)
	{
		char *var = strdup_parse(pkg, variable_tmpl);

		printf("%s", var);

		free(var);

		exit(EXIT_SUCCESS);
	}
}

static void
print_variables(pkg_t *pkg, void *unused)
{
	pkg_tuple_t *node;
	(void) unused;

	foreach_list_entry(pkg->vars, node)
		printf("%s\n", node->key);
}

static void
print_requires(pkg_t *pkg, void *unused)
{
	pkg_dependency_t *node;
	(void) unused;

	foreach_list_entry(pkg->requires, node)
	{
		printf("%s", node->package);

		if (node->version != NULL)
			printf(" %s %s", pkg_get_comparator(node), node->version);

		printf("\n");
	}
}

static void
print_requires_private(pkg_t *pkg, void *unused)
{
	pkg_dependency_t *node;
	(void) unused;

	foreach_list_entry(pkg->requires_private, node)
	{
		printf("%s", node->package);

		if (node->version != NULL)
			printf(" %s %s", pkg_get_comparator(node), node->version);

		printf("\n");
	}
}

static void
print_digraph_node(pkg_t *pkg, void *unused)
{
	pkg_dependency_t *node;
	(void) unused;

	printf("\"%s\" [fontname=Sans fontsize=8]\n", pkg->id);

	foreach_list_entry(pkg->requires, node)
	{
		printf("\"%s\" -- \"%s\" [fontname=Sans fontsize=8]\n", node->package, pkg->id);
	}
}

static void
exit_if_uninstalled(pkg_t *pkg, void *unused)
{
	(void) unused;

	if (pkg->uninstalled)
		exit (0);
}

typedef struct pkg_queue_ {
	struct pkg_queue_ *prev, *next;
	const char *package;
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
	int wanted_something = 0;
	pkg_queue_t *pkgq;
	pkg_t world = (pkg_t){
		.id = "world",
		.realname = "virtual",
	};

	/* if maximum_traverse_depth is one, then we will not traverse deeper
	 * than our virtual package.
	 */
	if (!maximum_traverse_depth)
		maximum_traverse_depth = -1;
	else if (maximum_traverse_depth > 0)
		maximum_traverse_depth++;

	foreach_list_entry(head, pkgq)
	{
		pkg_dependency_t *pkgdep;

		pkgdep = parse_deplist(&world, pkgq->package);
		world.requires = pkg_dependency_append(world.requires, pkgdep);
	}

	/* we should verify that the graph is complete before attempting to compute cflags etc. */
	pkg_verify_graph(&world, maximum_traverse_depth, global_traverse_flags);

	if (want_uninstalled)
	{
		wanted_something = 0;
		pkg_traverse(&world, exit_if_uninstalled, NULL, maximum_traverse_depth, global_traverse_flags);
		return 1;
	}

	if (want_digraph)
	{
		printf("graph deptree {\n");
		printf("edge [color=blue len=7.5 fontname=Sans fontsize=8]\n");
		printf("node [fontname=Sans fontsize=8]\n");

		pkg_traverse(&world, print_digraph_node, NULL, maximum_traverse_depth, global_traverse_flags);

		printf("}\n");

		return EXIT_SUCCESS;
	}

	if (want_modversion)
	{
		wanted_something = 0;
		want_cflags = 0;
		want_libs = 0;

		pkg_traverse(&world, print_modversion, NULL, 2, global_traverse_flags);
	}

	if (want_variables)
	{
		wanted_something = 0;
		want_cflags = 0;
		want_libs = 0;

		pkg_traverse(&world, print_variables, NULL, 2, global_traverse_flags);
	}

	if (want_requires)
	{
		pkg_dependency_t *iter;

		wanted_something = 0;
		want_cflags = 0;
		want_libs = 0;

		foreach_list_entry(world.requires, iter)
		{
			pkg_t *pkg;

			pkg = pkg_verify_dependency(iter, global_traverse_flags, NULL);
			print_requires(pkg, NULL);
		}
	}

	if (want_requires_private)
	{
		pkg_dependency_t *iter;

		wanted_something = 0;
		want_cflags = 0;
		want_libs = 0;

		foreach_list_entry(world.requires, iter)
		{
			pkg_t *pkg;

			pkg = pkg_verify_dependency(iter, global_traverse_flags | PKGF_SEARCH_PRIVATE, NULL);
			print_requires_private(pkg, NULL);
		}
	}

	if (want_cflags)
	{
		pkg_fragment_t *list = NULL;

		wanted_something++;
		pkg_traverse(&world, collect_cflags, &list, maximum_traverse_depth, global_traverse_flags | PKGF_SEARCH_PRIVATE);
		print_cflags(list);
	}

	if (want_libs)
	{
		pkg_fragment_t *list = NULL;

		wanted_something++;
		pkg_traverse(&world, collect_libs, &list, maximum_traverse_depth, global_traverse_flags);
		print_libs(list);
	}

	if (want_variable)
	{
		wanted_something++;
		pkg_traverse(&world, print_variable, NULL, 2, global_traverse_flags);
	}

	if (wanted_something)
		printf("\n");

	return EXIT_SUCCESS;
}

static void
version(void)
{
	printf("%s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
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

	printf("\nchecking specific pkg-config database entries:\n\n");

	printf("  --atleast-version                 require a specific version of a module\n");
	printf("  --exists                          check whether or not a module exists\n");
	printf("  --uninstalled                     check whether or not an uninstalled module will be used\n");
	printf("  --no-uninstalled                  never use uninstalled modules when satisfying dependencies\n");
	printf("  --maximum-traverse-depth          maximum allowed depth for dependency graph\n");
	printf("  --static                          be more aggressive when computing dependency graph\n");
	printf("                                    (for static linking)\n");
	printf("  --env-only                        look only for package entries in PKG_CONFIG_PATH\n");

	printf("\nquerying specific pkg-config database fields:\n\n");

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

	printf("\nreport bugs to <%s>.\n", PACKAGE_BUGREPORT);
}

int
main(int argc, char *argv[])
{
	int ret;
	pkg_queue_t *pkgq = NULL;
	pkg_queue_t *pkgq_head = NULL;

	struct option options[] = {
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
		{ "libs-only-L", no_argument, &want_libs, WANT_LIBS_ONLY_LDPATH, },
		{ "libs-only-l", no_argument, &want_libs, WANT_LIBS_ONLY_LIBNAME, },
		{ "libs-only-other", no_argument, &want_libs, WANT_LIBS_ONLY_OTHER, },
		{ "uninstalled", no_argument, &want_uninstalled, 24, },
		{ "no-uninstalled", no_argument, &want_no_uninstalled, 25, },
		{ NULL, 0, NULL, 0 }
	};

	while ((ret = getopt_long(argc, argv, "", options, NULL)) != -1)
	{
		switch (ret)
		{
		case 2:
			required_module_version = optarg;
			break;
		case 3:
			required_pkgconfig_version = optarg;
			break;
		case 7:
			want_variable = optarg;
			break;
		case 11:
			maximum_traverse_depth = atoi(optarg);
			break;
		default:
			break;
		}
	}

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

	if (want_static)
		global_traverse_flags |= PKGF_SEARCH_PRIVATE;

	if (want_env_only)
		global_traverse_flags |= PKGF_ENV_ONLY;

	if (want_no_uninstalled || getenv("PKG_CONFIG_DISABLE_UNINSTALLED") != NULL)
		global_traverse_flags |= PKGF_NO_UNINSTALLED;

	if (required_pkgconfig_version != NULL)
	{
		if (pkg_compare_version(PKG_PKGCONFIG_VERSION_EQUIV, required_pkgconfig_version) >= 0)
			return EXIT_SUCCESS;

		return EXIT_FAILURE;
	}

	if (required_module_version != NULL)
	{
		pkg_t *pkg;
		const char *package;

		package = argv[optind];
		if (package == NULL)
			return EXIT_SUCCESS;

		pkg = pkg_find(package, global_traverse_flags);
		if (pkg == NULL)
			return EXIT_FAILURE;

		if (pkg_compare_version(pkg->version, required_module_version) >= 0)
			return EXIT_SUCCESS;

		return EXIT_FAILURE;
	}

	while (1)
	{
		const char *package = argv[optind];

		if (package == NULL)
			break;

		if (argv[optind + 1] == NULL || !PKG_OPERATOR_CHAR(*(argv[optind + 1])))
		{
			pkgq = pkg_queue_push(pkgq, package);

			if (pkgq_head == NULL)
				pkgq_head = pkgq;

			optind++;
		}
		else
		{
			char packagebuf[BUFSIZ];

			snprintf(packagebuf, sizeof packagebuf, "%s %s %s", package, argv[optind + 1], argv[optind + 2]);
			optind += 3;

			pkgq = pkg_queue_push(pkgq, packagebuf);

			if (pkgq_head == NULL)
				pkgq_head = pkgq;
		}
	}

	if (pkgq_head == NULL)
	{
		fprintf(stderr, "Please specify at least one package name on the command line.\n");
		return EXIT_FAILURE;
	}

	return pkg_queue_walk(pkgq_head);
}
