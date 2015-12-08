libdir                 = @libdir@
datarootdir            = @datarootdir@
datadir                = @datadir@
includedir             = @includedir@
system_includedir      = @SYSTEM_INCLUDEDIR@
system_libdir          = @SYSTEM_LIBDIR@
pkgconfigdir           = @PKGCONFIGDIR@

AM_CFLAGS = -Wall -Wextra -Wformat=2 -std=gnu99

bin_PROGRAMS = pkgconf
lib_LTLIBRARIES = libpkgconf.la

EXTRA_DIST =	\
		pkg.m4 \
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
		tests/lib1/typelibdir.pc \
		tests/lib2/foo.pc \
		tests/lib1/comments.pc \
		tests/lib1/nolib.pc \
		tests/lib3/bar.pc \
		tests/lib1/conflicts.pc \
		tests/lib1/omg-uninstalled.pc

pkginclude_HEADERS = libpkgconf/bsdstubs.h libpkgconf/iter.h libpkgconf/libpkgconf.h libpkgconf/stdinc.h
libpkgconf_la_SOURCES  =	\
		libpkgconf/cache.c		\
		libpkgconf/pkg.c		\
		libpkgconf/bsdstubs.c		\
		libpkgconf/fragment.c		\
		libpkgconf/argvsplit.c		\
		libpkgconf/fileio.c		\
		libpkgconf/tuple.c		\
		libpkgconf/dependency.c		\
		libpkgconf/queue.c
libpkgconf_la_LDFLAGS = -version-info 1:0:0
# -export-symbols-regex '^pkgconf_'

dist_man_MANS    = pkgconf.1
pkgconf_LDADD    = libpkgconf.la
pkgconf_SOURCES  = main.c getopt_long.c
noinst_HEADERS   = getopt_long.h

dist_doc_DATA = README.md AUTHORS

m4datadir              = $(datadir)/aclocal
m4data_DATA            = pkg.m4

CLEANFILES = $(EXTRA_PROGRAMS)

check: pkgconf
	$(SHELL) tests/run.sh ./pkgconf
