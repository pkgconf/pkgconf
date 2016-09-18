#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	dos \
	no_trailing_newline \
	argv_parse \
	bad_option \
	argv_parse_3 \
	tilde_quoting \
	paren_quoting \
	multiline_field \
	quoted

dos_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L/test/lib/dos-lineendings -ldos-lineendings  \n" \
		pkgconf --libs dos-lineendings
}

no_trailing_newline_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-I/test/include/no-trailing-newline  \n" \
		pkgconf --cflags no-trailing-newline
}

argv_parse_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-llib-3 -llib-1 -llib-2 -lpthread  \n" \
		pkgconf --libs argv-parse
}

bad_option_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-e ignore \
		-s eq:1 \
		pkgconf --exists -foo
}

argv_parse_3_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-llib-1 -pthread /test/lib/lib2.so  \n" \
		pkgconf --libs argv-parse-3
}

tilde_quoting_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L~ -ltilde  \n" \
		pkgconf --libs tilde-quoting
	atf_check \
		-o inline:"-I~  \n" \
		pkgconf --cflags tilde-quoting
}

paren_quoting_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-L\$(libdir) -ltilde  \n" \
		pkgconf --libs paren-quoting
}

multiline_field_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-e ignore \
		-o match:"multiline description" \
		pkgconf --list-all
}

quoted_body()
{
	export PKG_CONFIG_PATH="${selfdir}/lib1"
	atf_check \
		-o inline:"-DQUOTED=\\\"bla\\\"  \n" \
		pkgconf --cflags quotes
}
