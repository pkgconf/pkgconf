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

#include <popt.h>

#include "pkg.h"

static int want_cflags = 0;
static int want_libs = 0;

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

	foreach_list_entry(head, pkgq)
	{
		pkg_t *pkg;

		pkg = pkg_find(pkgq->package);
		if (pkg)
		{
			if (want_cflags)
			{
				wanted_something++;
				pkg_traverse(pkg, print_cflags, NULL);
			}

			if (want_libs)
			{
				wanted_something++;
				pkg_traverse(pkg, print_libs, NULL);
			}
		}
		else
		{
			fprintf(stderr, "dependency '%s' could not be satisfied, see PKG_CONFIG_PATH.\n", pkgq->package);
			return EXIT_FAILURE;
		}
	}

	if (wanted_something)
		printf("\n");

	return EXIT_SUCCESS;
}

int
main(int argc, const char *argv[])
{
	int ret;
	poptContext opt_context;
	pkg_queue_t *pkgq = NULL;
	pkg_queue_t *pkgq_head = NULL;

	struct poptOption options[] = {
		{ "libs", 0, POPT_ARG_NONE, &want_libs, 0, "output all linker flags" },
		{ "cflags", 0, POPT_ARG_NONE, &want_cflags, 0, "output all compiler flags" },
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

	return pkg_queue_walk(pkgq_head);
}
