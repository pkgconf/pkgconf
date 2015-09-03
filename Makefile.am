libdir                 = @libdir@
datarootdir            = @datarootdir@
datadir                = @datadir@
includedir             = @includedir@
system_includedir      = @SYSTEM_INCLUDEDIR@
system_libdir          = @SYSTEM_LIBDIR@
pkgconfigdir           = @PKGCONFIGDIR@

CFLAGS = -Wall -Wextra -Wformat=2 -std=gnu99

bin_PROGRAMS = pkgconf

pkgconf_SOURCES  = main.c cache.c pkg.c bsdstubs.c getopt_long.c fragment.c argvsplit.c fileio.c tuple.c dependency.c queue.c
pkgconf_CPPFLAGS = -DLIBDIR=\"${libdir}\" -DINCLUDEDIR=\"${includedir}\" -DPKG_DEFAULT_PATH=\"${pkgconfigdir}\" -DSYSTEM_INCLUDEDIR=\"${system_includedir}\" -DSYSTEM_LIBDIR=\"${system_libdir}\"
pkgconf_MANS     = pkgconf.1

dist_doc_DATA = README.md AUTHORS

check: pkgconf
	$(SHELL) tests/run.sh ./pkgconf
