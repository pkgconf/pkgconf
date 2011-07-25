PROG		= pkgconf${PROG_SUFFIX}
SRCS		= main.c parse.c pkg.c
LIBS		= -lpopt

include buildsys.mk

install-extra:
	${LN} ${bindir}/pkgconf ${DESTDIR}/${bindir}/pkg-config

include .deps
