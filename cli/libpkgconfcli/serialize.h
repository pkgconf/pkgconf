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

#ifndef CLI__LIBSBOM__SERIALIZE_H
#define CLI__LIBSBOM__SERIALIZE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum pkgconfcli_serialize_type_
{
	PKGCONFCLI_SERIALIZE_TYPE_STRING, // JSON string type
	PKGCONFCLI_SERIALIZE_TYPE_INT, // JSON number type (int)
	PKGCONFCLI_SERIALIZE_TYPE_BOOL, // JSON bool type
	PKGCONFCLI_SERIALIZE_TYPE_NULL, // JSON null type
	PKGCONFCLI_SERIALIZE_TYPE_OBJECT, // JSON object type
	PKGCONFCLI_SERIALIZE_TYPE_ARRAY // JSON array type
} pkgconfcli_serialize_type_t;

typedef struct pkgconfcli_serialize_value_ {
	pkgconfcli_serialize_type_t type;
	union {
		char *s;
		int i;
		bool b;
		struct pkgconfcli_serialize_object_list_ *o;
		struct pkgconfcli_serialize_array_ *a;
	} value;
} pkgconfcli_serialize_value_t;

typedef struct pkgconfcli_serialize_object_ {
	char *key;
	pkgconfcli_serialize_value_t *value;
} pkgconfcli_serialize_object_t;

typedef struct pkgconfcli_serialize_object_list_ {
	pkgconf_list_t entries;
} pkgconfcli_serialize_object_list_t;

typedef struct pkgconfcli_serialize_array_ {
	pkgconf_list_t items;
} pkgconfcli_serialize_array_t;

bool
pkgconfcli_serialize_value_to_buf(pkgconf_buffer_t *buffer, pkgconfcli_serialize_value_t *value, unsigned int indent);

pkgconfcli_serialize_value_t *
pkgconfcli_serialize_value_dup(const pkgconfcli_serialize_value_t *value);

pkgconfcli_serialize_value_t *
pkgconfcli_serialize_object_add_take(pkgconfcli_serialize_object_list_t *object_list, const char *key, pkgconfcli_serialize_value_t* value);

pkgconfcli_serialize_object_list_t *
pkgconfcli_serialize_object_list_new(void);

pkgconfcli_serialize_array_t *
pkgconfcli_serialize_array_new(void);

pkgconfcli_serialize_value_t *
pkgconfcli_serialize_array_add_take(pkgconfcli_serialize_array_t *array, pkgconfcli_serialize_value_t* value);

void
pkgconfcli_serialize_value_free(pkgconfcli_serialize_value_t *value);

void
pkgconfcli_serialize_object_list_free(pkgconfcli_serialize_object_list_t *object_list);

void
pkgconfcli_serialize_object_free(pkgconfcli_serialize_object_t *object);

void
pkgconfcli_serialize_array_free(pkgconfcli_serialize_array_t *array);

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t * pkgconfcli_serialize_value_string(const char *s)
 *
 *    Construct a JSON string value. The string is copied internally.
 *    If this return value is not stolen, it must be freed with pkgconfcli_serialize_value_free().
 *
 *    :param const char *s: String to copy. May be NULL, in which case the value holds NULL.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_STRING.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_value_string(const char *s)
{
	if (!s)
		return NULL;

	char *sv = strdup(s);
	if (!sv)
		return NULL;

	pkgconfcli_serialize_value_t *value = calloc(1, sizeof(pkgconfcli_serialize_value_t));
	if (!value)
	{
		free(sv);
		return NULL;
	}

	value->type = PKGCONFCLI_SERIALIZE_TYPE_STRING;
	value->value.s = sv;
	return value;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_value_int(int d)
 *
 *    Construct a JSON integer value.
 *    If this return value is not stolen, it must be freed with pkgconfcli_serialize_value_free().
 *
 *    :param int d: int value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_INT.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_value_int(int i)
{
	pkgconfcli_serialize_value_t *value = calloc(1, sizeof(pkgconfcli_serialize_value_t));
	if (!value)
		return NULL;

	value->type = PKGCONFCLI_SERIALIZE_TYPE_INT;
	value->value.i = i;
	return value;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_value_bool(bool b)
 *
 *    Construct a JSON boolean value.
 *    If this return value is not stolen, it must be freed with pkgconfcli_serialize_value_free().
 *
 *    :param bool b: Boolean value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_BOOL.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_value_bool(bool b)
{
	pkgconfcli_serialize_value_t *value = calloc(1, sizeof(pkgconfcli_serialize_value_t));
	if (!value)
		return NULL;

	value->type = PKGCONFCLI_SERIALIZE_TYPE_BOOL;
	value->value.b = b;
	return value;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_value_null(void)
 *
 *    Construct a JSON null value.
 *    If this return value is not stolen, it must be freed with pkgconfcli_serialize_value_free().
 *
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_NULL.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_value_null(void)
{
	pkgconfcli_serialize_value_t *value = calloc(1, sizeof(pkgconfcli_serialize_value_t));
	if (!value)
		return NULL;

	value->type = PKGCONFCLI_SERIALIZE_TYPE_NULL;
	return value;
}


/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_value_object(pkgconfcli_serialize_object_list_t *object_list)
 *
 *    Construct a JSON object value wrapping an existing object list.
 *    The returned value takes ownership of the object list.
 *    If this return value is not stolen, it must be freed with pkgconfcli_serialize_value_free().
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to wrap. Ownership transfers to the returned value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_OBJECT.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_value_object(pkgconfcli_serialize_object_list_t *object_list)
{
	pkgconfcli_serialize_value_t *value = calloc(1, sizeof(pkgconfcli_serialize_value_t));
	if (!value)
	{
		pkgconfcli_serialize_object_list_free(object_list);
		return NULL;
	}

	value->type = PKGCONFCLI_SERIALIZE_TYPE_OBJECT;
	value->value.o = object_list;
	return value;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_value_array(pkgconfcli_serialize_array_t *array)
 *
 *    Construct a JSON array value wrapping an existing array.
 *    The returned value takes ownership of the array.
 *    If this return value is not stolen, it must be freed with pkgconfcli_serialize_value_free().
 *
 *    :param pkgconfcli_serialize_array_t *array: Array to wrap. Ownership transfers to the returned value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_ARRAY.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_value_array(pkgconfcli_serialize_array_t *array)
{
	pkgconfcli_serialize_value_t *value = calloc(1, sizeof(pkgconfcli_serialize_value_t));
	if (!value)
	{
		pkgconfcli_serialize_array_free(array);
		return NULL;
	}

	value->type = PKGCONFCLI_SERIALIZE_TYPE_ARRAY;
	value->value.a = array;
	return value;
}

/*
 * !doc
 *
 * .. c:function:: void pkgconfcli_serialize_object_add_string(pkgconfcli_serialize_object_list_t *object_list, const char *key, const char *value)
 *
 *    Add a string key-value pair to a JSON object. The string is copied internally.
 *    Unconditionally adds the key even if value is NULL.
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param const char *value: String value to copy.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_STRING, located in the object.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_object_add_string(pkgconfcli_serialize_object_list_t *object_list, const char *key, const char *value)
{
	return pkgconfcli_serialize_object_add_take(object_list, key, pkgconfcli_serialize_value_string(value));
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_object_add_string_opt(pkgconfcli_serialize_object_list_t *object_list, const char *key, const char *value)
 *
 *    Add a string key-value pair to a JSON object only if value is non-NULL.
 *    Use this for optional fields that should be omitted entirely when absent.
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param const char *value: String value to copy, or NULL to skip.
 *    :return: If value is set: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_STRING, located in the object.
 *             This object is not owned by the caller.
 *             If value is not set: NULL.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_object_add_string_opt(pkgconfcli_serialize_object_list_t *object_list, const char *key, const char *value)
{
	if (value)
		return pkgconfcli_serialize_object_add_take(object_list, key, pkgconfcli_serialize_value_string(value));

	return NULL;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_object_add_int(pkgconfcli_serialize_object_list_t *object_list, const char *key, int value)
 *
 *    Add a int key-value pair to a JSON object.
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param int value: Integer value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_INT, located in the object.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_object_add_int(pkgconfcli_serialize_object_list_t *object_list, const char *key, int value)
{
	return pkgconfcli_serialize_object_add_take(object_list, key, pkgconfcli_serialize_value_int(value));
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_object_add_bool(pkgconfcli_serialize_object_list_t *object_list, const char *key, bool value)
 *
 *    Add a boolean key-value pair to a JSON object.
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param bool value: Boolean value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_BOOL, located in the object.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_object_add_bool(pkgconfcli_serialize_object_list_t *object_list, const char *key, bool value)
{
	return pkgconfcli_serialize_object_add_take(object_list, key, pkgconfcli_serialize_value_bool(value));
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_object_add_null(pkgconfcli_serialize_object_list_t *object_list, const char *key)
 *
 *    Add a null key-value pair to a JSON object.
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_NULL, located in the object.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_object_add_null(pkgconfcli_serialize_object_list_t *object_list, const char *key)
{
	return pkgconfcli_serialize_object_add_take(object_list, key, pkgconfcli_serialize_value_null());
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_object_add_object(pkgconfcli_serialize_object_list_t *object_list, const char *key, pkgconfcli_serialize_object_list_t *value)
 *
 *    Add an object key-value pair to a JSON object.
 *    This takes ownership of the object in value unconditionally, freeing on failure.
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param pkgconfcli_serialize_object_list_t *value: Object value to add.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_OBJECT, located in the object.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_object_add_object(pkgconfcli_serialize_object_list_t *object_list, const char *key, pkgconfcli_serialize_object_list_t *value)
{
	return pkgconfcli_serialize_object_add_take(object_list, key, pkgconfcli_serialize_value_object(value));
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_object_add_array(pkgconfcli_serialize_object_list_t *object_list, const char *key, pkgconfcli_serialize_array_t *value)
 *
 *    Add an array key-value pair to a JSON object.
 *    This takes ownership of the array in value unconditionally, freeing on failure.
 *
 *    :param pkgconfcli_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param pkgconfcli_serialize_array_t *value: Array value to add.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_ARRAY, located in the object.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_object_add_array(pkgconfcli_serialize_object_list_t *object_list, const char *key, pkgconfcli_serialize_array_t *value)
{
	return pkgconfcli_serialize_object_add_take(object_list, key, pkgconfcli_serialize_value_array(value));
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_array_add_string(pkgconfcli_serialize_array_t *array, const char *value)
 *
 *    Append a string value to a JSON array. The string is copied internally.
 *    Unconditionally appends even if value is NULL.
 *
 *    :param pkgconfcli_serialize_array_t *array: Array to append to.
 *    :param const char *value: String value to copy.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_STRING, located in the array.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_array_add_string(pkgconfcli_serialize_array_t *array, const char *value)
{
	return pkgconfcli_serialize_array_add_take(array, pkgconfcli_serialize_value_string(value));
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_array_add_string_opt(pkgconfcli_serialize_array_t *a, const char *value)
 *
 *    Append a string value to a JSON array only if value is non-NULL.
 *    Use this for optional array entries that should be omitted when absent.
 *
 *    :param pkgconfcli_serialize_array_t *a: Array to append to.
 *    :param const char *value: String value to copy, or NULL to skip.
 *    :return: If value is set: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_STRING, located in the array.
 *             This object is not owned by the caller.
 *             If value is not set: NULL.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_array_add_string_opt(pkgconfcli_serialize_array_t *array, const char *value)
{
	if (value)
		return pkgconfcli_serialize_array_add_take(array, pkgconfcli_serialize_value_string(value));

	return NULL;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_array_add_int(pkgconfcli_serialize_array_t *array, int value)
 *
 *    Append a int value to a JSON array.
 *
 *    :param pkgconfcli_serialize_array_t *array: Array to append to.
 *    :param int value: integer value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_INT, located in the array.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_array_add_int(pkgconfcli_serialize_array_t *array, int value)
{
	return pkgconfcli_serialize_array_add_take(array, pkgconfcli_serialize_value_int(value));
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_array_add_bool(pkgconfcli_serialize_array_t *array, bool value)
 *
 *    Append a boolean value to a JSON array.
 *
 *    :param pkgconfcli_serialize_array_t *array: Array to append to.
 *    :param bool value: Boolean value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_BOOL, located in the array.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_array_add_bool(pkgconfcli_serialize_array_t *array, bool value)
{
	return pkgconfcli_serialize_array_add_take(array, pkgconfcli_serialize_value_bool(value));
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_array_add_null(pkgconfcli_serialize_array_t *array)
 *
 *    Append a null value to a JSON array.
 *
 *    :param pkgconfcli_serialize_array_t *array: Array to append to.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_NULL, located in the array.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_array_add_null(pkgconfcli_serialize_array_t *array)
{
	return pkgconfcli_serialize_array_add_take(array, pkgconfcli_serialize_value_null());
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_array_add_object(pkgconfcli_serialize_array_t *array, pkgconfcli_serialize_object_list_t *value)
 *
 *    Append an object value to a JSON array.
 *    This takes ownership of the object in value unconditionally, freeing on failure.
 *
 *    :param pkgconfcli_serialize_array_t *array: Array to append to.
 *    :param pkgconfcli_serialize_object_list_t *value: Object value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_OBJECT, located in the array.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_array_add_object(pkgconfcli_serialize_array_t *array, pkgconfcli_serialize_object_list_t *value)
{
	if (!value)
		return NULL;

	pkgconfcli_serialize_value_t *ret = pkgconfcli_serialize_value_object(value);
	if (!ret)
	{
		// Since we take possession of the pointer unconditionally, clean up.
		pkgconfcli_serialize_object_list_free(value);
		return NULL;
	}

	return pkgconfcli_serialize_array_add_take(array, ret);
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *pkgconfcli_serialize_array_add_array(pkgconfcli_serialize_array_t *array, pkgconfcli_serialize_array_t *value)
 *
 *    Append an array value to a JSON array.
 *    This takes ownership of the array in value unconditionally, freeing on failure.
 *
 *    :param pkgconfcli_serialize_array_t *array: Array to append to.
 *    :param pkgconfcli_serialize_array_t *value: Array value.
 *    :return: pkgconfcli_serialize_value_t * of type PKGCONFCLI_SERIALIZE_TYPE_ARRAY, located in the array.
 *             This object is not owned by the caller.
 */
static inline pkgconfcli_serialize_value_t *
pkgconfcli_serialize_array_add_array(pkgconfcli_serialize_array_t *array, pkgconfcli_serialize_array_t *value)
{
	if (!value)
		return NULL;

	pkgconfcli_serialize_value_t *ret = pkgconfcli_serialize_value_array(value);
	if (!ret)
	{
		// Since we take possession of the pointer unconditionally, clean up.
		pkgconfcli_serialize_array_free(value);
		return NULL;
	}

	return pkgconfcli_serialize_array_add_take(array, ret);
}

/*pkgconfcli_serialize_value_t *
pkgconfcli_serialize_sbom(pkgconf_client_t *client, libsbom_core_agent_t *agent, libsbom_core_tool_t *tool, libsbom_core_creation_info_t *creation, libsbom_core_spdx_document_t *spdx);*/

#ifdef __cplusplus
}
#endif

#endif
