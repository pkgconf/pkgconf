/*
 * stdinc.h
 * pull in standard headers (including portability hacks)
 *
 * Copyright (c) 2012 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#ifndef __STDINC_H
#define __STDINC_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <malloc.h>
# define PATH_DEV_NULL	"nul"
# ifdef _WIN64
#  define SIZE_FMT_SPECIFIER	"%llu"
# else
#  define SIZE_FMT_SPECIFIER	"%lu"
# endif
# ifndef ssize_t
# ifndef __MINGW32__
#  include <BaseTsd.h>
# else
#  include <basetsd.h>
# endif
#  define ssize_t SSIZE_T
# endif
# ifndef __MINGW32__
#  include "win-dirent.h"
# else
# include <dirent.h>
# endif
#else
# define PATH_DEV_NULL	"/dev/null"
# define SIZE_FMT_SPECIFIER	"%zu"
# include <dirent.h>
# include <unistd.h>
#endif

#endif
