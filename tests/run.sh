#!/bin/sh
# Tests for pkg-config compliance.
# * Copyright (c) 2012 Michał Górny <mgorny@gentoo.org>.

done=0
failed=0

run_test() {
	local res t_ret 2>/dev/null || true

	echo "$ ${1}"
	eval res="\$(${1})"
	echo "${res}"

	t_ret=0
	while [ ${#} -gt 1 ]; do
		shift

		case "${res}" in
			*${1}*)
				;;
			*)
				echo "! expected ${1}"
				t_ret=1
				;;
		esac
	done

	if [ ${t_ret} -eq 0 ]; then
		echo "+ [OK]"
	else
		failed=$(( failed + 1 ))
	fi
	done=$(( done + 1 ))

	echo
}

selfdir=$(cd "$(dirname "${0}")"; pwd)

# 1) overall 'is it working?' test
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --libs foo" \
	'-lfoo'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --cflags --libs foo" \
	'-lfoo' '-I/usr/include/foo' '-fPIC'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --exists 'foo > 1.2'; echo \$?" \
	'0'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --exists 'foo > 1.2.3'; echo \$?" \
	'1'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --exists 'foo' '>' '1.2'; echo \$?" \
	'0'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --uninstalled 'foo'; echo \$?" \
	'1'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --uninstalled 'omg'; echo \$?" \
	'0'
run_test "${1} ${selfdir}/lib1/foo.pc --modversion" \
	'1.2.3'

# 2) tests for PKG_CONFIG_PATH order
run_test "PKG_CONFIG_PATH=${selfdir}/lib1:${selfdir}/lib2 ${1} --libs foo" \
	'-lfoo'
run_test "PKG_CONFIG_PATH=${selfdir}/lib2:${selfdir}/lib1 ${1} --libs foo" \
	'-lbar'

# 3) tests for 'Requires' and 'Requires.private'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --libs bar" \
	'-lfoo' '-lbar'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --libs --cflags baz" \
	'-lbaz' '-fPIC' '-I/usr/include/foo'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --static --libs baz" \
	'-lfoo' '-lbaz' '-lzee'

# 4) tests for parser bugs
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --libs dos-lineendings" \
	'-L/usr/lib/dos-lineendings -ldos-lineendings'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --libs argv-parse" \
	'-llib-2 -lpthread'

# 5) tests for other regressions
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --variable=includedir foo" \
	'/usr/include'

# 6) tests for builtins
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --modversion pkg-config" \
	'0.26'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --variable=prefix foo" \
	'/usr'
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 ${1} --define-variable=prefix=/test --variable=prefix foo" \
	'/test'

# 7) tests for env modifiers
run_test "PKG_CONFIG_PATH=${selfdir}/lib1 PKG_CONFIG_SYSROOT_DIR=/test ${1} --cflags baz" \
	'-I/test/usr/include/foo'

if [ ${failed} -gt 0 ]; then
	echo "${failed} of ${done} tests failed. See output for details." >&2
	exit 1
else
	echo "${done} tests done. All succeeded." >&2
	exit 0
fi
