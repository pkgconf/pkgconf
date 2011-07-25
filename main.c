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

#include "pkg.h"

static int want_cflags = 1;
static int want_libs = 1;

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

int
handle_package(const char *package)
{
	pkg_t *pkg;

	pkg = pkg_find(package);
	if (pkg)
	{
		if (want_cflags)
			pkg_traverse(pkg, print_cflags, NULL);

		if (want_libs)
			pkg_traverse(pkg, print_libs, NULL);

		printf("\n");
	}
	else
	{
		fprintf(stderr, "dependency '%s' could not be satisfied, see PKG_CONFIG_PATH.\n", package);
		return -1;
	}
}

int
main(int argc, const char *argv[])
{
	handle_package(argv[1]);
	return 0;
}
