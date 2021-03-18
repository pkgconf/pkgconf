/*
 * fileio.c
 * File reading utilities
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

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>

char *
pkgconf_fgetline(char *line, size_t size, FILE *stream)
{
	char *s = line;
	char *end = line + size - 2;
	bool quoted = false;
	int c = '\0', c2;

	if (s == NULL)
		return NULL;

	while (s < end && (c = getc(stream)) != EOF)
	{
		if (c == '\\' && !quoted)
		{
			quoted = true;
			continue;
		}
		else if (c == '#')
		{
			if (!quoted) {
				/* Skip the rest of the line */
				do {
					c = getc(stream);
				} while (c != '\n' && c != EOF);
				*s++ = c;
				break;
			}
			else
				*s++ = c;

			quoted = false;
			continue;
		}
		else if (c == '\n')
		{
			if (quoted)
			{
				/* Trim spaces */
				do {
					c2 = getc(stream);
				} while (c2 == '\t' || c2 == ' ');

				ungetc(c2, stream);

				quoted = false;
				continue;
			}
			else
			{
				*s++ = c;
			}

			break;
		}
		else if (c == '\r')
		{
			*s++ = '\n';

			if ((c2 = getc(stream)) == '\n')
			{
				if (quoted)
				{
					quoted = false;
					continue;
				}

				break;
			}

			ungetc(c2, stream);

			if (quoted)
			{
				quoted = false;
				continue;
			}

			break;
		}
		else
		{
			if (quoted) {
				*s++ = '\\';
				quoted = false;
			}
			*s++ = c;
		}

	}

	if (c == EOF && (s == line || ferror(stream)))
		return NULL;

	*s = '\0';

	/* Remove newline character. */
	if (s > line && *(--s) == '\n') {
		*s = '\0';

		if (s > line && *(--s) == '\r')
			*s = '\0';
	}

	return line;
}
