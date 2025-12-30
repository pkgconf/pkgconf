#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	libs_intermediary \
	libs_circular_directpc \
	libs_static \
	libs_static_ordering \
	libs_metapackage \
	with_path \
	single_depth_selectors \
	print_variables_env \
	variable_env \
	variable_no_recurse \
	tuple_env

libs_intermediary_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-lintermediary-1 -lintermediary-2 -lfoo -lbar -lbaz\n" \
		pkgconf --libs intermediary-1 intermediary-2
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

with_path_body()
{
	atf_check \
		-o inline:"-L/test/lib -lfoo\n" \
		pkgconf --with-path=${selfdir}/lib1 --with-path=${selfdir}/lib2 --libs foo
	atf_check \
		-o inline:"-L/test/lib -lbar -lfoo\n" \
		pkgconf --with-path=${selfdir}/lib1 --with-path=${selfdir}/lib2 --libs bar
}

single_depth_selectors_body()
{
	export PKG_CONFIG_MAXIMUM_TRAVERSE_DEPTH=1
	atf_check \
		-o inline:"foo\n" \
		pkgconf --with-path=${selfdir}/lib3 --print-requires bar
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
