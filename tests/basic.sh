#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	libs_env \
	exists_nonexitent \
	nonexitent \
	exists_version \
	exists_version_bad \
	exists_version_bad2 \
	exists_version_bad3 \
	exists \
	exists2 \
	exists3 \
	exists_version_alt \
	exists_cflags \
	exists_cflags_env \
	uninstalled_bad \
	uninstalled \
	libs_intermediary \
	libs_circular1 \
	libs_circular2 \
	libs_circular_directpc \
	libs_static \
	libs_static_ordering \
	libs_metapackage \
	license_isc \
	license_noassertion \
	license_file_foo \
	license_file_empty \
	modversion_noflatten \
	pkg_config_path \
	nolibs \
	nocflags \
	arbitary_path \
	with_path \
	relocatable \
	single_depth_selectors \
	source_foo \
	source_empty \
	print_variables_env \
	variable_env \
	variable_no_recurse \
	tuple_env

exists_nonexitent_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-s exit:1 \
		pkgconf --exists nonexistant
}

nonexitent_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-s exit:1 \
		pkgconf nonexistant
}

exists_version_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		pkgconf --exists 'foo > 1.2'
}

exists_version_bad_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-s exit:1 \
		pkgconf --exists 'foo > 1.2.3'
}

exists_version_alt_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		pkgconf --exists 'foo' '>' '1.2'
}

uninstalled_bad_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-s exit:1 \
		pkgconf --uninstalled 'foo'
}

uninstalled_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		pkgconf --uninstalled 'omg'
}

exists_version_bad2_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-s exit:1 \
		-e ignore \
		pkgconf --exists 'foo >= '
}

exists_version_bad3_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-s exit:1 \
		pkgconf --exists 'tilde >= 1.0.0'
}

exists_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		pkgconf --exists 'tilde = 1.0.0~rc1'
}

exists2_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		pkgconf --exists 'tilde <= 1.0.0'
}

exists3_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		pkgconf --exists '' 'foo'
}

libs_intermediary_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-lintermediary-1 -lintermediary-2 -lfoo -lbar -lbaz\n" \
		pkgconf --libs intermediary-1 intermediary-2
}

libs_circular2_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"circular-1: breaking circular reference (circular-1 -> circular-2 -> circular-1)\n" \
		pkgconf circular-2 --validate
}

libs_circular1_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"circular-3: breaking circular reference (circular-3 -> circular-1 -> circular-3)\n" \
		pkgconf circular-1 --validate
}

libs_circular_directpc_body()
{
	atf_check \
		-o inline:"-lcircular-3 -lcircular-1 -lcircular-2\n" \
		pkgconf --libs ${selfdir}/lib1/circular-3.pc
}

libs_static_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"/libfoo.a -pthread\n" \
		pkgconf --libs static-archive-libs
}

libs_static_ordering_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/lib -lbar -lfoo\n" \
		pkgconf --libs foo bar
}

libs_metapackage_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/lib -lbar -lfoo\n" \
		pkgconf --static --libs metapackage-3
}

pkg_config_path_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1${PATH_SEP}${selfdir}/lib2"
	atf_check \
		-o inline:"-L/test/lib -lfoo\n" \
		pkgconf --libs foo
	atf_check \
		-o inline:"-L/test/lib -lbar -lfoo\n" \
		pkgconf --libs bar
}

with_path_body()
{
	atf_check \
		-o inline:"-L/test/lib -lfoo\n" \
		pkgconf --with-path=${selfdir}/lib1 --with-path=${selfdir}/lib2 --libs foo
	atf_check \
		-o inline:"-L/test/lib -lbar -lfoo\n" \
		pkgconf --with-path=${selfdir}/lib1 --with-path=${selfdir}/lib2 --libs bar
}

nolibs_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"\n" \
		pkgconf --libs nolib
}

nocflags_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"\n" \
		pkgconf --cflags nocflag
}

arbitary_path_body()
{
	cp ${selfdir}/lib1/foo.pc .
	atf_check \
		-o inline:"-L/test/lib -lfoo\n" \
		pkgconf --libs foo.pc
}

relocatable_body()
{
	basedir=$(pkgconf --relocate ${selfdir})
	atf_check \
		-o inline:"${basedir}/lib-relocatable\n" \
		pkgconf --define-prefix --variable=prefix ${basedir}/lib-relocatable/lib/pkgconfig/foo.pc
}

single_depth_selectors_body()
{
	export PKG_CONFIG_MAXIMUM_TRAVERSE_DEPTH=1
	atf_check \
		-o inline:"foo\n" \
		pkgconf --with-path=${selfdir}/lib3 --print-requires bar
}

license_isc_body()
{
	atf_check \
		-o inline:"foo: ISC\n" \
		pkgconf --with-path=${selfdir}/lib1 --license foo
}

license_noassertion_body()
{
	atf_check \
		-o inline:"bar: NOASSERTION\nfoo: ISC\n" \
		pkgconf --with-path=${selfdir}/lib1 --license bar
}

license_file_foo_body()
{
	atf_check \
		-o inline:"foo: https://foo.bar/foo/COPYING\n" \
		pkgconf --with-path=${selfdir}/lib1 --license-file foo
}

license_file_empty_body()
{
	atf_check \
		-o inline:"bar: \nfoo: https://foo.bar/foo/COPYING\n" \
		pkgconf --with-path=${selfdir}/lib1 --license-file bar
}

source_foo_body()
{
	atf_check \
		-o inline:"foo: https://foo.bar/foo\n" \
		pkgconf --with-path=${selfdir}/lib1 --source foo
}

source_empty_body()
{
	atf_check \
		-o inline:"bar: \nfoo: https://foo.bar/foo\n" \
		pkgconf --with-path=${selfdir}/lib1 --source bar
}

modversion_noflatten_body()
{
	atf_check \
		-o inline:"1.3\n" \
		pkgconf --with-path=${selfdir}/lib1 --modversion bar
}

exists_cflags_body()
{
	atf_check \
		-o inline:"-DHAVE_FOO\n" \
		pkgconf --with-path=${selfdir}/lib1 --cflags --exists-cflags --fragment-filter=D foo
}

exists_cflags_env_body()
{
	atf_check \
		-o inline:"FOO_CFLAGS='-DHAVE_FOO'\n" \
		pkgconf --with-path=${selfdir}/lib1 --cflags --exists-cflags --fragment-filter=D --env=FOO foo
}

libs_env_body()
{
	atf_check \
		-o inline:"FOO_LIBS='-L/test/lib -lfoo'\n" \
		pkgconf --with-path=${selfdir}/lib1 --libs --env=FOO foo
}

print_variables_env_body()
{
	atf_check \
		-o inline:"FOO_CFLAGS='-fPIC -I/test/include/foo'\nFOO_LIBS='-L/test/lib -lfoo'\nFOO_INCLUDEDIR='/test/include'\nFOO_LIBDIR='/test/lib'\nFOO_EXEC_PREFIX='/test'\nFOO_PREFIX='/test'\nFOO_PCFILEDIR='${selfdir}/lib1'\n" \
		pkgconf --with-path=${selfdir}/lib1 --env=FOO --print-variables --cflags --libs foo

}

variable_env_body()
{
	atf_check \
		-o inline:"FOO_INCLUDEDIR='/test/include'\n" \
		pkgconf --with-path=${selfdir}/lib1 --env=FOO --variable=includedir foo
}

variable_no_recurse_body()
{
	atf_check \
		-o inline:"/test/include\n" \
		pkgconf --with-path=${selfdir}/lib1 --variable=includedir bar
}

tuple_env_body()
{
	PKG_CONFIG_DUPLICATE_TUPLE_PREFIX=/bar \
	atf_check \
		-o inline:"/bar\n" \
		pkgconf --with-path=${selfdir}/lib1 --variable=prefix duplicate-tuple
}
