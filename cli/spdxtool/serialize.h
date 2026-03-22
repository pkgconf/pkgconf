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
#include "util.h"

#ifndef CLI__SPDXTOOL__SERIALIZE_H
#define CLI__SPDXTOOL__SERIALIZE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum spdxtool_serialize_type_
{
	SPDXTOOL_SERIALIZE_TYPE_STRING, // JSON string type
	SPDXTOOL_SERIALIZE_TYPE_INT, // JSON number type (int)
	SPDXTOOL_SERIALIZE_TYPE_BOOL, // JSON bool type
	SPDXTOOL_SERIALIZE_TYPE_NULL, // JSON null type
	SPDXTOOL_SERIALIZE_TYPE_OBJECT, // JSON object type
	SPDXTOOL_SERIALIZE_TYPE_ARRAY // JSON array type
} spdxtool_serialize_type_t;

typedef struct spdxtool_serialize_value_ {
	spdxtool_serialize_type_t type;
	union {
		char *s;
		int i;
		bool b;
		struct spdxtool_serialize_object_list_ *o;
		struct spdxtool_serialize_array_ *a;
	} value;
} spdxtool_serialize_value_t;

typedef struct spdxtool_serialize_object_ {
	char *key;
	spdxtool_serialize_value_t value;
} spdxtool_serialize_object_t;

typedef struct spdxtool_serialize_object_list_ {
	pkgconf_list_t entries;
} spdxtool_serialize_object_list_t;

typedef struct spdxtool_serialize_array_ {
	pkgconf_list_t items;
} spdxtool_serialize_array_t;


void
spdxtool_serialize_value_to_buf(pkgconf_buffer_t *buffer, spdxtool_serialize_value_t value, unsigned int indent);

void
spdxtool_serialize_object_add(spdxtool_serialize_object_list_t *object_list, const char *key, spdxtool_serialize_value_t value);

spdxtool_serialize_object_list_t *
spdxtool_serialize_object_list_new(void);

spdxtool_serialize_array_t *
spdxtool_serialize_array_new(void);

void
spdxtool_serialize_value_free(spdxtool_serialize_value_t *value);

void
spdxtool_serialize_object_list_free(spdxtool_serialize_object_list_t *object_list);

void
spdxtool_serialize_object_free(spdxtool_serialize_object_t *object);

void
spdxtool_serialize_array_add(spdxtool_serialize_array_t *array, spdxtool_serialize_value_t value);

void
spdxtool_serialize_array_free(spdxtool_serialize_array_t *array);

/*
 * !doc
 *
 * .. c:function:: spdxtool_serialize_value_t spdxtool_serialize_string(const char *s)
 *
 *    Construct a JSON string value. The string is copied internally.
 *
 *    :param const char *s: String to copy. May be NULL, in which case the value holds NULL.
 *    :return: spdxtool_serialize_value_t of type SPDXTOOL_SERIALIZE_TYPE_STRING.
 */
static inline spdxtool_serialize_value_t
spdxtool_serialize_string(const char *s)
{
	return (spdxtool_serialize_value_t){
		.type = SPDXTOOL_SERIALIZE_TYPE_STRING,
		.value.s = s ? strdup(s) : NULL,
	};
}

/*
 * !doc
 *
 * .. c:function:: spdxtool_serialize_value_t spdxtool_serialize_int(int d)
 *
 *    Construct a JSON floating-point value.
 *
 *    :param int d: int value.
 *    :return: spdxtool_serialize_value_t of type SPDXTOOL_SERIALIZE_TYPE_INT.
 */
static inline spdxtool_serialize_value_t
spdxtool_serialize_int(int i)
{
	return (spdxtool_serialize_value_t){
		.type = SPDXTOOL_SERIALIZE_TYPE_INT,
		.value.i = i
	};
}

/*
 * !doc
 *
 * .. c:function:: spdxtool_serialize_value_t spdxtool_serialize_bool(bool b)
 *
 *    Construct a JSON boolean value.
 *
 *    :param bool b: Boolean value.
 *    :return: spdxtool_serialize_value_t of type SPDXTOOL_SERIALIZE_TYPE_BOOL.
 */
static inline spdxtool_serialize_value_t
spdxtool_serialize_bool(bool b)
{
	return (spdxtool_serialize_value_t){
		.type = SPDXTOOL_SERIALIZE_TYPE_BOOL,
		.value.b = b
	};
}

/*
 * !doc
 *
 * .. c:function:: spdxtool_serialize_value_t spdxtool_serialize_object(spdxtool_serialize_object_list_t *object_list)
 *
 *    Construct a JSON object value wrapping an existing object list.
 *    The returned value takes ownership of the object list.
 *
 *    :param spdxtool_serialize_object_list_t *object_list: Object list to wrap. Ownership transfers to the returned value.
 *    :return: spdxtool_serialize_value_t of type SPDXTOOL_SERIALIZE_TYPE_OBJECT.
 */
static inline spdxtool_serialize_value_t
spdxtool_serialize_object(spdxtool_serialize_object_list_t *object_list)
{
	return (spdxtool_serialize_value_t){
		.type = SPDXTOOL_SERIALIZE_TYPE_OBJECT,
		.value.o = object_list,
	};
}

/*
 * !doc
 *
 * .. c:function:: spdxtool_serialize_value_t spdxtool_serialize_null(void)
 *
 *    Construct a JSON null value.
 *
 *    :return: spdxtool_serialize_value_t of type SPDXTOOL_SERIALIZE_TYPE_NULL.
 */
static inline spdxtool_serialize_value_t
spdxtool_serialize_null(void)
{
	// We don't read value, but set it defensively anyway
	return (spdxtool_serialize_value_t){
		.type = SPDXTOOL_SERIALIZE_TYPE_NULL,
		.value.s = NULL,
	};
}

/*
 * !doc
 *
 * .. c:function:: spdxtool_serialize_value_t spdxtool_serialize_array(spdxtool_serialize_array_t *array)
 *
 *    Construct a JSON array value wrapping an existing array.
 *    The returned value takes ownership of the array.
 *
 *    :param spdxtool_serialize_array_t *a: Array to wrap. Ownership transfers to the returned value.
 *    :return: spdxtool_serialize_value_t of type SPDXTOOL_SERIALIZE_TYPE_ARRAY.
 */
static inline spdxtool_serialize_value_t
spdxtool_serialize_array(spdxtool_serialize_array_t *array)
{
	return (spdxtool_serialize_value_t){
		.type = SPDXTOOL_SERIALIZE_TYPE_ARRAY,
		.value.a = array
	};
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_object_add_string(spdxtool_serialize_object_list_t *object_list, const char *key, const char *value)
 *
 *    Add a string key-value pair to a JSON object. The string is copied internally.
 *    Unconditionally adds the key even if value is NULL.
 *
 *    :param spdxtool_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param const char *value: String value to copy.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_object_add_string(spdxtool_serialize_object_list_t *object_list, const char *key, const char *value)
{
	spdxtool_serialize_object_add(object_list, key, spdxtool_serialize_string(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_object_add_string_opt(spdxtool_serialize_object_list_t *object_list, const char *key, const char *value)
 *
 *    Add a string key-value pair to a JSON object only if value is non-NULL.
 *    Use this for optional fields that should be omitted entirely when absent.
 *
 *    :param spdxtool_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param const char *value: String value to copy, or NULL to skip.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_object_add_string_opt(spdxtool_serialize_object_list_t *object_list, const char *key, const char *value)
{
	if (value)
		spdxtool_serialize_object_add(object_list, key, spdxtool_serialize_string(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_object_add_int(spdxtool_serialize_object_list_t *object_list, const char *key, int value)
 *
 *    Add a int key-value pair to a JSON object.
 *
 *    :param spdxtool_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param int value: Integer value.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_object_add_int(spdxtool_serialize_object_list_t *object_list, const char *key, int value)
{
	spdxtool_serialize_object_add(object_list, key, spdxtool_serialize_int(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_object_add_bool(spdxtool_serialize_object_list_t *object_list, const char *key, bool value)
 *
 *    Add a boolean key-value pair to a JSON object.
 *
 *    :param spdxtool_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param bool value: Boolean value.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_object_add_bool(spdxtool_serialize_object_list_t *object_list, const char *key, bool value)
{
	spdxtool_serialize_object_add(object_list, key, spdxtool_serialize_bool(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_object_add_null(spdxtool_serialize_object_list_t *object_list, const char *key)
 *
 *    Add a null key-value pair to a JSON object.
 *
 *    :param spdxtool_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_object_add_null(spdxtool_serialize_object_list_t *object_list, const char *key)
{
	spdxtool_serialize_object_add(object_list, key, spdxtool_serialize_null());
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_object_add_object(spdxtool_serialize_object_list_t *object_list, const char *key, spdxtool_serialize_object_list_t *value)
 *
 *    Add an object key-value pair to a JSON object.
 *    This takes ownership of the object in value.
 *
 *    :param spdxtool_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param spdxtool_serialize_object_list_t *value: Object value to add.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_object_add_object(spdxtool_serialize_object_list_t *object_list, const char *key, spdxtool_serialize_object_list_t *value)
{
	spdxtool_serialize_object_add(object_list, key, spdxtool_serialize_object(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_object_add_array(spdxtool_serialize_object_list_t *object_list, const char *key, spdxtool_serialize_array_t *value)
 *
 *    Add an array key-value pair to a JSON object.
 *    This takes ownership of the array in value.
 *
 *    :param spdxtool_serialize_object_list_t *object_list: Object list to add to.
 *    :param const char *key: Key string.
 *    :param spdxtool_serialize_array_t *value: Array value to add.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_object_add_array(spdxtool_serialize_object_list_t *object_list, const char *key, spdxtool_serialize_array_t *value)
{
	spdxtool_serialize_object_add(object_list, key, spdxtool_serialize_array(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_array_add_string(spdxtool_serialize_array_t *array, const char *value)
 *
 *    Append a string value to a JSON array. The string is copied internally.
 *    Unconditionally appends even if value is NULL.
 *
 *    :param spdxtool_serialize_array_t *array: Array to append to.
 *    :param const char *value: String value to copy.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_array_add_string(spdxtool_serialize_array_t *array, const char *value)
{
	spdxtool_serialize_array_add(array, spdxtool_serialize_string(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_array_add_string_opt(spdxtool_serialize_array_t *a, const char *value)
 *
 *    Append a string value to a JSON array only if value is non-NULL.
 *    Use this for optional array entries that should be omitted when absent.
 *
 *    :param spdxtool_serialize_array_t *a: Array to append to.
 *    :param const char *value: String value to copy, or NULL to skip.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_array_add_string_opt(spdxtool_serialize_array_t *array, const char *value)
{
	if (value)
		spdxtool_serialize_array_add(array, spdxtool_serialize_string(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_array_add_int(spdxtool_serialize_array_t *array, int value)
 *
 *    Append a int value to a JSON array.
 *
 *    :param spdxtool_serialize_array_t *a: Array to append to.
 *    :param int value: integer value.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_array_add_int(spdxtool_serialize_array_t *array, int value)
{
	spdxtool_serialize_array_add(array, spdxtool_serialize_int(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_array_add_bool(spdxtool_serialize_array_t *array, bool value)
 *
 *    Append a boolean value to a JSON array.
 *
 *    :param spdxtool_serialize_array_t *array: Array to append to.
 *    :param bool value: Boolean value.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_array_add_bool(spdxtool_serialize_array_t *array, bool value)
{
	spdxtool_serialize_array_add(array, spdxtool_serialize_bool(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_array_add_null(spdxtool_serialize_array_t *array)
 *
 *    Append a null value to a JSON array.
 *
 *    :param spdxtool_serialize_array_t *array: Array to append to.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_array_add_null(spdxtool_serialize_array_t *array)
{
	spdxtool_serialize_array_add(array, spdxtool_serialize_null());
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_array_add_object(spdxtool_serialize_array_t *array, spdxtool_serialize_object_list_t *value)
 *
 *    Append an object value to a JSON array.
 *    This takes ownership of the object in vallue.
 *
 *    :param spdxtool_serialize_array_t *array: Array to append to.
 *    :param spdxtool_serialize_object_list_t *value: Object value.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_array_add_object(spdxtool_serialize_array_t *array, spdxtool_serialize_object_list_t *value)
{
	spdxtool_serialize_array_add(array, spdxtool_serialize_object(value));
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_array_add_array(spdxtool_serialize_array_t *array, spdxtool_serialize_array_t *value)
 *
 *    Append an array value to a JSON array.
 *    This takes ownership of the array in value.
 *
 *    :param spdxtool_serialize_array_t *array: Array to append to.
 *    :param spdxtool_serialize_array_t *value: Array value.
 *    :return: nothing
 */
static inline void
spdxtool_serialize_array_add_array(spdxtool_serialize_array_t *array, spdxtool_serialize_array_t *value)
{
	spdxtool_serialize_array_add(array, spdxtool_serialize_array(value));
}

spdxtool_serialize_value_t
spdxtool_serialize_sbom(pkgconf_client_t *client, spdxtool_core_agent_t *agent, spdxtool_core_creation_info_t *creation, spdxtool_core_spdx_document_t *spdx);

#ifdef __cplusplus
}
#endif

#endif
