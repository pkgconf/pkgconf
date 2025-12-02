/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2025 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "core.h"

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_add_indent(pkgconf_buffer_t *buffer, unsigned int level)
 *
 *    Add some indent. Level tells how many spaces.
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param unsigned int level: Add level * 4 spaces. There can be 12 levels
 *    :return: nothing
 */
void
spdxtool_serialize_add_indent(pkgconf_buffer_t *buffer, unsigned int level)
{
	unsigned int indent = 0;
	char indent_str[48];

	if(level >= 12)
	{
		indent = 12;
	}

	indent = level * 4;

	memset(indent_str, 0x00, 48);
	memset(indent_str, ' ', indent);

	pkgconf_buffer_append(buffer, indent_str);
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_add_ch_with_comma(pkgconf_buffer_t *buffer, char ch, unsigned int level, bool comma)
 *
 *    Add character and optional comma to buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param char ch: Char to add
 *    :param bool comma: If comma is true then add comma it not then not
 *    :return: nothing
 */
void
spdxtool_serialize_add_ch_with_comma(pkgconf_buffer_t *buffer, char ch, unsigned int level, bool comma)
{
	char append_string[4] = {0x00, 0x00, 0x00, 0x00};
	char *ptr = append_string;
	/* If this just used to add comman adn \n then ch is 0x00 */
	if(ch > 0x00)
	{
		spdxtool_serialize_add_indent(buffer, level);
		*(ptr) = ch;
		ptr ++;
	}

	if(comma)
	{
		*(ptr) = ',';
		ptr ++;
	}

	*(ptr) = '\n';

	pkgconf_buffer_append(buffer, append_string);
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_add_string_with_comma(pkgconf_buffer_t *buffer, char *string, bool comma)
 *
 *    Add string and optional comma to buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param char *string: String to add
 *    :param bool comma: If comma is true then add comma it not then not
 *    :param unsigned int level: Indent level
 *    :return: nothing
 */
void
spdxtool_serialize_add_string_with_comma(pkgconf_buffer_t *buffer, char *string, bool comma)
{
	pkgconf_buffer_append(buffer, string);
	free(string);

	spdxtool_serialize_add_ch_with_comma(buffer, 0x00, 0, comma);
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_parm_and_string(pkgconf_buffer_t *buffer, char *parm, char *string, unsigned int level, bool comma)
 *
 *    Add paramter, string and optional comma to buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param char *param: Parameter name
 *    :param char *string: String to add
 *    :param unsigned int level: Indent level
 *    :param bool comma: If comma is true then add comma it not then not
 *    :return: nothing
 */
void
spdxtool_serialize_parm_and_string(pkgconf_buffer_t *buffer, char *parm, char *string, unsigned int level, bool comma)
{
	char *tmp_str = NULL;

	spdxtool_serialize_add_indent(buffer, level);
	if( asprintf(&tmp_str, "\"%s\": \"%s\"", parm, string) < 0)
	{
		pkgconf_error(NULL, "Can't serialize paramter and string to JSON. Memory exhausted!");
	}
	else
	{
		spdxtool_serialize_add_string_with_comma(buffer, tmp_str, comma);
	}
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_parm_and_char(pkgconf_buffer_t *buffer, char *parm, char ch, unsigned int level, bool comma)
 *
 *    Add paramter, char and optional comma to buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param char *param: Parameter name
 *    :param char ch: Char to add
 *    :param unsigned int level: Indent level
 *    :param bool comma: If comma is true then add comma it not then not
 *    :return: nothing
 */
void
spdxtool_serialize_parm_and_char(pkgconf_buffer_t *buffer, char *parm, char ch, unsigned int level, bool comma)
{
	char *tmp_str = NULL;

	spdxtool_serialize_add_indent(buffer, level);
	if(asprintf(&tmp_str, "\"%s\": %c", parm, ch) < 0)
	{
		pkgconf_error(NULL, "Can't serialize paramter and char to JSON. Memory exhausted!");
	}
	else
	{
		spdxtool_serialize_add_string_with_comma(buffer, tmp_str, comma);
	}
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_parm_and_int(pkgconf_buffer_t *buffer, char *parm, int integer, unsigned int level, bool comma)
 *
 *    Add paramter, integer and optional comma to buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param char *param: Parameter name
 *    :param int integer: Int to add
 *    :param unsigned int level: Indent level
 *    :param bool comma: If comma is true then add comma it not then not
 *    :return: nothing
 */
void
spdxtool_serialize_parm_and_int(pkgconf_buffer_t *buffer, char *parm, int integer, unsigned int level, bool comma)
{
	char *tmp_str = NULL;

	spdxtool_serialize_add_indent(buffer, level);
	if(asprintf(&tmp_str, "\"%s\": %d", parm, integer) < 0)
	{
		pkgconf_error(NULL, "Can't serialize paramter and int to JSON. Memory exhausted!");
	}
	else
	{
		spdxtool_serialize_add_string_with_comma(buffer, tmp_str, comma);
	}
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_string(pkgconf_buffer_t *buffer, char *string, unsigned int level, bool comma)
 *
 *    Add just string.
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param char *param: Parameter name
 *    :param char *ch: String to add
 *    :param unsigned int: level Indent level
 *    :param bool comma: If comma is true then add comma it not then not
 *    :return: nothing
 */
void
spdxtool_serialize_string(pkgconf_buffer_t *buffer, char *string, unsigned int level, bool comma)
{
	char *tmp_str = NULL;

	spdxtool_serialize_add_indent(buffer, level);
	if( asprintf(&tmp_str, "\"%s\"", string) < 0)
	{
		pkgconf_error(NULL, "Can't serialize string to JSON. Memory exhausted!");
	}
	else
	{
		spdxtool_serialize_add_string_with_comma(buffer, tmp_str, comma);
	}
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_obj_start(pkgconf_buffer_t *buffer, unsigned int level)
 *
 *    Start JSON object to buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param unsigned int level: Indent level
 *    :return: nothing
 */
void
spdxtool_serialize_obj_start(pkgconf_buffer_t *buffer, unsigned int level)
{
	spdxtool_serialize_add_ch_with_comma(buffer, '{', level, 0);
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_obj_end(pkgconf_buffer_t *buffer, unsigned int level, bool comma)
 *
 *    End JSON object to buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param unsigned int level: Level which is added
 *    :param bool comma: If need to add comma
 *    :return: nothing
 */
void
spdxtool_serialize_obj_end(pkgconf_buffer_t *buffer, unsigned int level, bool comma)
{
	spdxtool_serialize_add_ch_with_comma(buffer, '}', level, comma);
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_obj_start(pkgconf_buffer_t *buffer, unsigned int level)
 *
 *    Start JSON array to buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param unsigned int level: Level which is added
 *    :return: nothing
 */
void
spdxtool_serialize_array_start(pkgconf_buffer_t *buffer, unsigned int level)
{
	spdxtool_serialize_add_ch_with_comma(buffer, '[', level, 0);
}

/*
 * !doc
 *
 * .. c:function:: void spdxtool_serialize_array_end(pkgconf_buffer_t *buffer, unsigned int level, bool comma)
 *
 *    End JSON array to buffer
 *
 *    :param pkgconf_buffer_t *buffer: Buffer to add.
 *    :param unsigned int level: Level which is added
 *    :param bool comma: If need to add comma
 *    :return: nothing
 */
void
spdxtool_serialize_array_end(pkgconf_buffer_t *buffer, unsigned int level, bool comma)
{
	spdxtool_serialize_add_ch_with_comma(buffer, ']', level, comma);
}
