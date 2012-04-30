PROG		= pkgconf${PROG_SUFFIX}
SRCS		= main.c parse.c pkg.c

include buildsys.mk

CFLAGS		+= -DPKG_DEFAULT_PATH=\"${libdir}/pkgconfig\"
LIBS		= -lpopt

install-extra:
	mkdir -p $(DESTDIR)/$(datarootdir)/aclocal
	install -c -m644 pkg.m4 $(DESTDIR)/$(datarootdir)/aclocal/pkg.m4

include .deps
