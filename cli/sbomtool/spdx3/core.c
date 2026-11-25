/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2026 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include <stdlib.h>
#include "util.h"
#include "serialize.h"
#include "spdx3/core.h"


/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_spdx3_core_agent(pkgconf_client_t *client, const char *creation_info_id, const char *name)
 *
 *    Serialize /Core/Agent as a JSON value tree.
 *
 *    :param pkgconf_client_t *client: Pkgconf Client
 *    :param sbomtool_spdx3_document_t *document: SPDX document structure which hold needed information
 *    :param const char *name: Agent name string
 *    :return: pkgconfcli_serialize_value_t * representing the Agent object.
 */
pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_agent(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *name)
{
	pkgconfcli_serialize_value_t *ret = NULL;
	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	char *spdx_id = sbomtool_util_get_sbom_id_string(client, "Agent", name);

	if (!spdx_id || !object_list)
		goto err;

	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", "Agent") &&
	        pkgconfcli_serialize_object_add_string(object_list, "creationInfo", document->creation_info_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "spdxId", spdx_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "name", name)))
		goto err;

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_spdx3_core_agent: out of memory");

	free(spdx_id);
	pkgconfcli_serialize_object_list_free(object_list);
	return ret;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_spdx3_core_tool_to_object(pkgconf_client_t *client, const sbomtool_core_tool_t *tool)
 *
 *    Serialize /Core/Tool as a JSON value tree.
 *
 *    :param pkgconf_client_t *client: Pkgconf Client
 *    :param sbomtool_spdx3_document_t *document: SPDX document structure which hold needed information
 *    :param const char *short_name: Short name of tool
 *    :param const char *name: Longer tool name
 *    :return: pkgconfcli_serialize_value_t * representing the Agent object.
 */
pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_tool(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *short_name, const char *name)
{
	pkgconfcli_serialize_value_t *ret = NULL;
	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	char *spdx_id = sbomtool_util_get_sbom_id_string(client, "Tool", short_name);

	if (!spdx_id || !object_list)
		goto err;

	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", "Tool") &&
	        pkgconfcli_serialize_object_add_string(object_list, "creationInfo", document->creation_info_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "spdxId", spdx_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "name", name)))
		goto err;

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_spdx3_core_tool: out of memory");

	free(spdx_id);
	pkgconfcli_serialize_object_list_free(object_list);
	return ret;
}


/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_spdx3_core_creation_info_to_object(pkgconf_client_t *client, const sbomtool_core_creation_info_t *creation)
 *
 *    Serialize /Core/CreationInfo as a JSON value tree.
 *
 *    :param pkgconf_client_t *client: Pkgconf Client
 *    :param sbomtool_spdx3_document_t *document: SPDX document structure which hold needed information
 *    :return: pkgconfcli_serialize_value_t * representing the CreationInfo object.
 */
pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_creation_info(pkgconf_client_t *client, sbomtool_spdx3_document_t *document)
{
	pkgconfcli_serialize_value_t *ret = NULL;
	pkgconfcli_serialize_object_list_t *object_list = NULL;
	char *cur_time = NULL;
	const char *ptr_timestamp = NULL;
	pkgconfcli_serialize_array_t *created_by = NULL;
	pkgconfcli_serialize_array_t *created_using = NULL;

	if (!document->timestamp)
	{
		cur_time = sbomtool_util_get_current_iso8601_time();

		if (!cur_time)
			goto err;

		ptr_timestamp = cur_time;
	}
	else
		ptr_timestamp = document->timestamp;

	object_list = pkgconfcli_serialize_object_list_new();

	if (!object_list)
		goto err;

	created_by = pkgconfcli_serialize_array_new();

	if (!created_by)
		goto err;

	if (!pkgconfcli_serialize_array_add_string(created_by, document->agent_spdx_id))
		goto err;

	created_using = pkgconfcli_serialize_array_new();

	if (!created_using)
		goto err;

	if (document->tool_spdx_id)
	{

		if (!pkgconfcli_serialize_array_add_string(created_using, document->tool_spdx_id))
			goto err;
	}
	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", "CreationInfo") &&
	        pkgconfcli_serialize_object_add_string(object_list, "@id", document->creation_info_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "created", ptr_timestamp)))
		goto err;

	/* object_add_array takes ownership of created_by, freeing it on failure */
	if (!pkgconfcli_serialize_object_add_array(object_list, "createdBy", created_by))
	{
		created_by = NULL;
		goto err;
	}

	created_by = NULL;

	/* object_add_array takes ownership of created_using, freeing it on failure */
	if (!pkgconfcli_serialize_object_add_array(object_list, "createUsing", created_using))
	{
		created_using = NULL;
		goto err;
	}

	created_using = NULL;

	if (!pkgconfcli_serialize_object_add_string(object_list, "specVersion", SPDX3_SPECVERSION))
		goto err;

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_spdx3_core_creation_info: out of memory");

	free(cur_time);
	pkgconfcli_serialize_array_free(created_by);
	pkgconfcli_serialize_array_free(created_using);
	pkgconfcli_serialize_object_list_free(object_list);
	return ret;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_core_relationship_to_object(pkgconf_client_t *client, const sbomtool_core_relationship_t *relationship)
 *
 *    Serialize /Core/Relationship struct to a JSON value tree.
 *
 *    :param pkgconf_client_t *client: Pkgconf Client
 *    :param sbomtool_spdx3_document_t *document: SPDX document structure which hold needed information
 *    :param const char *from: Relation from SPDX ID
 *    :param pkgconfcli_serialize_array_t *to: Relation to array of SPDX ID
 *    :param const char *name: Name of relationship. Normally package name.
 *    :param const char *relationship_type: Which kind of relation (See SPDX 3.x documentation for valid strings)
 *    :param scope: Is this private relation or what. Can be NULL.
 *    :return: pkgconfcli_serialize_value_t * representing the Relationship object.
 */
pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_relationship(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *from, pkgconfcli_serialize_array_t *to, const char *name, const char *relationship_type, const char *scope)
{
	if (!document || !from || !to)
		return NULL;

	pkgconfcli_serialize_value_t *ret = NULL;
	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	pkgconf_buffer_t relationship_buf = PKGCONF_BUFFER_INITIALIZER;
	char *spdx_id = NULL;
	char sep = sbomtool_util_get_uri_separator(client);

	if (!object_list)
		goto err;

	if (!pkgconf_buffer_append_fmt(&relationship_buf, "%s%c%s", name, sep, relationship_type))
		goto err;

	spdx_id = sbomtool_util_get_sbom_id_string(client, "Relationship", pkgconf_buffer_str(&relationship_buf));

	if (!spdx_id)
		goto err;

	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", "Relationship") &&
	        pkgconfcli_serialize_object_add_string(object_list, "creationInfo", document->creation_info_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "spdxId", spdx_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "from", from)))
	{
		/* none of the above transfers ownership of `to` */
		goto err;
	}

	/* object_add_array always takes ownership of `to` (it is freed even on failure) */
	if (!pkgconfcli_serialize_object_add_array(object_list, "to", to))
		goto err;

	if (!pkgconfcli_serialize_object_add_string(object_list, "relationshipType", relationship_type))
		goto err;

	if (scope &&
	        !pkgconfcli_serialize_object_add_string(object_list, "scope", scope))
		goto err;

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_spdx3_core_relationship: out of memory");

	free(spdx_id);
	pkgconf_buffer_finalize(&relationship_buf);
	pkgconfcli_serialize_object_list_free(object_list);
	return ret;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *spdxtool_core_spdx_document_to_object(pkgconf_client_t *client, spdxtool_core_spdx_document_t *spdx)
 *
 *    Serialize /Core/SpdxDocument as a JSON value tree.
 *
 *    :param pkgconf_client_t *client: Pkgconf Client
 *    :param sbomtool_spdx3_document_t *document: SPDX document structure which hold needed information
 *    :return: pkgconfcli_serialize_value_t * representing the SpdxDocument object.
 */
pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_spdx_document(pkgconf_client_t *client, sbomtool_spdx3_document_t *document)
{
	pkgconfcli_serialize_value_t *ret = NULL;
	pkgconfcli_serialize_object_list_t *object_list = NULL;
	pkgconfcli_serialize_array_t *root_element_array = NULL;
	pkgconfcli_serialize_array_t *element_array = NULL;

	char *spdx_id = sbomtool_util_get_sbom_id_int(client, "spdxDocument");

	if (!spdx_id)
		goto err;

	object_list = pkgconfcli_serialize_object_list_new();
	if (!object_list)
		goto err;

	root_element_array = pkgconfcli_serialize_array_new();
	if (!root_element_array)
		goto err;

	document->root_elements = root_element_array;

	element_array = pkgconfcli_serialize_array_new();
	if (!element_array)
		goto err;

	document->elements = element_array;

	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", "SpdxDocument") &&
	        pkgconfcli_serialize_object_add_string(object_list, "creationInfo", document->creation_info_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "spdxId", spdx_id)))
		goto err;

	/* object_add_array always takes ownership of the array (it is freed even on
	 * failure), so clear our reference before checking the result to avoid a
	 * double free at the error label.
	 */
	ret = pkgconfcli_serialize_object_add_array(object_list, "rootElement", root_element_array);
	root_element_array = NULL;
	if (!ret)
		goto err;

	ret = pkgconfcli_serialize_object_add_array(object_list, "element", element_array);
	element_array = NULL;
	if (!ret)
		goto err;

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "spdxtool_spdx3_core_document: out of memory");

	pkgconfcli_serialize_object_list_free(object_list);
	pkgconfcli_serialize_array_free(root_element_array);
	pkgconfcli_serialize_array_free(element_array);
	free(spdx_id);

	return ret;
}
