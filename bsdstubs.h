/*
 * bsdstubs.h
 * Header for stub BSD function prototypes if unavailable on a specific platform.
 *
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include "config.h"
#include "getopt_long.h"

#ifndef __BSDSTUBS_H__
#define __BSDSTUBS_H__

#ifndef HAVE_STRLCPY
extern size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_STRLCAT
extern size_t strlcat(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_STRNDUP
extern char *strndup(const char *src, size_t len);
#endif

#ifndef HAVE_STRTOK_R
extern char *strtok_r(char *s, const char *sep, char **p);
#endif

#endif
