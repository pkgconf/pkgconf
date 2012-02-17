PROG		= pkgconf${PROG_SUFFIX}
SRCS		= main.c parse.c pkg.c

include buildsys.mk

CFLAGS		+= -DPKG_DEFAULT_PATH=\"${libdir}/pkgconfig\"
LIBS		= -lpopt

include .deps
