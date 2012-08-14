/*
 * argvsplit.c
 * argv_split() routine
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

#include "config.h"
#include "pkg.h"

void
pkg_argv_free(char **argv)
{
	free(argv[0]);
	free(argv);
}

int
pkg_argv_split(const char *src, int *argc, char ***argv)
{
	char *buf = malloc(strlen(src) + 1);
	const char *src_iter;
	char *dst_iter;
	int argc_count = 0;
	int argv_size = 5;
	char quote = 0;

	src_iter = src;
	dst_iter = buf;

	memset(buf, 0, strlen(src) + 1);

	*argv = calloc(sizeof (void *), argv_size);
	(*argv)[argc_count] = dst_iter;

	while (*src_iter)
	{
		if (quote == *src_iter)
			quote = 0;
		else if (quote)
		{
			if (*src_iter == '\\')
			{
				src_iter++;
				if (!*src_iter)
				{
					free(*argv);
					free(buf);
					return -1;
				}

				if (*src_iter != quote)
					*dst_iter++ = '\\';
			}
			*dst_iter++ = *src_iter;
		}
		else if (isspace(*src_iter))
		{
			if ((*argv)[argc_count] != NULL)
			{
				argc_count++, dst_iter++;

				if (argc_count == argv_size)
				{
					argv_size += 5;
					*argv = realloc(*argv, sizeof(void *) * argv_size);
				}

				(*argv)[argc_count] = dst_iter;
			}
		}
		else switch(*src_iter)
		{
			case '"':
			case '\'':
				quote = *src_iter;
				break;

			case '\\':
				src_iter++;

				if (!*src_iter)
				{
					free(argv);
					free(buf);
					return -1;
				} else {
					*dst_iter++ = '\\';
				}
			default:
				*dst_iter++ = *src_iter;
				break;
		}

		src_iter++;
	}

	if (strlen((*argv)[argc_count])) {
		argc_count++;
	}

	*argc = argc_count;
	return 0;
}
