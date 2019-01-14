#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	libs \
	libs_cflags \
	libs_static \
	libs_static_pure \
	argv_parse2 \
	static_cflags \
	private_duplication \
	libs_static2 \
	missing \
	requires_internal \
	requires_internal_missing \
	requires_internal_collision \
	orphaned_requires_private

libs_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/lib -lbar -lfoo \n" \
		pkgconf --libs bar
}

libs_cflags_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-fPIC -I/test/include/foo -L/test/lib -lbaz \n" \
		pkgconf --libs --cflags baz
}

libs_static_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/lib -lbaz -L/test/lib -lzee -L/test/lib -lfoo \n" \
		pkgconf --static --libs baz
}

libs_static_pure_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/lib -lbaz -L/test/lib -lfoo \n" \
		pkgconf --static --pure --libs baz
}

argv_parse2_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-llib-1 -pthread /test/lib/lib2.so \n" \
		pkgconf --static --libs argv-parse-2
}

static_cflags_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-fPIC -I/test/include/foo -DFOO_STATIC \n" \
		pkgconf --static --cflags baz
}

private_duplication_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-lprivate -lfoo -lbaz -lzee -lbar -lfoo \n" \
		pkgconf --static --libs-only-l private-libs-duplication
}

libs_static2_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-lbar -lbar-private -L/test/lib -lfoo \n" \
		pkgconf --static --libs static-libs
}

missing_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-s exit:1 \
		-e ignore \
		-o inline:"\n" \
		pkgconf --cflags missing-require
}

requires_internal_body()
{
	atf_check \
		-o inline:"-lbar -lbar-private -L/test/lib -lfoo \n" \
		pkgconf --with-path="${selfdir}/lib1" --static --libs requires-internal
}

requires_internal_missing_body()
{
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --with-path="${selfdir}/lib1" --static --libs requires-internal-missing
}

requires_internal_collision_body()
{
	atf_check \
		-o inline:"-I/test/local/include/foo \n" \
		pkgconf --with-path="${selfdir}/lib1" --cflags requires-internal-collision
}

orphaned_requires_private_body()
{
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --with-path="${selfdir}/lib1" --cflags --libs orphaned-requires-private
}
