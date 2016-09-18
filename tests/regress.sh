#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	variable \
	keep_system_libs \
	libs \
	libs_only \
	cflags_only \
	incomplete_libs \
	incomplete_cflags

variable_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"/test/include\n" \
		pkgconf --variable=includedir foo
}

keep_system_libs_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/local/lib  \n" \
		pkgconf --libs-only-L --keep-system-libs cflags-libs-only
}

libs_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/local/lib -lfoo  \n" \
		pkgconf --libs cflags-libs-only
}

libs_only_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/local/lib -lfoo  \n" \
		pkgconf --libs-only-L --libs-only-l cflags-libs-only
}

cflags_only_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-I/test/local/include/foo  \n" \
		pkgconf --cflags-only-I --cflags-only-other cflags-libs-only
}

incomplete_libs_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:" \n" \
		pkgconf --libs incomplete
}

incomplete_cflags_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:" \n" \
		pkgconf --cflags incomplete
}
