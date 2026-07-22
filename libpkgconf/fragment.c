/*
 * fragment.c
 * Management of fragment lists.
 *
 * SPDX-License-Identifier: pkgconf
 *
 * Copyright (c) 2012, 2013, 2014 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include <libpkgconf/config.h>
#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>

#ifndef _WIN32
#include <locale.h>
#include <langinfo.h>
#endif

#ifdef __APPLE__
#include <xlocale.h>
#endif

/*
 * !doc
 *
 * libpkgconf `fragment` module
 * ============================
 *
 * The `fragment` module provides low-level management and rendering of fragment lists.  A
 * `fragment list` contains various `fragments` of text (such as ``-I /usr/include``) in a matter
 * which is composable, mergeable and reorderable.
 */

struct pkgconf_fragment_check {
	const char *token;
	size_t len;
};

static inline bool
pkgconf_fragment_is_greedy(const char *string)
{
	static const struct pkgconf_fragment_check check_fragments[] = {
		{"-F", 2},
		{"-I", 2},
		{"-L", 2},
		{"-D", 2},
		{"-l", 2},
	};

	if (*string != '-')
		return false;

	for (size_t i = 0; i < PKGCONF_ARRAY_SIZE(check_fragments); i++)
		if (!strncmp(string, check_fragments[i].token, check_fragments[i].len))
		{
			/* if it is the bare flag, then we want the next token to be the data */
			if (!*(string + check_fragments[i].len))
				return true;
		}

	return false;
}

static inline bool
pkgconf_fragment_should_check_sysroot(const char *string)
{
	static const struct pkgconf_fragment_check check_fragments[] = {
		{"-F", 2},
		{"-I", 2},
		{"-L", 2},
		{"-isystem", 8},
		{"-idirafter", 10},
	};

	if (*string != '-')
		return false;

	for (size_t i = 0; i < PKGCONF_ARRAY_SIZE(check_fragments); i++)
		if (!strncmp(string, check_fragments[i].token, check_fragments[i].len))
			return true;

	return false;
}

static inline bool
pkgconf_fragment_is_unmergeable(const char *string)
{
	static const struct pkgconf_fragment_check check_fragments[] = {
		{"-framework", 10},
		{"-isystem", 8},
		{"-idirafter", 10},
		{"-pthread", 8},
		{"-Wa,", 4},
		{"-Wl,", 4},
		{"-Wp,", 4},
		{"-trigraphs", 10},
		{"-pedantic", 9},
		{"-ansi", 5},
		{"-std=", 5},
		{"-stdlib=", 8},
		{"-include", 8},
		{"-nostdinc", 9},
		{"-nostdlibinc", 12},
		{"-nobuiltininc", 13},
		{"-nodefaultlibs", 14},
	};

	if (*string != '-')
		return true;

	for (size_t i = 0; i < PKGCONF_ARRAY_SIZE(check_fragments); i++)
		if (!strncmp(string, check_fragments[i].token, check_fragments[i].len))
			return true;

	/* only one pair of {-flag, arg} may be merged together */
	if (strchr(string, ' ') != NULL)
		return false;

	return false;
}

static inline bool
pkgconf_fragment_only_group_one(const char *string)
{
	static const struct pkgconf_fragment_check check_fragments[] = {
		{"-framework", 10},
		{"-isystem", 8},
		{"-idirafter", 10},
		{"-include", 8},
	};

	for (size_t i = 0; i < PKGCONF_ARRAY_SIZE(check_fragments); i++)
		if (!strncmp(string, check_fragments[i].token, check_fragments[i].len))
			return true;

	return false;
}

static inline bool
pkgconf_fragment_is_groupable(const char *string)
{
	static const struct pkgconf_fragment_check check_fragments[] = {
		{"-Wl,--start-group", 17},
	};

	if (pkgconf_fragment_only_group_one(string))
		return true;

	for (size_t i = 0; i < PKGCONF_ARRAY_SIZE(check_fragments); i++)
		if (!strncmp(string, check_fragments[i].token, check_fragments[i].len))
			return true;

	return false;
}

static inline bool
pkgconf_fragment_is_terminus(const char *parent, const char *string)
{
	static const struct pkgconf_fragment_check check_fragments[] = {
		{"-Wl,--end-group", 15},
	};

	if (pkgconf_fragment_only_group_one(parent))
		return true;

	for (size_t i = 0; i < PKGCONF_ARRAY_SIZE(check_fragments); i++)
		if (!strncmp(string, check_fragments[i].token, check_fragments[i].len))
			return true;

	return false;
}

static inline bool
pkgconf_fragment_is_special(const char *string)
{
	if (*string != '-')
		return true;

	if (!strncmp(string, "-lib:", 5))
		return true;

	return pkgconf_fragment_is_unmergeable(string);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_insert(const pkgconf_client_t *client, pkgconf_list_t *list, char type, const char *data, bool tail)
 *
 *    Adds a `fragment` of text to a `fragment list` directly without interpreting it.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_list_t* list: The fragment list.
 *    :param char type: The type of the fragment.
 *    :param char* data: The data of the fragment.
 *    :param bool tail: Whether to place the fragment at the beginning of the list or the end.
 *    :return: nothing
 */
void
pkgconf_fragment_insert(pkgconf_client_t *client, pkgconf_list_t *list, char type, const char *data, bool tail)
{
	(void) client;

	pkgconf_fragment_t *frag;

	frag = calloc(1, sizeof(pkgconf_fragment_t));
	if (frag == NULL)
		return;

	frag->type = type;
	if (data != NULL)
	{
		frag->data = strdup(data);
		if (frag->data == NULL)
		{
			free(frag);
			return;
		}
	}

	if (tail)
	{
		pkgconf_node_insert_tail(&frag->iter, frag, list);
		return;
	}

	pkgconf_node_insert(&frag->iter, frag, list);
}

static bool
should_inject_sysroot(const pkgconf_client_t *client, const char *string, bool saw_sysroot, unsigned int flags)
{
	if (client->flags & PKGCONF_PKG_PKGF_NO_SYSROOT_INJECTION)
		return false;

	/* we never automatically inject sysroot on -uninstalled packages */
	if (flags & PKGCONF_PKG_PROPF_UNINSTALLED)
	{
		/* ... unless we are emulating pkgconf 1.x */
		if (!(client->flags & PKGCONF_PKG_PKGF_PKGCONF1_SYSROOT_RULES))
			return false;
	}

	if (client->sysroot_dir == NULL)
		return false;

	if (saw_sysroot)
		return false;

	if (!pkgconf_fragment_should_check_sysroot(string))
		return false;

	if (!strncmp(string + 2, client->sysroot_dir, strlen(client->sysroot_dir)) &&
		*(string + 2 + strlen(client->sysroot_dir)) == '/')
	{
		return false;
	}

	return true;
}

static bool
should_inject_sysroot_child(const pkgconf_client_t *client, const pkgconf_fragment_t *last, const char *string, bool saw_sysroot, unsigned int flags)
{
	if (client->flags & PKGCONF_PKG_PKGF_NO_SYSROOT_INJECTION)
		return false;

	/* we never automatically inject sysroot on -uninstalled packages */
	if (flags & PKGCONF_PKG_PROPF_UNINSTALLED)
	{
		/* ... unless we are emulating pkgconf 1.x */
		if (!(client->flags & PKGCONF_PKG_PKGF_PKGCONF1_SYSROOT_RULES))
			return false;
	}

	if (last->type)
		return false;

	if (last->data == NULL)
		return false;

	if (client->sysroot_dir == NULL)
		return false;

	if (saw_sysroot)
		return false;

	if (!pkgconf_fragment_should_check_sysroot(last->data))
		return false;

	if (!strncmp(string, client->sysroot_dir, strlen(client->sysroot_dir)) &&
		*(string + strlen(client->sysroot_dir)) == '/')
	{
		return false;
	}

	return true;
}

static inline bool
fragment_is_unquoted_var(const char *value)
{
	size_t len;

	if (value == NULL)
		return false;

	len = strlen(value);

	if (len < 4 || value[0] != '$')
		return false;

	if (value[1] == '{' && value[len - 1] == '}')
		return true;

	return false;
}

/*
 * Insert an already-expanded fragment string into the list.  No variable
 * substitution is performed here: `string` is taken verbatim, so this must
 * only be called with input that has already been through the bytecode
 * evaluator.  `saw_sysroot` records whether that prior evaluation expanded
 * ${pc_sysrootdir}, which governs whether sysroot injection is still needed.
 */
static bool
fragment_insert_evaluated(pkgconf_client_t *client, pkgconf_list_t *list, const char *string, bool saw_sysroot, unsigned int flags)
{
	pkgconf_list_t *target = list;
	pkgconf_fragment_t *terminate_parent = NULL;
	pkgconf_fragment_t *frag;

	if (string == NULL || *string == '\0')
		return true;

	if (list->tail != NULL && list->tail->data != NULL &&
		!(client->flags & PKGCONF_PKG_PKGF_DONT_MERGE_SPECIAL_FRAGMENTS))
	{
		pkgconf_fragment_t *parent = list->tail->data;

		/* only attempt to merge 'special' fragments together */
		if (!parent->type && parent->data != NULL &&
			pkgconf_fragment_is_unmergeable(parent->data) &&
			!(parent->flags & PKGCONF_PKG_FRAGF_TERMINATED))
		{
			if (pkgconf_fragment_is_groupable(parent->data))
				target = &parent->children;

			if (pkgconf_fragment_is_terminus(parent->data, string))
				terminate_parent = parent;

			PKGCONF_TRACE(client, "adding fragment as child to list @%p", target);
		}
	}

	frag = calloc(1, sizeof(pkgconf_fragment_t));
	if (frag == NULL)
	{
		PKGCONF_TRACE(client, "failed to add new fragment due to allocation failure to list @%p", target);
		return false;
	}

	if (strlen(string) > 1 && !pkgconf_fragment_is_special(string))
	{
		frag->type = *(string + 1);

		if (should_inject_sysroot(client, string, saw_sysroot, flags))
		{
			pkgconf_buffer_t sysroot_buf = PKGCONF_BUFFER_INITIALIZER;

			if (!pkgconf_buffer_append(&sysroot_buf, client->sysroot_dir) ||
				!pkgconf_buffer_append(&sysroot_buf, string + 2))
			{
				pkgconf_buffer_finalize(&sysroot_buf);
				free(frag);
				return false;
			}

			frag->data = pkgconf_buffer_freeze(&sysroot_buf);
		}
		else
			frag->data = strdup(string + 2);
	}
	else
	{
		frag->type = 0;

		if (client->sysroot_dir != NULL && list->tail != NULL && list->tail->data != NULL &&
			should_inject_sysroot_child(client, list->tail->data, string, saw_sysroot, flags))
		{
			pkgconf_buffer_t sysroot_buf = PKGCONF_BUFFER_INITIALIZER;

			if (pkgconf_buffer_append(&sysroot_buf, client->sysroot_dir) &&
				pkgconf_buffer_append(&sysroot_buf, string))
				frag->data = pkgconf_buffer_freeze(&sysroot_buf);
			else
				pkgconf_buffer_finalize(&sysroot_buf);
		}
		else
			frag->data = strdup(string);
	}

	if (frag->data == NULL)
	{
		free(frag);
		return false;
	}

	if (frag->type)
		PKGCONF_TRACE(client, "added fragment {%c, '%s'} to list @%p", frag->type, frag->data, list);
	else
		PKGCONF_TRACE(client, "created special fragment {'%s'} in list @%p", frag->data, target);

	pkgconf_node_insert_tail(&frag->iter, frag, target);
	if (terminate_parent != NULL)
		terminate_parent->flags |= PKGCONF_PKG_FRAGF_TERMINATED;

	return true;
}

/*
 * Split a string into whitespace-delimited fragments, honouring shell quoting
 * and greedy flags (e.g. "-I /usr/include" -> "-I/usr/include").
 *
 * When `evaluate` is true, each resulting token is passed through
 * pkgconf_fragment_add so that variable references are expanded.  When it is
 * false, `value` is assumed to already be fully expanded and the tokens are
 * inserted verbatim: this is the path used to re-split a single variable whose
 * value expands to several fragments, and it must not evaluate again lest an
 * expansion that yields a literal "${...}" (via a "$$" escape) recurse forever.
 */
static bool
fragment_split(pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_list_t *vars, const char *value, unsigned int flags, bool evaluate)
{
	int i, ret, argc;
	char **argv;

	ret = pkgconf_argv_split(value, &argc, &argv);
	if (ret < 0)
	{
		PKGCONF_TRACE(client, "unable to parse fragment string [%s]", value);
		return false;
	}

	for (i = 0; i < argc; i++)
	{
		const char *token;
		pkgconf_buffer_t greedybuf = PKGCONF_BUFFER_INITIALIZER;
		bool ok;

		if (argv[i] == NULL)
		{
			PKGCONF_TRACE(client, "parsed fragment string is inconsistent: argc = %d while argv[%d] == NULL", argc, i);
			pkgconf_argv_free(argv);
			return false;
		}

		token = argv[i];

		PKGCONF_TRACE(client, "processing [%s]", argv[i]);

		if (pkgconf_fragment_is_greedy(argv[i]) && i + 1 < argc)
		{
			if (!pkgconf_buffer_append(&greedybuf, argv[i]) ||
				!pkgconf_buffer_append(&greedybuf, argv[i + 1]))
			{
				pkgconf_buffer_finalize(&greedybuf);
				pkgconf_argv_free(argv);
				return false;
			}

			token = pkgconf_buffer_str(&greedybuf);

			/* skip over next arg as we combined them */
			i++;
		}

		if (evaluate)
			ok = pkgconf_fragment_add(client, list, vars, token, flags);
		else
			ok = fragment_insert_evaluated(client, list, token, false, flags);

		pkgconf_buffer_finalize(&greedybuf);

		if (!ok)
		{
			pkgconf_argv_free(argv);
			return false;
		}
	}

	pkgconf_argv_free(argv);

	return true;
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_fragment_add(const pkgconf_client_t *client, pkgconf_list_t *list, const char *string, unsigned int flags)
 *
 *    Adds a `fragment` of text to a `fragment list`, possibly modifying the fragment if a sysroot is set.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_list_t* list: The fragment list.
 *    :param char* string: The string of text to add as a fragment to the fragment list.
 *    :param uint flags: Parsing-related flags for the package.
 *    :return: true on success, false on parse error or allocation failure
 */
bool
pkgconf_fragment_add(pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_list_t *vars, const char *value, unsigned int flags)
{
	pkgconf_buffer_t evalbuf = PKGCONF_BUFFER_INITIALIZER;
	bool saw_sysroot = false;
	char *string;
	bool ret;

	if (!pkgconf_bytecode_eval_str_to_buf(client, vars, value, &saw_sysroot, &evalbuf))
	{
		pkgconf_buffer_finalize(&evalbuf);
		return false;
	}

	if (pkgconf_buffer_len(&evalbuf) == 0)
	{
		pkgconf_buffer_finalize(&evalbuf);
		return true;
	}

	string = pkgconf_buffer_freeze(&evalbuf);
	if (string == NULL)
		return false;

	/* A bare "${var}" may expand to several whitespace-separated fragments, so
	 * re-split the (already-expanded) result.  The split must not evaluate the
	 * tokens again, otherwise a value that expands to a literal "${var}" would
	 * recurse without bound.
	 */
	if (fragment_is_unquoted_var(value))
		ret = fragment_split(client, list, vars, string, flags, false);
	else
		ret = fragment_insert_evaluated(client, list, string, saw_sysroot, flags);

	free(string);
	return ret;
}

static inline pkgconf_fragment_t *
pkgconf_fragment_lookup(pkgconf_list_t *list, const pkgconf_fragment_t *base)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY_REVERSE(list->tail, node)
	{
		pkgconf_fragment_t *frag = node->data;

		if (base->type != frag->type)
			continue;

		if (base->data == NULL || frag->data == NULL)
		{
			if (base->data == frag->data)
				return frag;

			continue;
		}

		if (!strcmp(base->data, frag->data))
			return frag;
	}

	return NULL;
}

/* Order fragments by (type, data) so that the cursor index can be searched
 * with bsearch().  NULL data sorts before any non-NULL data of the same type. */
static int
fragment_index_cmp(const void *a, const void *b)
{
	const pkgconf_fragment_t *fa = *(pkgconf_fragment_t * const *) a;
	const pkgconf_fragment_t *fb = *(pkgconf_fragment_t * const *) b;

	if (fa->type != fb->type)
		return (unsigned char) fa->type < (unsigned char) fb->type ? -1 : 1;

	if (fa->data == NULL || fb->data == NULL)
	{
		if (fa->data == fb->data)
			return 0;

		return fa->data == NULL ? -1 : 1;
	}

	return strcmp(fa->data, fb->data);
}

static pkgconf_fragment_t *
fragment_index_lookup(const pkgconf_fragment_cursor_t *cursor, const pkgconf_fragment_t *base)
{
	pkgconf_fragment_t **found;

	if (cursor->count == 0)
		return NULL;

	found = bsearch(&base, cursor->index, cursor->count, sizeof(void *), fragment_index_cmp);
	return found != NULL ? *found : NULL;
}

static bool
fragment_index_insert(pkgconf_fragment_cursor_t *cursor, pkgconf_fragment_t *frag)
{
	size_t lo = 0, hi = cursor->count;

	if (cursor->count == cursor->alloc)
	{
		size_t newalloc = cursor->alloc != 0 ? cursor->alloc * 2 : 16;
		pkgconf_fragment_t **newindex = pkgconf_reallocarray(cursor->index, newalloc, sizeof(void *));

		if (newindex == NULL)
			return false;

		cursor->index = newindex;
		cursor->alloc = newalloc;
	}

	while (lo < hi)
	{
		size_t mid = lo + (hi - lo) / 2;

		if (fragment_index_cmp(&cursor->index[mid], &frag) < 0)
			lo = mid + 1;
		else
			hi = mid;
	}

	memmove(&cursor->index[lo + 1], &cursor->index[lo], (cursor->count - lo) * sizeof(void *));
	cursor->index[lo] = frag;
	cursor->count++;

	return true;
}

static void
fragment_index_remove(pkgconf_fragment_cursor_t *cursor, const pkgconf_fragment_t *frag)
{
	pkgconf_fragment_t **found = bsearch(&frag, cursor->index, cursor->count, sizeof(void *), fragment_index_cmp);
	size_t idx;

	if (found == NULL)
		return;

	idx = (size_t) (found - cursor->index);
	memmove(&cursor->index[idx], &cursor->index[idx + 1], (cursor->count - idx - 1) * sizeof(void *));
	cursor->count--;
}

/* Look up an existing fragment matching `base`: via the cursor's sorted index
 * when one is provided, otherwise a linear scan of the list. */
static inline pkgconf_fragment_t *
fragment_lookup(pkgconf_list_t *list, const pkgconf_fragment_cursor_t *cursor, const pkgconf_fragment_t *base)
{
	if (cursor != NULL)
		return fragment_index_lookup(cursor, base);

	return pkgconf_fragment_lookup(list, base);
}

static inline bool
pkgconf_fragment_can_merge_back(const pkgconf_fragment_t *base, unsigned int flags, bool is_private)
{
	(void) flags;

	if (base->type == 'l')
	{
		if (is_private)
			return false;

		return true;
	}

	if (base->type == 'F')
		return false;
	if (base->type == 'L')
		return false;
	if (base->type == 'I')
		return false;

	return true;
}

static inline bool
pkgconf_fragment_can_merge(const pkgconf_fragment_t *base, unsigned int flags, bool is_private)
{
	(void) flags;

	if (is_private)
		return false;

	if (base->children.head != NULL)
		return false;

	if (base->data == NULL)
		return false;

	return pkgconf_fragment_is_unmergeable(base->data);
}

static inline pkgconf_fragment_t *
pkgconf_fragment_exists(pkgconf_list_t *list, const pkgconf_fragment_cursor_t *cursor, const pkgconf_fragment_t *base, unsigned int flags, bool is_private)
{
	if (!pkgconf_fragment_can_merge_back(base, flags, is_private))
		return NULL;

	if (!pkgconf_fragment_can_merge(base, flags, is_private))
		return NULL;

	return fragment_lookup(list, cursor, base);
}

static inline bool
pkgconf_fragment_should_merge(const pkgconf_fragment_t *base)
{
	const pkgconf_fragment_t *parent;

	/* if we are the first fragment, that means the next fragment is the same, so it's always safe. */
	if (base->iter.prev == NULL)
		return true;

	/* this really shouldn't ever happen, but handle it */
	parent = base->iter.prev->data;
	if (parent == NULL)
		return true;

	switch (parent->type)
	{
	case 'l':
	case 'L':
	case 'I':
		return true;
	default:
		return !base->type || parent->type == base->type;
	}
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_fragment_has_system_dir(const pkgconf_client_t *client, const pkgconf_fragment_t *frag)
 *
 *    Checks if a `fragment` contains a `system path`.  System paths are detected at compile time and optionally overridden by
 *    the ``PKG_CONFIG_SYSTEM_INCLUDE_PATH`` and ``PKG_CONFIG_SYSTEM_LIBRARY_PATH`` environment variables.
 *
 *    :param pkgconf_client_t* client: The pkgconf client object the fragment belongs to.
 *    :param pkgconf_fragment_t* frag: The fragment being checked.
 *    :return: true if the fragment contains a system path, else false
 *    :rtype: bool
 */
bool
pkgconf_fragment_has_system_dir(const pkgconf_client_t *client, const pkgconf_fragment_t *frag)
{
	const pkgconf_list_t *check_paths = NULL;

	switch (frag->type)
	{
	case 'L':
		check_paths = &client->filter_libdirs;
		break;
	case 'I':
		check_paths = &client->filter_includedirs;
		break;
	default:
		return false;
	}

	return pkgconf_path_match_list(frag->data, check_paths);
}

static bool
pkgconf_fragment_copy_node(const pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_fragment_cursor_t *cursor, const pkgconf_fragment_t *base, bool is_private);

static bool
pkgconf_fragment_copy_list_node(const pkgconf_client_t *client, pkgconf_list_t *list, const pkgconf_list_t *base)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(base->head, node)
	{
		pkgconf_fragment_t *frag = node->data;

		/* children are copied into their own (small) list, which is never
		 * indexed, so no cursor is threaded down here. */
		if (!pkgconf_fragment_copy_node(client, list, NULL, frag, true))
			return false;
	}

	return true;
}

static bool
pkgconf_fragment_copy_node(const pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_fragment_cursor_t *cursor, const pkgconf_fragment_t *base, bool is_private)
{
	pkgconf_fragment_t *old_frag = NULL;
	pkgconf_fragment_t *frag;

	if ((old_frag = pkgconf_fragment_exists(list, cursor, base, client->flags, is_private)) != NULL)
	{
		if (!pkgconf_fragment_should_merge(old_frag))
			old_frag = NULL;
	}
	else if (!is_private && !pkgconf_fragment_can_merge_back(base, client->flags, is_private) && (fragment_lookup(list, cursor, base) != NULL))
		return true;

	frag = calloc(1, sizeof(pkgconf_fragment_t));
	if (frag == NULL)
		return false;

	frag->type = base->type;
	if (!pkgconf_fragment_copy_list_node(client, &frag->children, &base->children))
	{
		pkgconf_fragment_free(&frag->children);
		free(frag);
		return false;
	}

	if (base->data != NULL)
	{
		frag->data = strdup(base->data);
		if (frag->data == NULL)
		{
			pkgconf_fragment_free(&frag->children);
			free(frag);
			return false;
		}
	}

	if (old_frag != NULL)
	{
		if (cursor != NULL)
			fragment_index_remove(cursor, old_frag);

		pkgconf_fragment_delete(list, old_frag);
	}

	pkgconf_node_insert_tail(&frag->iter, frag, list);

	if (cursor != NULL && !fragment_index_insert(cursor, frag))
		return false;

	return true;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_copy(const pkgconf_client_t *client, pkgconf_list_t *list, const pkgconf_fragment_t *base, bool is_private)
 *
 *    Copies a `fragment` to another `fragment list`, possibly removing a previous copy of the `fragment`
 *    in a process known as `mergeback`.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_list_t* list: The list the fragment is being added to.
 *    :param pkgconf_fragment_t* base: The fragment being copied.
 *    :param bool is_private: Whether the fragment list is a `private` fragment list (static linking).
 *    :return: nothing
 */
void
pkgconf_fragment_copy(const pkgconf_client_t *client, pkgconf_list_t *list, const pkgconf_fragment_t *base, bool is_private)
{
	(void) pkgconf_fragment_copy_node(client, list, NULL, base, is_private);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_cursor_init(pkgconf_fragment_cursor_t *cursor, pkgconf_list_t *list)
 *
 *    Initialises a `fragment cursor` bound to a (typically empty) destination list.  While the
 *    cursor is in use, fragments must be added to the list only via ``pkgconf_fragment_copy_cursor()``
 *    so that the cursor's index stays in sync.
 *
 *    :param pkgconf_fragment_cursor_t* cursor: The cursor to initialise.
 *    :param pkgconf_list_t* list: The destination fragment list.
 *    :return: nothing
 */
void
pkgconf_fragment_cursor_init(pkgconf_fragment_cursor_t *cursor, pkgconf_list_t *list)
{
	pkgconf_node_t *node;

	cursor->list = list;
	cursor->index = NULL;
	cursor->count = 0;
	cursor->alloc = 0;

	/* seed the index with anything already present, so dedup against pre-existing
	 * fragments behaves exactly as the linear scan would. */
	PKGCONF_FOREACH_LIST_ENTRY(list->head, node)
		(void) fragment_index_insert(cursor, node->data);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_cursor_deinit(pkgconf_fragment_cursor_t *cursor)
 *
 *    Releases the index held by a `fragment cursor`.  The destination list and its fragments are
 *    not affected.
 *
 *    :param pkgconf_fragment_cursor_t* cursor: The cursor to release.
 *    :return: nothing
 */
void
pkgconf_fragment_cursor_deinit(pkgconf_fragment_cursor_t *cursor)
{
	free(cursor->index);
	cursor->index = NULL;
	cursor->count = 0;
	cursor->alloc = 0;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_copy_cursor(const pkgconf_client_t *client, pkgconf_fragment_cursor_t *cursor, const pkgconf_fragment_t *base, bool is_private)
 *
 *    Like ``pkgconf_fragment_copy()``, but uses the cursor's sorted index for the mergeback lookup,
 *    turning what would be a linear scan of the destination list into a bsearch.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_fragment_cursor_t* cursor: The cursor bound to the destination list.
 *    :param pkgconf_fragment_t* base: The fragment being copied.
 *    :param bool is_private: Whether the fragment list is a `private` fragment list (static linking).
 *    :return: nothing
 */
void
pkgconf_fragment_copy_cursor(const pkgconf_client_t *client, pkgconf_fragment_cursor_t *cursor, const pkgconf_fragment_t *base, bool is_private)
{
	(void) pkgconf_fragment_copy_node(client, cursor->list, cursor, base, is_private);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_copy_list(const pkgconf_client_t *client, pkgconf_list_t *list, const pkgconf_list_t *base)
 *
 *    Copies a `fragment list` to another `fragment list`, possibly removing a previous copy of the fragments
 *    in a process known as `mergeback`.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_list_t* list: The list the fragments are being added to.
 *    :param pkgconf_list_t* base: The list the fragments are being copied from.
 *    :return: nothing
 */
void
pkgconf_fragment_copy_list(const pkgconf_client_t *client, pkgconf_list_t *list, const pkgconf_list_t *base)
{
	(void) pkgconf_fragment_copy_list_node(client, list, base);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_filter(const pkgconf_client_t *client, pkgconf_list_t *dest, pkgconf_list_t *src, pkgconf_fragment_filter_func_t filter_func)
 *
 *    Copies a `fragment list` to another `fragment list` which match a user-specified filtering function.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_list_t* dest: The destination list.
 *    :param pkgconf_list_t* src: The source list.
 *    :param pkgconf_fragment_filter_func_t filter_func: The filter function to use.
 *    :param void* data: Optional data to pass to the filter function.
 *    :return: nothing
 */
void
pkgconf_fragment_filter(const pkgconf_client_t *client, pkgconf_list_t *dest, pkgconf_list_t *src, pkgconf_fragment_filter_func_t filter_func, void *data)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(src->head, node)
	{
		pkgconf_fragment_t *frag = node->data;

		if (filter_func(client, frag, data))
			(void) pkgconf_fragment_copy_node(client, dest, NULL, frag, true);
	}
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_is_locale_utf8(void)
 *
 *    Check whether text is expected to be UTF-8 encoded in the current environment.
 *
 *    :return: :code:`true` if text is expected to be UTF-8 encoded, :code:`false` otherwise.
 */
#ifndef _WIN32
static bool
codeset_is_utf8(const char *codeset)
{
	return codeset != NULL && (!strcasecmp(codeset, "UTF-8") || !strcasecmp(codeset, "UTF8"));
}
#endif

bool
pkgconf_is_locale_utf8(void)
{
#ifdef _WIN32
	return GetACP() == CP_UTF8;
#else
	static int cached = -1;

	if (cached >= 0)
		return cached;

#if HAVE_DECL_NL_LANGINFO_L
	locale_t loc = newlocale(LC_CTYPE_MASK, "", (locale_t)0);

	if (loc != (locale_t)0)
	{
		cached = codeset_is_utf8(nl_langinfo_l(CODESET, loc));
		freelocale(loc);
	}
	else
		cached = 0;
#else
	const char *prev_locale = setlocale(LC_CTYPE, NULL);
	char *saved_locale = prev_locale != NULL ? strdup(prev_locale) : NULL;

	setlocale(LC_CTYPE, "");
	cached = codeset_is_utf8(nl_langinfo(CODESET));
	setlocale(LC_CTYPE, saved_locale != NULL ? saved_locale : "C");
	free(saved_locale);
#endif

	return cached;
#endif
}

static bool
fragment_quote(pkgconf_buffer_t *out, const pkgconf_fragment_t *frag)
{
	if (frag->data == NULL)
		return true;

	const pkgconf_buffer_t *src = PKGCONF_BUFFER_FROM_STR(frag->data);
	const pkgconf_span_t quote_spans[] = {
		{ 0x00, 0x1f },
		{ (unsigned char)' ', (unsigned char)'#' },
		{ (unsigned char)'%', (unsigned char)'\'' },
		{ (unsigned char)'*', (unsigned char)'*' },
		{ (unsigned char)';', (unsigned char)'<' },
		{ (unsigned char)'>', (unsigned char)'?' },
		{ (unsigned char)'[', (unsigned char)']' },
		{ (unsigned char)'`', (unsigned char)'`' },
		{ (unsigned char)'{', (unsigned char)'}' },
		{ 0x7f, 0xff },
	};

	/* If the local is UTF-8 we must not split character over 0x7f because it would add "\" between each bytes.
	   So only DEL (0x7f) needs escaping */
	const pkgconf_span_t quote_spans_utf8[] = {
		{ 0x00, 0x1f },
		{ (unsigned char)' ', (unsigned char)'#' },
		{ (unsigned char)'%', (unsigned char)'\'' },
		{ (unsigned char)'*', (unsigned char)'*' },
		{ (unsigned char)';', (unsigned char)'<' },
		{ (unsigned char)'>', (unsigned char)'?' },
		{ (unsigned char)'[', (unsigned char)']' },
		{ (unsigned char)'`', (unsigned char)'`' },
		{ (unsigned char)'{', (unsigned char)'}' },
		{ 0x7f, 0x7f },
	};

	if (pkgconf_is_locale_utf8())
		return pkgconf_buffer_escape(out, src, quote_spans_utf8, PKGCONF_ARRAY_SIZE(quote_spans_utf8));

	return pkgconf_buffer_escape(out, src, quote_spans, PKGCONF_ARRAY_SIZE(quote_spans));
}

static bool
fragment_render(const pkgconf_fragment_render_ctx_t *ctx, const pkgconf_fragment_t *frag, pkgconf_buffer_t *buf)
{
	const pkgconf_node_t *iter;
	pkgconf_buffer_t quoted = PKGCONF_BUFFER_INITIALIZER;

	if (!fragment_quote(&quoted, frag))
		goto fail;

	if (frag->type &&
		(!pkgconf_buffer_push_byte(buf, '-') || !pkgconf_buffer_push_byte(buf, frag->type)))
		goto fail;

	if (!pkgconf_buffer_append(buf, pkgconf_buffer_str_or_empty(&quoted)))
		goto fail;

	pkgconf_buffer_finalize(&quoted);

	PKGCONF_FOREACH_LIST_ENTRY(frag->children.head, iter)
	{
		const pkgconf_fragment_t *child_frag = iter->data;

		if (!pkgconf_buffer_push_byte(buf, ctx->delim) ||
			!fragment_render(ctx, child_frag, buf))
			return false;
	}

	return true;

fail:
	pkgconf_buffer_finalize(&quoted);
	return false;
}

static const pkgconf_fragment_render_ops_t default_render_ops = {
	.render = fragment_render
};

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_render_buf(const pkgconf_list_t *list, char *buf, size_t buflen, bool escape, const pkgconf_fragment_render_ops_t *ops, char delim)
 *
 *    Renders a `fragment list` into a buffer.
 *
 *    :param pkgconf_list_t* list: The `fragment list` being rendered.
 *    :param pkgconf_buffer_t* buf: The buffer to render the fragment list into.
 *    :param bool escape: Whether or not to escape special shell characters (deprecated).
 *    :param pkgconf_fragment_render_ops_t* ops: An optional ops structure to use for custom renderers, else ``NULL``.
 *    :param char delim: The delimiter to use between fragments.
 *    :return: nothing
 */
bool
pkgconf_fragment_render_buf(const pkgconf_list_t *list, pkgconf_buffer_t *buf, bool escape, const pkgconf_fragment_render_ops_t *ops, char delim)
{
	pkgconf_node_t *node;
	pkgconf_fragment_render_ctx_t ctx = {
		.escape = escape,
		.delim = delim,
	};

	ops = ops != NULL ? ops : &default_render_ops;

	PKGCONF_FOREACH_LIST_ENTRY(list->head, node)
	{
		const pkgconf_fragment_t *frag = node->data;
		if (!ops->render(&ctx, frag, buf))
			return false;

		if (node->next != NULL && !pkgconf_buffer_push_byte(buf, ctx.delim))
			return false;
	}

	return true;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_delete(pkgconf_list_t *list, pkgconf_fragment_t *node)
 *
 *    Delete a `fragment node` from a `fragment list`.
 *
 *    :param pkgconf_list_t* list: The `fragment list` to delete from.
 *    :param pkgconf_fragment_t* node: The `fragment node` to delete.
 *    :return: nothing
 */
void
pkgconf_fragment_delete(pkgconf_list_t *list, pkgconf_fragment_t *node)
{
	pkgconf_node_delete(&node->iter, list);

	pkgconf_fragment_free(&node->children);
	free(node->data);
	free(node);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_free(pkgconf_list_t *list)
 *
 *    Delete an entire `fragment list`.
 *
 *    :param pkgconf_list_t* list: The `fragment list` to delete.
 *    :return: nothing
 */
void
pkgconf_fragment_free(pkgconf_list_t *list)
{
	pkgconf_node_t *node, *next;

	PKGCONF_FOREACH_LIST_ENTRY_SAFE(list->head, next, node)
	{
		pkgconf_fragment_t *frag = node->data;

		pkgconf_fragment_free(&frag->children);
		free(frag->data);
		free(frag);
	}

	pkgconf_list_zero(list);
}

/*
 * !doc
 *
 * .. c:function:: bool pkgconf_fragment_parse(const pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_list_t *vars, const char *value)
 *
 *    Parse a string into a `fragment list`.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_list_t* list: The `fragment list` to add the fragment entries to.
 *    :param pkgconf_list_t* vars: A list of variables to use for variable substitution.
 *    :param uint flags: Any parsing flags to be aware of.
 *    :param char* value: The string to parse into fragments.
 *    :return: true on success, false on parse error
 */
bool
pkgconf_fragment_parse(pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_list_t *vars, const char *value, unsigned int flags)
{
	return fragment_split(client, list, vars, value, flags, true);
}
