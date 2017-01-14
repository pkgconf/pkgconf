/*
 * fragment.c
 * Management of fragment lists.
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

#include <libpkgconf/libpkgconf.h>

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
	char *token;
	size_t len;
};

static inline bool
pkgconf_fragment_is_unmergeable(const char *string)
{
	static struct pkgconf_fragment_check check_fragments[] = {
		{"-framework", 10},
		{"-isystem", 8},
		{"-idirafter", 10},
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
pkgconf_fragment_should_munge(const char *string, const char *sysroot_dir)
{
	static struct pkgconf_fragment_check check_fragments[] = {
		{"-isystem", 8},
		{"-idirafter", 10},
	};

	if (*string != '/')
		return false;

	if (sysroot_dir != NULL && strncmp(sysroot_dir, string, strlen(sysroot_dir)))
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

static inline void
pkgconf_fragment_munge(const pkgconf_client_t *client, char *buf, size_t buflen, const char *source, const char *sysroot_dir)
{
	*buf = '\0';

	if (sysroot_dir == NULL)
		sysroot_dir = pkgconf_tuple_find_global(client, "pc_sysrootdir");

	if (sysroot_dir != NULL && pkgconf_fragment_should_munge(source, sysroot_dir))
		pkgconf_strlcat(buf, sysroot_dir, buflen);

	pkgconf_strlcat(buf, source, buflen);
}

static inline char *
pkgconf_fragment_copy_munged(const pkgconf_client_t *client, const char *source)
{
	char mungebuf[PKGCONF_BUFSIZE];

	if (client->sysroot_dir == NULL)
		return strdup(source);

	pkgconf_fragment_munge(client, mungebuf, sizeof mungebuf, source, client->sysroot_dir);

	return strdup(mungebuf);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_add(const pkgconf_client_t *client, pkgconf_list_t *list, const char *string)
 *
 *    Adds a `fragment` of text to a `fragment list`, possibly modifying the fragment if a sysroot is set.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_list_t* list: The fragment list.
 *    :param char* string: The string of text to add as a fragment to the fragment list.
 *    :return: nothing
 */
void
pkgconf_fragment_add(const pkgconf_client_t *client, pkgconf_list_t *list, const char *string)
{
	pkgconf_fragment_t *frag;

	if (*string == '\0')
		return;

	if (!pkgconf_fragment_is_special(string))
	{
		frag = calloc(sizeof(pkgconf_fragment_t), 1);

		frag->type = *(string + 1);
		frag->data = pkgconf_fragment_copy_munged(client, string + 2);
	}
	else
	{
		char mungebuf[PKGCONF_BUFSIZE];

		if (list->tail != NULL && list->tail->data != NULL)
		{
			pkgconf_fragment_t *parent = list->tail->data;

			/* only attempt to merge 'special' fragments together */
			if (!parent->type && pkgconf_fragment_is_unmergeable(parent->data))
			{
				size_t len;
				char *newdata;

				pkgconf_fragment_munge(client, mungebuf, sizeof mungebuf, string, NULL);

				len = strlen(parent->data) + strlen(mungebuf) + 2;
				newdata = malloc(len);

				pkgconf_strlcpy(newdata, parent->data, len);
				pkgconf_strlcat(newdata, " ", len);
				pkgconf_strlcat(newdata, mungebuf, len);

				free(parent->data);
				parent->data = newdata;

				/* use a copy operation to force a dedup */
				pkgconf_node_delete(&parent->iter, list);
				pkgconf_fragment_copy(list, parent, 0, false);

				/* the fragment list now (maybe) has the copied node, so free the original */
				free(parent->data);
				free(parent);

				return;
			}
		}

		frag = calloc(sizeof(pkgconf_fragment_t), 1);

		frag->type = 0;
		frag->data = strdup(string);
	}

	pkgconf_node_insert_tail(&frag->iter, frag, list);
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

		if (!strcmp(base->data, frag->data))
			return frag;
	}

	return NULL;
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

	return pkgconf_fragment_is_unmergeable(base->data);
}

static inline pkgconf_fragment_t *
pkgconf_fragment_exists(pkgconf_list_t *list, const pkgconf_fragment_t *base, unsigned int flags, bool is_private)
{
	if (!pkgconf_fragment_can_merge_back(base, flags, is_private))
		return NULL;

	if (!pkgconf_fragment_can_merge(base, flags, is_private))
		return NULL;

	return pkgconf_fragment_lookup(list, base);
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

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_copy(pkgconf_list_t *list, const pkgconf_fragment_t *base, unsigned int flags, bool is_private)
 *
 *    Copies a `fragment` to another `fragment list`, possibly removing a previous copy of the `fragment`
 *    in a process known as `mergeback`.
 *
 *    :param pkgconf_list_t* list: The list the fragment is being added to.
 *    :param pkgconf_fragment_t* base: The fragment being copied.
 *    :param uint flags: A set of dependency resolver flags.
 *    :param bool is_private: Whether the fragment list is a `private` fragment list (static linking).
 *    :return: nothing
 */
void
pkgconf_fragment_copy(pkgconf_list_t *list, const pkgconf_fragment_t *base, unsigned int flags, bool is_private)
{
	pkgconf_fragment_t *frag;

	if ((frag = pkgconf_fragment_exists(list, base, flags, is_private)) != NULL)
	{
		if (pkgconf_fragment_should_merge(frag))
			pkgconf_fragment_delete(list, frag);
	}
	else if (!is_private && !pkgconf_fragment_can_merge_back(base, flags, is_private) && (pkgconf_fragment_lookup(list, base) != NULL))
		return;

	frag = calloc(sizeof(pkgconf_fragment_t), 1);

	frag->type = base->type;
	frag->data = strdup(base->data);

	pkgconf_node_insert_tail(&frag->iter, frag, list);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_filter(const pkgconf_client_t *client, pkgconf_list_t *dest, pkgconf_list_t *src, pkgconf_fragment_filter_func_t filter_func, unsigned int flags)
 *
 *    Copies a `fragment list` to another `fragment list` which match a user-specified filtering function.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_list_t* dest: The destination list.
 *    :param pkgconf_list_t* src: The source list.
 *    :param pkgconf_fragment_filter_func_t filter_func: The filter function to use.
 *    :param uint flags: A set of dependency resolver flags.
 *    :return: nothing
 */
void
pkgconf_fragment_filter(const pkgconf_client_t *client, pkgconf_list_t *dest, pkgconf_list_t *src, pkgconf_fragment_filter_func_t filter_func, unsigned int flags)
{
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(src->head, node)
	{
		pkgconf_fragment_t *frag = node->data;

		if (filter_func(client, frag, flags))
			pkgconf_fragment_copy(dest, frag, flags, true);
	}
}

static inline size_t
pkgconf_fragment_len(const pkgconf_fragment_t *frag)
{
	size_t len = 1;

	if (frag->type)
		len += 2;

	if (frag->data != NULL)
		len += strlen(frag->data);

	return len;
}

/*
 * !doc
 *
 * .. c:function:: size_t pkgconf_fragment_render_len(const pkgconf_list_t *list)
 *
 *    Calculates the required memory to store a `fragment list` when rendered as a string.
 *
 *    :param pkgconf_list_t* list: The `fragment list` being rendered.
 *    :return: the amount of bytes required to represent the `fragment list` when rendered
 *    :rtype: size_t
 */
size_t
pkgconf_fragment_render_len(const pkgconf_list_t *list)
{
	size_t out = 1;		/* trailing nul */
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(list->head, node)
	{
		const pkgconf_fragment_t *frag = node->data;
		out += pkgconf_fragment_len(frag);
	}

	return out;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_render_buf(const pkgconf_list_t *list, char *buf, size_t buflen)
 *
 *    Renders a `fragment list` into a buffer.
 *
 *    :param pkgconf_list_t* list: The `fragment list` being rendered.
 *    :param char* buf: The buffer to render the fragment list into.
 *    :param size_t buflen: The length of the buffer.
 *    :return: nothing
 */
void
pkgconf_fragment_render_buf(const pkgconf_list_t *list, char *buf, size_t buflen)
{
	pkgconf_node_t *node;
	char *bptr = buf;

	memset(buf, 0, buflen);

	PKGCONF_FOREACH_LIST_ENTRY(list->head, node)
	{
		const pkgconf_fragment_t *frag = node->data;
		size_t buf_remaining = buflen - (bptr - buf);

		if (pkgconf_fragment_len(frag) > buf_remaining)
			break;

		if (frag->type)
		{
			*bptr++ = '-';
			*bptr++ = frag->type;
		}

		if (frag->data)
			bptr += pkgconf_strlcpy(bptr, frag->data, buf_remaining);

		*bptr++ = ' ';
	}

	*bptr = '\0';
}

/*
 * !doc
 *
 * .. c:function:: char *pkgconf_fragment_render(const pkgconf_list_t *list)
 *
 *    Allocate memory and render a `fragment list` into it.
 *
 *    :param pkgconf_list_t* list: The `fragment list` being rendered.
 *    :return: An allocated string containing the rendered `fragment list`.
 *    :rtype: char *
 */
char *
pkgconf_fragment_render(const pkgconf_list_t *list)
{
	size_t buflen = pkgconf_fragment_render_len(list);
	char *buf = calloc(1, buflen);

	pkgconf_fragment_render_buf(list, buf, buflen);

	return buf;
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

		free(frag->data);
		free(frag);
	}
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_fragment_parse(const pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_list_t *vars, const char *value)
 *
 *    Parse a string into a `fragment list`.
 *
 *    :param pkgconf_client_t* client: The pkgconf client being accessed.
 *    :param pkgconf_list_t* list: The `fragment list` to add the fragment entries to.
 *    :param pkgconf_list_t* vars: A list of variables to use for variable substitution.
 *    :param char* value: The string to parse into fragments.
 *    :return: nothing
 */
void
pkgconf_fragment_parse(const pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_list_t *vars, const char *value)
{
	int i, argc;
	char **argv;
	char *repstr = pkgconf_tuple_parse(client, vars, value);

	pkgconf_argv_split(repstr, &argc, &argv);

	for (i = 0; i < argc; i++)
		pkgconf_fragment_add(client, list, argv[i]);

	pkgconf_argv_free(argv);
	free(repstr);
}
