#include <stdio.h>
#include <assert.h>

#include "../libpkgconf.h"

void test_simple()
{
	int argc;
	char **argv;

	pkgconf_argv_split("A B", &argc, &argv);
	assert(argc == 2);
	assert(!strcmp(argv[0], "A"));
	assert(!strcmp(argv[1], "B"));
	pkgconf_argv_free(argv);
}

void test_escaped()
{
	int argc;
	char **argv;

	pkgconf_argv_split("A\\ B", &argc, &argv);
	assert(argc == 1);
	assert(!strcmp(argv[0], "A\\ B"));
	pkgconf_argv_free(argv);
}

void test_quoted()
{
	int argc;
	char **argv;

	pkgconf_argv_split("\"A B\"", &argc, &argv);
	assert(argc == 1);
	assert(!strcmp(argv[0], "\"A B\""));
	pkgconf_argv_free(argv);
}

int main(int argc, char **argv)
{
	(void) argc; (void) argv;
	test_simple();
	test_escaped();
	test_quoted();
}
