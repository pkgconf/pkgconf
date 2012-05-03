PROG		= pkgconf${PROG_SUFFIX}
SRCS		= main.c parse.c pkg.c bsdstubs.c getopt_long.c

include buildsys.mk

CFLAGS		+= -DPKG_DEFAULT_PATH=\"${libdir}/pkgconfig\" -Wall -Wextra -Wformat=2 -std=gnu99 -D_FORTIFY_SOURCE=2

install-extra:
	mkdir -p $(DESTDIR)/$(datarootdir)/aclocal
	install -c -m644 pkg.m4 $(DESTDIR)/$(datarootdir)/aclocal/pkg.m4

check: $(PROG)
	$(SHELL) tests/run.sh ./$(PROG)

include .deps
