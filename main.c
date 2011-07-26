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

#include <libgen.h>
#include <popt.h>

#include "pkg.h"

/* we are compatible with 0.26 of pkg-config */
#define PKGCONFIG_VERSION_EQUIV		"0.26"

static int want_version = 0;
static int want_cflags = 0;
static int want_libs = 0;
static int want_modversion = 0;
static int want_static = 0;
static int maximum_traverse_depth = 2;

static char *required_pkgconfig_version = NULL;
static char *required_module_version = NULL;
static char *want_variable = NULL;

static void
print_cflags(pkg_t *pkg, void *unused)
{
	if (pkg->cflags != NULL)
		printf("%s ", pkg->cflags);
}

static void
print_libs(pkg_t *pkg, void *unused)
{
	if (pkg->libs != NULL)
		printf("%s ", pkg->libs);
}

static void
print_modversion(pkg_t *pkg, void *unused)
{
	if (pkg->version != NULL)
		printf("%s\n", pkg->version);
}

static void
print_variable(pkg_t *pkg, void *unused)
{
	char *variable;

	variable = tuple_find(pkg->vars, want_variable);
	if (variable != NULL)
		printf("%s ", variable);
}

typedef struct pkg_queue_ {
	struct pkg_queue_ *prev, *next;
	const char *package;
} pkg_queue_t;

static pkg_queue_t *
pkg_queue_push(pkg_queue_t *parent, const char *package)
{
	pkg_queue_t *pkgq = calloc(sizeof(pkg_queue_t), 1);

	pkgq->package = package;
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
		.realname = "virtual"
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
	pkg_verify_graph(&world, maximum_traverse_depth);

	if (want_modversion)
	{
		wanted_something = 0;
		want_cflags = 0;
		want_libs = 0;

		pkg_traverse(&world, print_modversion, NULL, 2);
	}

	if (want_cflags)
	{
		wanted_something++;
		pkg_traverse(&world, print_cflags, NULL, maximum_traverse_depth);
	}

	if (want_libs)
	{
		wanted_something++;
		pkg_traverse(&world, print_libs, NULL, maximum_traverse_depth);
	}

	if (want_variable)
	{
		wanted_something++;
		pkg_traverse(&world, print_variable, NULL, 2);
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

int
main(int argc, const char *argv[])
{
	int ret;
	poptContext opt_context;
	pkg_queue_t *pkgq = NULL;
	pkg_queue_t *pkgq_head = NULL;

	struct poptOption options[] = {
		{ "version", 0, POPT_ARG_NONE, &want_version, 0, "output pkgconf version" },
		{ "atleast-version", 0, POPT_ARG_STRING, &required_module_version, 0, "require specified version of a package" },
		{ "atleast-pkgconfig-version", 0, POPT_ARG_STRING, &required_pkgconfig_version, 0, "require compatibility level with specified version of pkg-config" },
		{ "libs", 0, POPT_ARG_NONE, &want_libs, 0, "output all linker flags" },
		{ "cflags", 0, POPT_ARG_NONE, &want_cflags, 0, "output all compiler flags" },
		{ "modversion", 0, POPT_ARG_NONE, &want_modversion, 0, "output package version" },
		{ "variable", 0, POPT_ARG_STRING, &want_variable, 0, "get the value of the specified variable" },
		{ "exists", 0, POPT_ARG_NONE, NULL, 0, "return 0 if all packages present" },
		{ "print-errors", 0, POPT_ARG_NONE, NULL, 0, "dummy option for pkg-config compatibility" },
		{ "short-errors", 0, POPT_ARG_NONE, NULL, 0, "dummy option for pkg-config compatibility" },
		{ "maximum-traverse-depth", 0, POPT_ARG_INT, &maximum_traverse_depth, 0, "limits maximum traversal depth of the computed dependency graph" },
		{ "static", 0, POPT_ARG_NONE, &want_static, 0, "be more aggressive when walking the dependency graph (for intermediary static linking deps)" },
		POPT_AUTOHELP
		{ NULL, 0, 0, NULL, 0 }
	};

	opt_context = poptGetContext(NULL, argc, argv, options, 0);
	ret = poptGetNextOpt(opt_context);
	if (ret != -1)
	{
		fprintf(stderr, "%s: %s\n",
			poptBadOption(opt_context, POPT_BADOPTION_NOALIAS),
			poptStrerror(ret));
		return EXIT_FAILURE;
	}

	if (want_version)
	{
		version();
		return EXIT_SUCCESS;
	}

	if (want_static)
		maximum_traverse_depth++;

	if (required_pkgconfig_version != NULL)
	{
		if (pkg_compare_version(PKGCONFIG_VERSION_EQUIV, required_pkgconfig_version) >= 0)
			return EXIT_SUCCESS;

		return EXIT_FAILURE;
	}

	if (required_module_version != NULL)
	{
		pkg_t *pkg;
		const char *package;

		package = poptGetArg(opt_context);
		if (package == NULL)
			return EXIT_SUCCESS;

		pkg = pkg_find(package);
		if (pkg == NULL)
			return EXIT_FAILURE;

		if (pkg_compare_version(pkg->version, required_module_version) >= 0)
			return EXIT_SUCCESS;

		return EXIT_FAILURE;
	}

	while (1)
	{
		const char *package = poptGetArg(opt_context);
		if (package == NULL)
			break;

		pkgq = pkg_queue_push(pkgq, package);
		if (pkgq_head == NULL)
			pkgq_head = pkgq;
	}

	poptFreeContext(opt_context);

	if (pkgq_head == NULL)
	{
		fprintf(stderr, "Please specify at least one package name on the command line.\n");
		return EXIT_FAILURE;
	}

	return pkg_queue_walk(pkgq_head);
}
