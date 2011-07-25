PROG		= pkgconf${PROG_SUFFIX}
SRCS		= main.c parse.c pkg.c

include buildsys.mk

LIBS		= -lpopt

install-extra:
	${LN} ${bindir}/pkgconf ${DESTDIR}/${bindir}/pkg-config

include .deps
