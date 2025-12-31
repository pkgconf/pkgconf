#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	define_variable \
	define_variable_override \
	libs \
	libs_only \
	libs_never_mergeback \
	cflags_only \
	cflags_never_mergeback \
	isystem_munge_sysroot \
	idirafter_munge_sysroot \
	modversion_common_prefix \
	modversion_one_word_expression \
	modversion_two_word_expression \
	modversion_three_word_expression \
	modversion_one_word_expression_no_space \
	modversion_one_word_expression_no_space_zero \
	malformed_1 \
	malformed_quoting \
	explicit_sysroot \
	define_prefix_child_prefix_1 \
	define_prefix_child_prefix_1_env

#	sysroot_munge \

define_variable_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check -o inline:"\\\${libdir}/typelibdir\n" \
		pkgconf --variable=typelibdir --define-variable='libdir=\${libdir}' typelibdir
}

define_variable_override_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check -o inline:"/test\n" \
		pkgconf --variable=prefix --define-variable='prefix=/test' typelibdir
}

libs_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/local/lib -lfoo\n" \
		pkgconf --libs cflags-libs-only
}

libs_only_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/local/lib -lfoo\n" \
		pkgconf --libs-only-L --libs-only-l cflags-libs-only
}

libs_never_mergeback_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/bar/lib -lfoo1\n" \
		pkgconf --libs prefix-foo1
	atf_check \
		-o inline:"-L/test/bar/lib -lfoo1 -lfoo2\n" \
		pkgconf --libs prefix-foo1 prefix-foo2
}

cflags_only_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-I/test/local/include/foo\n" \
		pkgconf --cflags-only-I --cflags-only-other cflags-libs-only
}

cflags_never_mergeback_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-I/test/bar/include/foo -DBAR -fPIC -DFOO\n" \
		pkgconf --cflags prefix-foo1 prefix-foo2
}

isystem_munge_sysroot_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1" PKG_CONFIG_SYSROOT_DIR="${selfdir}"
	atf_check \
		-o match:"-isystem ${selfdir}/opt/bad/include" \
		pkgconf --cflags isystem
}

idirafter_munge_sysroot_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1" PKG_CONFIG_SYSROOT_DIR="${selfdir}"
	atf_check \
		-o match:"-idirafter ${selfdir}/opt/bad/include" \
		pkgconf --cflags idirafter
}

sysroot_munge_body()
{
	sed "s|/sysroot/|${selfdir}/|g" ${selfdir}/lib1/sysroot-dir.pc > ${selfdir}/lib1/sysroot-dir-selfdir.pc
	export PKG_CONFIG_PATH="${selfdir}/lib1" PKG_CONFIG_SYSROOT_DIR="${selfdir}"
	atf_check \
		-o inline:"-L${selfdir}/lib -lfoo\n" \
		pkgconf --libs sysroot-dir-selfdir
}

malformed_1_body()
{
	atf_check -s exit:1 -o ignore \
		pkgconf --validate --with-path="${selfdir}/lib1" malformed-1
}

malformed_quoting_body()
{
	atf_check -s exit:0 -o ignore \
		pkgconf --validate --with-path="${selfdir}/lib1" malformed-quoting
}

explicit_sysroot_body()
{
	export PKG_CONFIG_SYSROOT_DIR=${selfdir}
	atf_check -o inline:"${selfdir}/usr/share/test\n" \
		pkgconf --with-path="${selfdir}/lib1" --variable=pkgdatadir explicit-sysroot
}

modversion_common_prefix_body()
{
	atf_check -o inline:"foo: 1.2.3\nfoobar: 3.2.1\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion --verbose foo foobar
}

modversion_one_word_expression_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion "foo > 1.0"
}

modversion_two_word_expression_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion foo "> 1.0"
}

modversion_three_word_expression_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion foo ">" 1.0
}

modversion_one_word_expression_no_space_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion "foo >1.0"
}

modversion_one_word_expression_no_space_zero_body()
{
	atf_check -o inline:"1.2.3\n" \
		pkgconf --with-path="${selfdir}/lib1" --modversion "foo >0.5"
}

define_prefix_child_prefix_1_body()
{
	atf_check -o inline:"-I${selfdir}/lib1/include/child-prefix-1 -L${selfdir}/lib1/lib64 -lchild-prefix-1\n" \
		pkgconf --with-path="${selfdir}/lib1/child-prefix/pkgconfig" --define-prefix --cflags --libs child-prefix-1
}

define_prefix_child_prefix_1_env_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1/child-prefix/pkgconfig"
	export PKG_CONFIG_RELOCATE_PATHS=1
	atf_check -o inline:"-I${selfdir}/lib1/include/child-prefix-1 -L${selfdir}/lib1/lib64 -lchild-prefix-1\n" \
		pkgconf --cflags --libs child-prefix-1
}
