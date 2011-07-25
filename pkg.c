/*
 * pkg.c
 * main() routine and basic dependency solving...
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

pkg_t *
pkg_find(const char *name)
{
	char locbuf[BUFSIZ];

	snprintf(locbuf, sizeof locbuf, "/usr/lib/pkgconfig/%s.pc", name);

	return parse_file(locbuf);
}

void
pkg_traverse(pkg_t *root,
	void (*pkg_traverse_func)(pkg_t *package, void *data),
	void *data)
{
	pkg_dependency_t *node;

	foreach_list_entry(root->requires, node)
	{
		pkg_t *pkgdep;

		pkgdep = pkg_find(node->package);
		if (pkgdep == NULL)
		{
			fprintf(stderr, "dependency '%s' is not satisfiable, see PKG_CONFIG_PATH\n", node->package);
			continue;
		}

		pkg_traverse(pkgdep, pkg_traverse_func, data);
	}

	pkg_traverse_func(root, data);
}

void
print_cflags(pkg_t *pkg, void *unused)
{
	if (pkg->cflags != NULL)
		printf("%s ", pkg->cflags);
}

void
print_libs(pkg_t *pkg, void *unused)
{
	if (pkg->libs != NULL)
		printf("%s ", pkg->libs);
}

int main(int argc, const char *argv[])
{
	pkg_t *pkg;

	pkg = pkg_find(argv[1]);
	if (pkg)
	{
		pkg_traverse(pkg, print_cflags, NULL);
		pkg_traverse(pkg, print_libs, NULL);
		printf("\n");
	}
	else
	{
		printf("%s not found\n", argv[1]);
		return -1;
	}

	return 0;
}

