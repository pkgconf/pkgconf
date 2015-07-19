dnl configure.ac
dnl m4 preprocessor script for autotools
dnl
dnl Copyright (c) 2011, 2012, 2013, 2014 pkgconf authors (see AUTHORS).
dnl
dnl Permission to use, copy, modify, and/or distribute this software for any
dnl purpose with or without fee is hereby granted, provided that the above
dnl copyright notice and this permission notice appear in all copies.
dnl
dnl This software is provided 'as is' and without any warranty, express or
dnl implied.  In no event shall the authors be liable for any damages arising
dnl from the use of this software.

AC_PREREQ([2.68])
AC_INIT([pkgconf], [0.9.12], [http://github.com/pkgconf/pkgconf/issues])
AC_CONFIG_SRCDIR([pkg.c])
AC_CONFIG_HEADERS([config.h])
AC_CHECK_FUNCS([strlcpy strlcat strndup strtok_r])

AC_SYS_LARGEFILE

AC_ARG_ENABLE([strict],
	      [AC_HELP_STRING([--enable-strict],
			     [enable POSIX-strict argument checking and disable some workarounds])],
	      [ac_cv_use_strict=$enableval], [ac_cv_use_strict=no])
AC_CACHE_CHECK([whether to enable strict mode],
	       [ac_cv_use_strict], [ac_cv_use_strict=no])

if test "x$ac_cv_use_strict" = "xyes"; then
	AC_DEFINE([HAVE_STRICT_MODE], [1], [Define if strict mode is requested.])
	STRICT_MODE=yes
else
	AC_DEFINE([HAVE_STRICT_MODE], [0], [Define if strict mode is requested.])
	STRICT_MODE=no
fi

AC_SUBST([STRICT_MODE])

AC_ARG_WITH([pkg-config-dir],[AC_HELP_STRING([--with-pkg-config-dir],[specify
	     the place where pc files will be found])],PKGCONFIGDIR="$withval",
	     PKGCONFIGDIR="${libdir}/pkgconfig:${datadir}/pkgconfig")

AC_SUBST([PKGCONFIGDIR])

AC_ARG_WITH([system-libdir],[AC_HELP_STRING([--with-system-libdir],[specify the
	     system library directory (default LIBDIR)])],
	     SYSTEM_LIBDIR="$withval", SYSTEM_LIBDIR="${libdir}")

AC_SUBST([SYSTEM_LIBDIR])

AC_ARG_WITH([system-includedir],[AC_HELP_STRING([--with-system-includedir],[specify the
	     system include directory (default INCLUDEDIR)])],
	     SYSTEM_INCLUDEDIR="$withval", SYSTEM_INCLUDEDIR="${includedir}")

AC_SUBST([SYSTEM_INCLUDEDIR])

AC_PROG_CPP
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

AC_CONFIG_FILES([Makefile tests/run.sh])
AC_OUTPUT
