/*
 * fragment.c
 * Management of fragment lists.
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

pkg_fragment_t *
pkg_fragment_append(pkg_fragment_t *head, pkg_fragment_t *tail)
{
	pkg_fragment_t *node;

	if (head == NULL)
		return tail;

	/* skip to end of list */
	PKG_FOREACH_LIST_ENTRY(head, node)
	{
		if (node->next == NULL)
			break;
	}

	node->next = tail;
	tail->prev = node;

	return head;
}

pkg_fragment_t *
pkg_fragment_add(pkg_fragment_t *head, const char *string)
{
	pkg_fragment_t *frag;

	frag = calloc(sizeof(pkg_fragment_t), 1);

	if (*string == '-' && strncmp(string, "-lib:", 5))
	{
		frag->type = *(string + 1);
		frag->data = strdup(string + 2);
	}
	else
	{
		frag->type = 0;
		frag->data = strdup(string);
	}

	return pkg_fragment_append(head, frag);
}

bool
pkg_fragment_exists(pkg_fragment_t *head, pkg_fragment_t *base)
{
	pkg_fragment_t *node;

	PKG_FOREACH_LIST_ENTRY(head, node)
	{
		if (base->type != node->type)
			continue;

		if (!strcmp(base->data, node->data))
			return true;
	}

	return false;
}

pkg_fragment_t *
pkg_fragment_copy(pkg_fragment_t *head, pkg_fragment_t *base)
{
	pkg_fragment_t *frag;

	if (pkg_fragment_exists(head, base))
		return head;

	frag = calloc(sizeof(pkg_fragment_t), 1);

	frag->type = base->type;
	frag->data = strdup(base->data);

	return pkg_fragment_append(head, frag);
}

void
pkg_fragment_delete(pkg_fragment_t *node)
{
	if (node->prev != NULL)
		node->prev->next = node->next;

	if (node->next != NULL)
		node->next->prev = node->prev;

	free(node->data);
	free(node);
}

void
pkg_fragment_free(pkg_fragment_t *head)
{
	pkg_fragment_t *node, *next;

	PKG_FOREACH_LIST_ENTRY_SAFE(head, next, node)
		pkg_fragment_delete(node);
}

pkg_fragment_t *
pkg_fragment_parse(pkg_fragment_t *head, pkg_tuple_t *vars, const char *value)
{
	int i, argc;
	char **argv;
	char *repstr = pkg_tuple_parse(vars, value);

	pkg_argv_split(repstr, &argc, &argv);

	for (i = 0; i < argc; i++)
		head = pkg_fragment_add(head, argv[i]);

	pkg_argv_free(argv);
	free(repstr);

	return head;
}
