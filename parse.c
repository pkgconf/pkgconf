/*
 * parse.c
 * Parser for .pc file.
 *
 * Copyright (c) 2011, 2012 William Pitcock <nenolod@dereferenced.org>.
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

/*
 * parse_file(filename)
 *
 * Parse a .pc file into a pkg_t object structure.
 */
pkg_t *
pkg_new_from_file(const char *filename, FILE *f)
{
	pkg_t *pkg;
	char readbuf[BUFSIZ];

	pkg = calloc(sizeof(pkg_t), 1);
	pkg->filename = strdup(filename);

	while (pkg_fgetline(readbuf, BUFSIZ, f) != NULL)
	{
		char op, *p, *key = NULL, *value = NULL;

		readbuf[strlen(readbuf) - 1] = '\0';

		p = readbuf;
		while (*p && (isalpha(*p) || isdigit(*p) || *p == '_' || *p == '.'))
			p++;

		key = strndup(readbuf, p - readbuf);
		if (!isalpha(*key) && !isdigit(*p))
			goto cleanup;

		while (*p && isspace(*p))
			p++;

		op = *p++;

		while (*p && isspace(*p))
			p++;

		value = strdup(p);

		switch (op)
		{
		case ':':
			if (!strcasecmp(key, "Name"))
				pkg->realname = pkg_tuple_parse(pkg->vars, value);
			else if (!strcasecmp(key, "Description"))
				pkg->description = pkg_tuple_parse(pkg->vars, value);
			else if (!strcasecmp(key, "Version"))
				pkg->version = pkg_tuple_parse(pkg->vars, value);
			else if (!strcasecmp(key, "CFLAGS"))
				pkg->cflags = pkg_fragment_parse(pkg->cflags, pkg->vars, value);
			else if (!strcasecmp(key, "LIBS"))
				pkg->libs = pkg_fragment_parse(pkg->libs, pkg->vars, value);
			else if (!strcasecmp(key, "LIBS.private"))
				pkg->libs_private = pkg_fragment_parse(pkg->libs_private, pkg->vars, value);
			else if (!strcasecmp(key, "Requires"))
				pkg->requires = pkg_dependency_parse(pkg, value);
			else if (!strcasecmp(key, "Requires.private"))
				pkg->requires_private = pkg_dependency_parse(pkg, value);
			else if (!strcasecmp(key, "Conflicts"))
				pkg->conflicts = pkg_dependency_parse(pkg, value);
			break;
		case '=':
			pkg->vars = pkg_tuple_add(pkg->vars, key, value);
			break;
		default:
			break;
		}

cleanup:
		if (key)
			free(key);

		if (value)
			free(value);
	}

	fclose(f);
	return pkg;
}
