/*
 * renderer-msvc.c
 * MSVC library syntax renderer
 *
 * Copyright (c) 2017 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include <string.h>
#include <stdlib.h>

#include <libpkgconf/libpkgconf.h>
#include "renderer-msvc.h"

static inline char *
fragment_escape(const char *src)
{
	ssize_t outlen = strlen(src) + 10;
	char *out = calloc(outlen, 1);
	char *dst = out;

	while (*src)
	{
		if (((*src < ' ') ||
		    (*src > ' ' && *src < '$') ||
		    (*src > '$' && *src < '(') ||
		    (*src > ')' && *src < '+') ||
		    (*src > ':' && *src < '=') ||
		    (*src > '=' && *src < '@') ||
		    (*src > 'Z' && *src < '^') ||
		    (*src == '`') ||
		    (*src > 'z' && *src < '~') ||
		    (*src > '~')) && *src != '\\')
			*dst++ = '\\';

		*dst++ = *src++;

		if ((ptrdiff_t)(dst - out) + 2 > outlen)
		{
			outlen *= 2;
			out = realloc(out, outlen);
		}
	}

	*dst = 0;
	return out;
}

static inline size_t
fragment_len(const pkgconf_fragment_t *frag, bool escape)
{
	size_t len = 1;

	if (!escape)
		len += strlen(frag->data);
	else
	{
		char *tmp = fragment_escape(frag->data);
		len += strlen(tmp);
		free(tmp);
	}

	return len;
}

static inline bool
allowed_fragment(const pkgconf_fragment_t *frag)
{
	return !(!frag->type || frag->data == NULL || strchr("Ll", frag->type) == NULL);
}

static size_t
msvc_renderer_render_len(const pkgconf_list_t *list, bool escape)
{
	size_t out = 1;		/* trailing nul */
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(list->head, node)
	{
		const pkgconf_fragment_t *frag = node->data;

		if (!allowed_fragment(frag))
			continue;

		switch (frag->type)
		{
			case 'L':
				out += 9; /* "/libpath:" */
				break;
			case 'l':
				out += 4; /* ".lib" */
				break;
			default:
				break;
		}

		out += fragment_len(frag, escape);
	}

	return out;
}

static void
msvc_renderer_render_buf(const pkgconf_list_t *list, char *buf, size_t buflen, bool escape)
{
	pkgconf_node_t *node;
	char *bptr = buf;

	memset(buf, 0, buflen);

	PKGCONF_FOREACH_LIST_ENTRY(list->head, node)
	{
		const pkgconf_fragment_t *frag = node->data;
		size_t buf_remaining = buflen - (bptr - buf);

		if (!allowed_fragment(frag))
			continue;

		if (fragment_len(frag, escape) > buf_remaining)
			break;

		if (frag->type == 'L')
		{
			size_t cnt = pkgconf_strlcpy(bptr, "/libpath:", buf_remaining);
			bptr += cnt;
			buf_remaining -= cnt;
		}

		if (!escape)
		{
			size_t cnt = pkgconf_strlcpy(bptr, frag->data, buf_remaining);
			bptr += cnt;
			buf_remaining -= cnt;
		}
		else
		{
			char *tmp = fragment_escape(frag->data);
			size_t cnt = pkgconf_strlcpy(bptr, tmp, buf_remaining);
			free(tmp);

			bptr += cnt;
			buf_remaining -= cnt;
		}

		if (frag->type == 'l')
		{
			size_t cnt = pkgconf_strlcpy(bptr, ".lib", buf_remaining);
			bptr += cnt;
			buf_remaining -= cnt;
		}

		*bptr++ = ' ';
	}

	*bptr = '\0';
}

static const pkgconf_fragment_render_ops_t msvc_renderer_ops = {
	.render_len = msvc_renderer_render_len,
	.render_buf = msvc_renderer_render_buf
};

const pkgconf_fragment_render_ops_t *
msvc_renderer_get(void)
{
	return &msvc_renderer_ops;
}
