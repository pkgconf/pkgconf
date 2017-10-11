#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	cflags \
	variable \
	do_not_eat_slash \

do_not_eat_slash_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	export PKG_CONFIG_SYSROOT_DIR="/"
	atf_check \
		-o inline:"-fPIC -I/test/include/foo \n" \
		pkgconf --cflags baz
}

cflags_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	export PKG_CONFIG_SYSROOT_DIR="${SYSROOT_DIR}"
	atf_check \
		-o inline:"-fPIC -I${SYSROOT_DIR}/test/include/foo \n" \
		pkgconf --cflags baz
}

variable_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	export PKG_CONFIG_SYSROOT_DIR="${SYSROOT_DIR}"
	atf_check \
		-o inline:"${SYSROOT_DIR}/test\n" \
		pkgconf --variable=prefix foo
	atf_check \
		-o inline:"${SYSROOT_DIR}/test/include\n" \
		pkgconf --variable=includedir foo
}
