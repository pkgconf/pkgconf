AC_PREREQ([2.68])
AC_INIT([pkgconf], [0.8.1], [http://github.com/nenolod/pkgconf/issues])
AC_CONFIG_SRCDIR([pkg.c])
AC_CONFIG_HEADERS([config.h])
AC_CHECK_FUNCS([strlcpy strlcat strndup])

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

AC_PROG_CPP
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

BUILDSYS_SHARED_LIB
BUILDSYS_PROG_IMPLIB
BUILDSYS_TOUCH_DEPS

AC_CONFIG_FILES([buildsys.mk tests/run.sh])
AC_OUTPUT
