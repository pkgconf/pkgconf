/*
 * personality.c
 * libpkgconf cross-compile personality database
 *
 * Copyright (c) 2018 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>
#include <libpkgconf/config.h>

static bool default_personality_init = false;
static pkgconf_cross_personality_t default_personality = {
	.name = "default",
};

/*
 * !doc
 *
 * .. c:function:: const pkgconf_cross_personality_t *pkgconf_cross_personality_default(void)
 *
 *    Returns the default cross-compile personality.
 *
 *    :rtype: pkgconf_cross_personality_t*
 *    :return: the default cross-compile personality
 */
const pkgconf_cross_personality_t *
pkgconf_cross_personality_default(void)
{
	if (default_personality_init)
		return &default_personality;

	pkgconf_path_split(SYSTEM_LIBDIR, &default_personality.filter_libdirs, true);
	pkgconf_path_split(SYSTEM_INCLUDEDIR, &default_personality.filter_includedirs, true);

	default_personality_init = true;
	return &default_personality;
}
