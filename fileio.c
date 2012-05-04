/*
 * fileio.c
 * File reading utilities
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

#include "pkg.h"
#include "bsdstubs.h"

char *
pkg_fgetline(char *line, size_t size, FILE *stream)
{
	char *s = line;
	char *end = line + size - 1;
	int c = '\0', c2;

	if (s == NULL)
		return NULL;

	while (s < end && (c = getc(stream)) != EOF)
	{
		if (c == '\n')
		{
			*s++ = c;
			break;
		}
		else if (c == '\r')
		{
			*s++ = '\n';

			if ((c2 = getc(stream)) == '\n')
				break;

			ungetc(c2, stream);
			break;
		}
		else
			*s++ = c;
	}

	*s = '\0';

	if (c == EOF && (s == line || ferror(stream)))
		return NULL;

	return line;
}
