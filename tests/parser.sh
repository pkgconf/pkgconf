#!/usr/bin/env atf-sh

. $(atf_get_srcdir)/test_env.sh

tests_init \
	fragment_tree

fragment_quoting_7a_body()
{
	set -x

	test_cflags=$(pkgconf --with-path=${selfdir}/lib1 --cflags fragment-quoting-7)
	echo $test_cflags
#	test_cflags='-Dhello=10 -Dworld=+32 -DDEFINED_FROM_PKG_CONFIG=hello\\ world'

	cat > test.c <<- __TESTCASE_END__
		int main(int argc, char *argv[]) { return DEFINED_FROM_PKG_CONFIG; }
	__TESTCASE_END__
	cc -o test-fragment-quoting-7 ${test_cflags} ./test.c
	atf_check -e 42 ./test-fragment-quoting-7
	rm -f test.c test-fragment-quoting-7

	set +x
}

fragment_tree_body()
{
	atf_check \
		-o inline:"'-Wl,--start-group' [untyped]
  '-la' [type l]
  '-lb' [type l]
  '-Wl,--end-group' [untyped]

'-nodefaultlibs' [untyped]
'-Wl,--start-group' [untyped]
  '-la' [type l]
  '-lgcc' [type l]
  '-Wl,--end-group' [untyped]

'-Wl,--gc-sections' [untyped]

" \
		pkgconf --with-path="${selfdir}/lib1" --fragment-tree fragment-groups-2
}

fragment_whitespace_body()
{
	atf_check \
		-o inline:"'-I/includedir' [type I]\n\n" \
			pkgconf --with-path="${selfdir}/lib1" --fragment-tree flag-whitespace-2

	atf_check \
		-o inline:"'-I/includedir' [type I]\n\n" \
			pkgconf --with-path="${selfdir}/lib1" --fragment-tree flag-whitespace
}
