/*
* SPDX-License-Identifier: BSD-2-Clause
*
*​ Copyright (c) 2026 The FreeBSD Foundation
*​
*​ Portions of this software were developed by
* Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
* the FreeBSD Foundation
*/

#include <libpkgconf/config.h>
#include <libpkgconfcli/serialize.h>
#include "spdx3/structure.h"
#include "spdx3/core.h"

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_spdx3_structure_new(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, bool is_tool)
 *
 *    Create basic structure of SDPX 3.x document
 *
 *    :param pkgconf_client_t *client: Pkgconf Client
 *    :param sbomtool_spdx3_document_t *document: SPDX document structure which hold needed information
 *    :param bool is_tool: Should tool info be added. True it should and false it shouldn't
 *    :return: pkgconfcli_serialize_value_t * which contains strucre of CycloneDX
 */
pkgconfcli_serialize_value_t *
sbomtool_spdx3_structure_new(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, bool is_tool)
{
	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	pkgconfcli_serialize_value_t *agent = NULL;
	pkgconfcli_serialize_value_t *tool = NULL;
	pkgconfcli_serialize_value_t *agent_value = NULL;
	pkgconfcli_serialize_value_t *tool_value = NULL;

	if (!object_list)
		goto err;

	if (!pkgconfcli_serialize_object_add_string(object_list, "@context", SPDX3_CONTEXT))
		goto err;

	document->graph = pkgconfcli_serialize_array_new();

	if (!document->graph)
		goto err;

	if (!pkgconfcli_serialize_object_add_array(object_list, "@graph", document->graph))
		goto err;

	agent = sbomtool_spdx3_core_agent(client, document, document->agent_name);

	if (!pkgconfcli_serialize_array_add_take(document->graph, agent))
		goto err;

	agent_value = pkgconfcli_serialize_value_object_find(agent, "spdxId");

	if (!agent_value || !pkgconfcli_serialize_is_value(agent_value, PKGCONFCLI_SERIALIZE_TYPE_STRING))
		goto err;

	document->agent_spdx_id = pkgconfcli_serialize_value_get_string(agent_value);

	if (is_tool)
	{
		tool = sbomtool_spdx3_core_tool(client, document, PACKAGE_NAME, "sbomtool");

		if (!pkgconfcli_serialize_array_add_take(document->graph, tool))
			goto err;

		tool_value = pkgconfcli_serialize_value_object_find(tool, "spdxId");

		if (!tool_value || !pkgconfcli_serialize_is_value(tool_value, PKGCONFCLI_SERIALIZE_TYPE_STRING))
			goto err;

		document->tool_spdx_id = pkgconfcli_serialize_value_get_string(tool_value);
	}

	if (!pkgconfcli_serialize_array_add_take(document->graph,
	        sbomtool_spdx3_core_creation_info(client,
	                document)))
		goto err;

	document->document_root = pkgconfcli_serialize_value_object(object_list);

	object_list = NULL;

	if (!document->document_root)
		goto err;

err:
	if (!document->document_root)
		pkgconf_error(client, "sbomtool_spdx3_structure_new: out of memory");

	pkgconfcli_serialize_object_list_free(object_list);
	return document->document_root;
}
