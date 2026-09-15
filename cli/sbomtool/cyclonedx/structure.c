/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2026 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include "structure.h"
#include "metadata.h"

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_cyclonedx_structure_new(pkgconf_client_t *client, int numversion)
 *
 *    Create basic structure of CycloneDX document
 *
 *    :param pkgconf_client_t *client: Pkgconf Client
 *    :param int numversion: Current revision of document
 *    :return: pkgconfcli_serialize_value_t * which contains strucre of CycloneDX
 */
pkgconfcli_serialize_value_t *
sbomtool_cyclonedx_structure_new(pkgconf_client_t *client, int numversion)
{
	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	pkgconfcli_serialize_value_t *ret = NULL;

	if (!object_list)
	{
		goto err;
	}

	if (!(pkgconfcli_serialize_object_add_string(object_list, "$schema", CYCLONEDX_SCHEMA) &&
	        pkgconfcli_serialize_object_add_string(object_list, "bomFormat", CYCLONEDX_BOMFORMAT) &&
	        pkgconfcli_serialize_object_add_string(object_list, "specVersion", CYCLONEDX_SPECVERSION) &&
	        pkgconfcli_serialize_object_add_int(object_list, "specVersion", numversion)))
		goto err;

	if (!(pkgconfcli_serialize_object_add_object(object_list, "metadata", pkgconfcli_serialize_object_list_new()) &&
	        pkgconfcli_serialize_object_add_array(object_list, "components", pkgconfcli_serialize_array_new()) &&
	        pkgconfcli_serialize_object_add_array(object_list, "dependencies", pkgconfcli_serialize_array_new())))
		goto err;

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_cyclonedx_structure_new: out of memory");

	pkgconfcli_serialize_object_list_free(object_list);
	return ret;
}

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_cyclonedx_structure_add_metadata(pkgconf_client_t *client, pkgconfcli_serialize_value_t *cyclonedx_doc, const char *timestamp, const char *manufacturer_name, const char *manufacturer_url, bool add_tool)
 *
 *
 *    Create Metadata part to CycloneDX doc
 *
 *    :param pkgconf_client_t *client: Pkgconf Client struct
 *    :param pkgconfcli_serialize_value_t *cyclonedx_doc: Current revision of document
 *    :param const char *timestamp: Timestamp to be used or NULL if current is wanted
 *    :param const char *manufacturer_name: Manufacturer name
 *    :param const char *manufacturer_url: Manufacturer URL
 *    :param bool add_tool: Should tool info be added (if true it will be added to document)
 *    :return: pkgconfcli_serialize_value_t * which contains strucre of CycloneDX
 */
pkgconfcli_serialize_value_t *
sbomtool_cyclonedx_structure_add_metadata(pkgconf_client_t *client, pkgconfcli_serialize_value_t *cyclonedx_doc, const char *timestamp, const char *manufacturer_name, const char *manufacturer_url, bool add_tool)
{
	pkgconfcli_serialize_value_t *metadata = pkgconfcli_serialize_value_object_find(cyclonedx_doc, "metadata");
	pkgconfcli_serialize_object_list_t *object_list = NULL;

	if (!metadata || !manufacturer_name || !manufacturer_url || !pkgconfcli_serialize_is_value(metadata, PKGCONFCLI_SERIALIZE_TYPE_OBJECT))
		return NULL;

	object_list = pkgconfcli_serialize_value_get_object(metadata);

	if (!object_list)
		goto err;

	if (!sbomtool_cyclonedx_metadata_add_timestamp(client, object_list, timestamp))
		goto err;

	if(!sbomtool_cyclonedx_metadata_add_manufacturer(client, object_list, manufacturer_name, manufacturer_url))
		goto err;

	if (add_tool)
		if (!sbomtool_cyclonedx_metadata_add_tools(client, object_list))
			goto err;

	return metadata;
err:
	pkgconf_error(client, "sbomtool_cyclonedx_structure_add_metadata: out of memory");

	pkgconfcli_serialize_object_list_free(object_list);

	return NULL;
}
