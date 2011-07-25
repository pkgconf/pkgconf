PROG		= pkgconf${PROG_SUFFIX}
SRCS		= parse.c pkg.c
LIBS		= -lpopt

include buildsys.mk

install-extra:
	${LN} ${bindir}/pkgconf ${DESTDIR}/${bindir}/pkg-config

include .deps
