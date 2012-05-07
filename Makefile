PROG		= pkgconf${PROG_SUFFIX}
SRCS		= main.c parse.c pkg.c bsdstubs.c getopt_long.c fragment.c argvsplit.c fileio.c tuple.c

include buildsys.mk

CFLAGS		+= -DLIBDIR=\"${libdir}\" -DINCLUDEDIR=\"${includedir}\" -DPKG_DEFAULT_PATH=\"${libdir}/pkgconfig:${datadir}/pkgconfig\" -Wall -Wextra -Wformat=2 -std=gnu99 -D_FORTIFY_SOURCE=2

install-extra:
	mkdir -p $(DESTDIR)/$(datarootdir)/aclocal
	install -c -m644 pkg.m4 $(DESTDIR)/$(datarootdir)/aclocal/pkg.m4

check: $(PROG)
	$(SHELL) tests/run.sh ./$(PROG)

valgrind-check: $(PROG)
	$(SHELL) tests/run.sh 'valgrind --leak-check=full --show-reachable=yes ./$(PROG)'

include .deps
