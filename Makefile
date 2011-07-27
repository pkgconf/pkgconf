PROG		= pkgconf${PROG_SUFFIX}
SRCS		= main.c parse.c pkg.c

include buildsys.mk

LIBS		= -lpopt

include .deps
