#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	baz \
	quux \
	moo \
	meow

baz_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o ignore \
		pkgconf --libs provides-test-baz
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-baz = 1.1.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-baz >= 1.1.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-baz <= 1.1.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-baz != 1.1.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-baz != 1.0.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-baz > 1.1.1'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-baz > 1.1.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-baz < 1.1.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-baz < 1.2.0'
}

quux_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o ignore \
		pkgconf --libs provides-test-quux
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-quux = 1.1.9'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-quux >= 1.1.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-quux >= 1.1.9'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-quux >= 1.2.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-quux <= 1.2.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-quux <= 1.1.9'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-quux != 1.2.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-quux != 1.1.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-quux != 1.0.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-quux > 1.1.9'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-quux > 1.2.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-quux < 1.1.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-quux > 1.2.0'
}

moo_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o ignore \
		pkgconf --libs provides-test-moo
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-moo = 1.2.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-moo >= 1.1.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-moo >= 1.2.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-moo >= 1.2.1'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-moo <= 1.2.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-moo != 1.1.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-moo != 1.0.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-moo > 1.1.9'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-moo > 1.2.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-moo < 1.1.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-moo < 1.2.0'
}

meow_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o ignore \
		pkgconf --libs provides-test-meow
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-meow = 1.3.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-meow != 1.3.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-meow > 1.2.9'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-meow < 1.3.1'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-meow < 1.3.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-meow > 1.3.0'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-meow >= 1.3.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-meow >= 1.3.1'
	atf_check \
		-s exit:1 \
		-e ignore \
		-o ignore \
		pkgconf --libs 'provides-test-meow <= 1.3.0'
	atf_check \
		-o ignore \
		pkgconf --libs 'provides-test-meow < 1.2.9'
}
