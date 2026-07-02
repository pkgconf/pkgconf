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

bool
pkgconf_fgetline(pkgconf_buffer_t *buffer, FILE *stream)
{
	bool quoted = false;
	bool got_data = false;
	int c;

	while ((c = getc(stream)) != EOF)
	{
		got_data = true;

		if (c == '\\' && !quoted)
		{
			quoted = true;
			continue;
		}

		if (c == '\n')
		{
			if (quoted)
			{
				quoted = false;
				continue;
			}

			break;
		}

		if (c == '\r')
		{
			int next = getc(stream);

			if (next != '\n' && next != EOF && ungetc(next, stream) == EOF)
				return false;

			if (quoted)
			{
				quoted = false;
				continue;
			}

			break;
		}

		if (quoted)
		{
			push_or_return_fail(buffer, '\\');
			quoted = false;
		}

		push_or_return_fail(buffer, c);
	}

	return got_data && !ferror(stream);
}
