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
 *    :param unsigned int level: Add level * 4 spaces.
 *    :return: nothing
 */
void
spdxtool_serialize_add_indent(pkgconf_buffer_t *buffer, unsigned int level)
{
	for (; level; level--)
		pkgconf_buffer_append(buffer, "    ");
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
	/* If this just used to add comman adn \n then ch is 0x00 */
	if(ch > 0x00)
	{
		spdxtool_serialize_add_indent(buffer, level);
		pkgconf_buffer_push_byte(buffer, ch);
	}

	if(comma)
		pkgconf_buffer_push_byte(buffer, ',');

	pkgconf_buffer_push_byte(buffer, '\n');
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
	spdxtool_serialize_add_indent(buffer, level);

	pkgconf_buffer_append_fmt(buffer, "\"%s\": \"%s\"", parm, string);
	spdxtool_serialize_add_ch_with_comma(buffer, 0x00, 0, comma);
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
	spdxtool_serialize_add_indent(buffer, level);

	pkgconf_buffer_append_fmt(buffer, "\"%s\": %c", parm, ch);
	spdxtool_serialize_add_ch_with_comma(buffer, 0x00, 0, comma);
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
	spdxtool_serialize_add_indent(buffer, level);

	pkgconf_buffer_append_fmt(buffer, "\"%s\": %d", parm, integer);
	spdxtool_serialize_add_ch_with_comma(buffer, 0x00, 0, comma);
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
	spdxtool_serialize_add_indent(buffer, level);

	pkgconf_buffer_push_byte(buffer, '"');
	pkgconf_buffer_append(buffer, string);
	pkgconf_buffer_push_byte(buffer, '"');
	spdxtool_serialize_add_ch_with_comma(buffer, 0x00, 0, comma);
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
