libdir                 = @libdir@
datarootdir            = @datarootdir@
datadir                = @datadir@
includedir             = @includedir@
system_includedir      = @SYSTEM_INCLUDEDIR@
system_libdir          = @SYSTEM_LIBDIR@
pkgconfigdir           = @PKGCONFIGDIR@

CFLAGS = -Wall -Wextra -Wformat=2 -std=gnu99

bin_PROGRAMS = pkgconf

EXTRA_DIST =	\
		tests/lib1/argv-parse-2.pc \
		tests/lib1/dos-lineendings.pc \
		tests/lib1/paren-quoting.pc \
		tests/lib1/argv-parse-3.pc \
		tests/lib1/foo.pc \
		tests/lib1/prefix-foo1.pc \
		tests/lib1/argv-parse.pc \
		tests/lib1/framework-1.pc \
		tests/lib1/prefix-foo2.pc \
		tests/lib1/bar.pc \
		tests/lib1/framework-2.pc \
		tests/lib1/private-libs-duplication.pc \
		tests/lib1/baz.pc \
		tests/lib1/incomplete.pc \
		tests/lib1/quotes.pc \
		tests/lib1/case-sensitivity.pc \
		tests/lib1/intermediary-1.pc \
		tests/lib1/static-archive-libs.pc \
		tests/lib1/cflags-libs-only.pc \
		tests/lib1/intermediary-2.pc \
		tests/lib1/static-libs.pc \
		tests/lib1/circular-1.pc \
		tests/lib1/missing-require.pc \
		tests/lib1/sysroot-dir.pc \
		tests/lib1/circular-2.pc \
		tests/lib1/multiline.pc \
		tests/lib1/tilde-quoting.pc \
		tests/lib1/circular-3.pc \
		tests/lib1/no-trailing-newline.pc \
		tests/lib1/tilde.pc \
		tests/lib1/comments-in-fields.pc \
		tests/lib1/nocflag.pc \
		tests/lib2/foo.pc \
		tests/lib1/comments.pc \
		tests/lib1/nolib.pc \
		tests/lib3/bar.pc \
		tests/lib1/conflicts.pc \
		tests/lib1/omg-uninstalled.pc

noinst_HEADERS = bsdstubs.h getopt_long.h iter.h pkg.h stdinc.h
pkgconf_SOURCES  = main.c cache.c pkg.c bsdstubs.c getopt_long.c fragment.c argvsplit.c fileio.c tuple.c dependency.c queue.c
pkgconf_CPPFLAGS = -DLIBDIR=\"${libdir}\" -DINCLUDEDIR=\"${includedir}\" -DPKG_DEFAULT_PATH=\"${pkgconfigdir}\" -DSYSTEM_INCLUDEDIR=\"${system_includedir}\" -DSYSTEM_LIBDIR=\"${system_libdir}\"
pkgconf_MANS     = pkgconf.1

dist_doc_DATA = README.md AUTHORS

check: pkgconf
	$(SHELL) tests/run.sh ./pkgconf
