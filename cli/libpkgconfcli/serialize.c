/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2025 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 *​
 *​ Copyright (C) 2026 Elizabeth Ashford.
 */

#include <stdlib.h>
#include <string.h>
#include <libpkgconf.h>
#include "serialize.h"

static bool
serialize_escape_string(pkgconf_buffer_t *buffer, const char *s)
{
	for (const char *p = s; *p; p++)
	{
		bool ret;

		switch (*p)
		{
		case '\"':
			ret = pkgconf_buffer_append(buffer, "\\\"");
			break;
		case '\\':
			ret = pkgconf_buffer_append(buffer, "\\\\");
			break;
		case '\b':
			ret = pkgconf_buffer_append(buffer, "\\b");
			break;
		case '\f':
			ret = pkgconf_buffer_append(buffer, "\\f");
			break;
		case '\n':
			ret = pkgconf_buffer_append(buffer, "\\n");
			break;
		case '\r':
			ret = pkgconf_buffer_append(buffer, "\\r");
			break;
		case '\t':
			ret = pkgconf_buffer_append(buffer, "\\t");
			break;
		default:
			if ((unsigned char) *p < 0x20)
				ret = pkgconf_buffer_append_fmt(buffer, "\\u%04x", (unsigned int)(unsigned char) *p);
			else
				ret = pkgconf_buffer_push_byte(buffer, *p);
		}

		if (!ret)
			return false;
	}

	return true;
}

static inline bool
serialize_add_indent(pkgconf_buffer_t *buffer, unsigned int level)
{
	for (; level; level--)
	{
		if (!pkgconf_buffer_append(buffer, "    "))
			return false;
	}

	return true;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconfcli_serialize_value_to_buf(pkgconf_buffer_t *buffer, pkgconfcli_serialize_value_t *value, unsigned int indent)
 *
 *    Serialize the given JSON to the buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add to.
 *    :param pkgconfcli_serialize_value *value: Value to serialize.
 *    :param unsigned int indent: Indent level
 *    :return: true on success, false on failure
 */
bool
pkgconfcli_serialize_value_to_buf(pkgconf_buffer_t *buffer, pkgconfcli_serialize_value_t *value, unsigned int indent)
{
	if (!buffer || !value)
		return false;

	switch(value->type) {
		case PKGCONFCLI_SERIALIZE_TYPE_STRING:
			return pkgconf_buffer_push_byte(buffer, '"') &&
				serialize_escape_string(buffer, value->value.s ? value->value.s : "") &&
				pkgconf_buffer_push_byte(buffer, '"');
		case PKGCONFCLI_SERIALIZE_TYPE_INT:
			return pkgconf_buffer_append_fmt(buffer, "%d", value->value.i);
		case PKGCONFCLI_SERIALIZE_TYPE_BOOL:
			return pkgconf_buffer_append(buffer, value->value.b ? "true" : "false");
		case PKGCONFCLI_SERIALIZE_TYPE_NULL:
			return pkgconf_buffer_append(buffer, "null");
		case PKGCONFCLI_SERIALIZE_TYPE_OBJECT:
		{
			pkgconf_node_t *iter;

			if (value->value.o == NULL ||
				!pkgconf_buffer_push_byte(buffer, '{') ||
				!pkgconf_buffer_push_byte(buffer, '\n'))
				return false;

			PKGCONF_FOREACH_LIST_ENTRY(value->value.o->entries.head, iter)
			{
				pkgconfcli_serialize_object_t *entry = iter->data;

				if (!serialize_add_indent(buffer, indent + 1) ||
					!pkgconf_buffer_push_byte(buffer, '"') ||
					!serialize_escape_string(buffer, entry->key ? entry->key : "") ||
					!pkgconf_buffer_append(buffer, "\": ") ||
					!pkgconfcli_serialize_value_to_buf(buffer, entry->value, indent + 1) ||
					(iter->next && !pkgconf_buffer_push_byte(buffer, ',')) ||
					!pkgconf_buffer_push_byte(buffer, '\n'))
					return false;
			}

			return serialize_add_indent(buffer, indent) &&
				pkgconf_buffer_push_byte(buffer, '}');
		}
		case PKGCONFCLI_SERIALIZE_TYPE_ARRAY:
		{
			pkgconf_node_t *iter;

			if (value->value.a == NULL ||
				!pkgconf_buffer_push_byte(buffer, '[') ||
				!pkgconf_buffer_push_byte(buffer, '\n'))
				return false;

			PKGCONF_FOREACH_LIST_ENTRY(value->value.a->items.head, iter)
			{
				pkgconfcli_serialize_value_t *entry = iter->data;

				if (!serialize_add_indent(buffer, indent + 1) ||
					!pkgconfcli_serialize_value_to_buf(buffer, entry, indent + 1) ||
					(iter->next && !pkgconf_buffer_push_byte(buffer, ',')) ||
					!pkgconf_buffer_push_byte(buffer, '\n'))
					return false;
			}

			return serialize_add_indent(buffer, indent) &&
				pkgconf_buffer_push_byte(buffer, ']');
		}
	}

	return false;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_object_add_take(pkgconfcli_serialize_object_list_t *object_list, const char *key, pkgconfcli_serialize_value_t *value)
 *
 *    Add a key-value pair to a JSON object list. The key is copied internally.
 *    The object list takes ownership of the value.
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string, copied internally.
 *    :param pkgconfcli_serialize_value_t *value: Value to associate with the key. Ownership transfers to the object list.
 *    :return: The value added, not owned by the caller.
 */
pkgconfcli_serialize_value_t *
pkgconfcli_serialize_object_add_take(pkgconfcli_serialize_object_list_t *object_list, const char *key, pkgconfcli_serialize_value_t *value)
{
	if (!object_list || !value)
	{
		pkgconfcli_serialize_value_free(value);
		return NULL;
	}

	pkgconf_node_t *node = calloc(1, sizeof(pkgconf_node_t));
	pkgconfcli_serialize_object_t *object = calloc(1, sizeof(pkgconfcli_serialize_object_t));
	char *keycopy = key ? strdup(key) : strdup("");
	if (!node || !object || !keycopy)
	{
		free(node);
		free(keycopy);
		/* object->key/value are not assigned yet; free the struct itself */
		free(object);
		pkgconfcli_serialize_value_free(value);
		return NULL;
	}

	object->key = keycopy;
	object->value = value;
	pkgconf_node_insert_tail(node, object, &object_list->entries);
	return value;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_array_t *array, pkgconfcli_serialize_value_t value)
 *
 *    Add a value to a JSON array. The array takes ownership of the value.
 *
 *    :param pkgconfcli_serialize_array_t *array: Array to add to.
 *    :param pkgconfcli_serialize_value_t value: Value to append. Ownership transfers to the array.
 *    :return: The value added, not owned by the caller.
 */
pkgconfcli_serialize_value_t *
pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_array_t *array, pkgconfcli_serialize_value_t *value)
{
	if (!array)
	{
		// Taking value, so free
		pkgconfcli_serialize_value_free(value);
		return NULL;
	}

	pkgconf_node_t *node = calloc(1, sizeof(pkgconf_node_t));
	if (!node)
	{
		pkgconfcli_serialize_value_free(value);
		return NULL;
	}

	pkgconf_node_insert_tail(node, value, &array->items);
	return value;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_object_list_t *pkgconfcli_serialize_object_list_new(void)
 *
 *    Allocate and initialize a new empty JSON object list.
 *
 *    :return: Pointer to a new pkgconfcli_serialize_object_list_t, or NULL on allocation failure.
 */
pkgconfcli_serialize_object_list_t *
pkgconfcli_serialize_object_list_new(void)
{
	return calloc(1, sizeof(pkgconfcli_serialize_object_list_t));
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_array_t *pkgconfcli_serialize_array_new(void)
 *
 *    Allocate and initialize a new empty JSON array.
 *
 *    :return: Pointer to a new pkgconfcli_serialize_array_t, or NULL on allocation failure.
 */
pkgconfcli_serialize_array_t *
pkgconfcli_serialize_array_new(void)
{
	return calloc(1, sizeof(pkgconfcli_serialize_array_t));
}

/*
 * !doc
 *
 * .. c:function:: void pkgconfcli_serialize_value_free(pkgconfcli_serialize_value_t *value)
 *
 *    Free all resources owned by a JSON value. For strings, frees the string.
 *    For objects and arrays, recursively frees all children. The value pointer
 *    itself is not freed as it is assumed to be stack-allocated.
 *
 *    :param pkgconfcli_serialize_value_t *value: Value to free. May be NULL.
 *    :return: nothing
 */
void
pkgconfcli_serialize_value_free(pkgconfcli_serialize_value_t *value)
{
	if (!value)
		return;

	switch (value->type)
	{
		case PKGCONFCLI_SERIALIZE_TYPE_STRING:
			free(value->value.s);
			break;
		case PKGCONFCLI_SERIALIZE_TYPE_ARRAY:
			pkgconfcli_serialize_array_free(value->value.a);
			break;
		case PKGCONFCLI_SERIALIZE_TYPE_OBJECT:
			pkgconfcli_serialize_object_list_free(value->value.o);
			break;
		default:
			// Nothing to do
			break;
	}

	free(value);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconfcli_serialize_object_free(pkgconfcli_serialize_object_t *object)
 *
 *    Free a JSON object entry, including its key string and owned value.
 *    The object pointer itself is not freed by this function.
 *
 *    :param pkgconfcli_serialize_object_t *object: Object entry to free. May be NULL.
 *    :return: nothing
 */
void
pkgconfcli_serialize_object_free(pkgconfcli_serialize_object_t *object)
{
	if (!object)
		return;

	free(object->key);
	pkgconfcli_serialize_value_free(object->value);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconfcli_serialize_object_list_free(pkgconfcli_serialize_object_list_t *object_list)
 *
 *    Free a JSON object list and all of its entries, including their keys and values.
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to free. May be NULL.
 *    :return: nothing
 */
void
pkgconfcli_serialize_object_list_free(pkgconfcli_serialize_object_list_t *object_list)
{
	if (!object_list)
		return;

	pkgconf_node_t *iter_next = NULL, *iter = NULL;
	PKGCONF_FOREACH_LIST_ENTRY_SAFE(object_list->entries.head, iter_next, iter)
	{
		pkgconfcli_serialize_object_t *object = iter->data;
		pkgconfcli_serialize_object_free(object);
		free(object);
		free(iter);
	}

	free(object_list);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconfcli_serialize_array_free(pkgconfcli_serialize_array_t *array)
 *
 *    Free a JSON array and all of its elements.
 *
 *    :param pkgconfcli_serialize_array_t *array: Array to free. May be NULL.
 *    :return: nothing
 */
void
pkgconfcli_serialize_array_free(pkgconfcli_serialize_array_t *array)
{
	if (!array)
		return;

	pkgconf_node_t *iter_next = NULL, *iter = NULL;
	PKGCONF_FOREACH_LIST_ENTRY_SAFE(array->items.head, iter_next, iter)
	{
		pkgconfcli_serialize_value_t *value = iter->data;
		pkgconfcli_serialize_value_free(value);
		free(iter);
	}

	free(array);
}
