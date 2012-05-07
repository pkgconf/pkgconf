/*
 * argvsplit.c
 * argv_split() routine
 *
 * Copyright (c) 2012 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "pkg.h"

void argv_free(char **argv)
{
	free(argv[0]);
	free(argv);
}

int argv_split(const char *src, int *argc, char ***argv)
{
	char *buf = malloc(strlen(src) + 1);
	const char *src_iter;
	char *dst_iter;
	int argc_count = 0;
	int argv_size = 5;
	char quote = 0;

	src_iter = src;
	dst_iter = buf;

	bzero(buf, strlen(src) + 1);

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
			if (*(src_iter + 1) == '-' && (*argv)[argc_count] != NULL)
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
