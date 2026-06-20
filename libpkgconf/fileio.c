/*
 * fileio.c
 * File reading utilities
 *
 * SPDX-License-Identifier: pkgconf
 *
 * Copyright (c) 2012, 2025 pkgconf authors (see AUTHORS).
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

#define push_or_return_fail(buf, c) \
	do { if (!pkgconf_buffer_push_byte((buf), (char) (c))) return false; } while (0)

#define trim_or_return_fail(buf) \
	do { if (!pkgconf_buffer_trim_byte((buf))) return false; } while (0)

bool
pkgconf_fgetline(pkgconf_buffer_t *buffer, FILE *stream)
{
	bool quoted = false;
	bool got_data = false;
	char in[PKGCONF_ITEM_SIZE];

	while (fgets(in, sizeof in, stream) != NULL)
	{
		char *p = in;

		got_data = true;

		while (*p != '\0')
		{
			unsigned char c = (unsigned char) *p++;

			if (c == '\\' && !quoted)
			{
				quoted = true;
				continue;
			}
			else if (c == '\n')
			{
				if (quoted)
				{
					quoted = false;
					continue;
				}
				else
					push_or_return_fail(buffer, (char) c);

				goto done;
			}
			else if (c == '\r')
			{
				if (*p == '\n')
					p++;

				if (quoted)
				{
					quoted = false;
					continue;
				}

				push_or_return_fail(buffer, '\n');
				goto done;
			}
			else
			{
				if (quoted)
				{
					push_or_return_fail(buffer, '\\');
					quoted = false;
				}

				push_or_return_fail(buffer, (char) c);
			}
		}
	}

done:
	/* Remove newline character. */
	if (pkgconf_buffer_lastc(buffer) == '\n')
		trim_or_return_fail(buffer);

	if (pkgconf_buffer_lastc(buffer) == '\r')
		trim_or_return_fail(buffer);

	if (!got_data)
		return false;

	return !ferror(stream);
}
