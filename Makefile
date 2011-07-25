PROG		= pkgconf${PROG_SUFFIX}
SRCS		= main.c parse.c pkg.c

include buildsys.mk

LIBS		= -lpopt

install-extra:
	${LN_S} -f ${bindir}/pkgconf ${DESTDIR}/${bindir}/pkg-config

include .deps
