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
#include <libpkgconf/config.h>
#include "component.h"
#include "metadata.h"
#include "util.h"

/*
 * !doc
 *
 * .. c:function:: bool sbomtool_cyclonedx_metadata_add_timestamp(pkgconf_client_t *client, pkgconfcli_serialize_object_list_t *metadata, const char *timestamp)
 *
 *    Add timestamp to metadata
 *
 *    :param const pkgconf_client_t *client: Pkgconf client struct
 *    :param const pkgconfcli_serialize_object_list_t *metadata: Metadata object struct
 *    :param const char *timestamp: Timestamp string or NULL if current stamp if wanted
 *    :return: True if success and false if not.
 */
bool
sbomtool_cyclonedx_metadata_add_timestamp(pkgconf_client_t *client, pkgconfcli_serialize_object_list_t *metadata, const char *timestamp)
{
	if (!client || !metadata)
		return false;

	bool rtn = true;
	char *cur_time = sbomtool_util_get_current_iso8601_time();
	const char *ptr_timestamp = timestamp ? timestamp : cur_time;

	if (!pkgconfcli_serialize_object_add_string(metadata, "timestamp", ptr_timestamp))
		rtn = false;

	free(cur_time);

	if (!rtn)
		pkgconf_error(client, "sbomtool_cyclonedx_metadata_add_timestamp: out of memory");

	return rtn;
}

/*
 * !doc
 *
 * .. c:function:: bool sbomtool_cyclonedx_metadata_add_manufacturer(pkgconf_client_t *client, pkgconfcli_serialize_object_list_t *metadata, const char *name, const char *url)
 *
 *    Add manufacturer info to metadata
 *
 *    :param const pkgconf_client_t *client: Pkgconf client struct
 *    :param const pkgconfcli_serialize_object_list_t *metadata: Metadata object struct
 *    :param const char *name: Name of manufacturer
 *    :param const char *url: URL for manufacturer
 *    :return: True if success and false if not.
 */
bool
sbomtool_cyclonedx_metadata_add_manufacturer(pkgconf_client_t *client, pkgconfcli_serialize_object_list_t *metadata, const char *name, const char *url)
{
	if (!client || !metadata || !name || !url)
		return false;

	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	pkgconfcli_serialize_array_t *url_array = pkgconfcli_serialize_array_new();
	char *bom_ref = NULL;

	if (!object_list || !url_array)
		goto err;

	bom_ref = sbomtool_util_get_sbom_id_string(client, "metadata", name);

	if (!bom_ref)
		goto err;

	if (!pkgconfcli_serialize_object_add_string(object_list, "bom-ref", bom_ref))
		goto err;

	free(bom_ref);
	bom_ref = NULL;

	if(!pkgconfcli_serialize_object_add_string(object_list, "name", name))
		goto err;

	if(!(pkgconfcli_serialize_object_add_array(object_list, "url", url_array) &&
	        pkgconfcli_serialize_array_add_string(url_array, url)))
		goto err;

	pkgconfcli_serialize_object_add_object(metadata, "manufacturer", object_list);
	return true;

err:
	pkgconf_error(client, "sbomtool_cyclonedx_metadata_add_manufacturer: out of memory");

	free(bom_ref);
	pkgconfcli_serialize_array_free(url_array);
	pkgconfcli_serialize_object_list_free(object_list);

	return false;
}

/*
 * !doc
 *
 * .. c:function:: bool sbomtool_cyclonedx_metadata_add_tools(pkgconf_client_t *client, pkgconfcli_serialize_object_list_t *metadata)
 *
 *    Add tool info to metadata
 *
 *    :param const pkgconf_client_t *client: Pkgconf client struct
 *    :param const pkgconfcli_serialize_object_list_t *metadata: Metadata object struct
 *    :return: True if success and false if not.
 */
bool
sbomtool_cyclonedx_metadata_add_tools(pkgconf_client_t *client, pkgconfcli_serialize_object_list_t *metadata)
{
	if (!client || !metadata)
		return false;

	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	pkgconfcli_serialize_array_t *add_array = pkgconfcli_serialize_array_new();
	pkgconfcli_serialize_array_t *ext_array = pkgconfcli_serialize_array_new();

	if(!object_list || !add_array || !ext_array)
		goto err;

	if (!pkgconfcli_serialize_object_add_array(metadata, "tools", add_array))
		goto err;

	if (!pkgconfcli_serialize_array_add_object(add_array, object_list))
		goto err;

	if (!(pkgconfcli_serialize_object_add_string(object_list, "vendor", "pkgconf") &&
	        pkgconfcli_serialize_object_add_string(object_list, "name", PACKAGE_NAME) &&
	        pkgconfcli_serialize_object_add_string(object_list, "version", PACKAGE_VERSION)))
		goto err;

	if (!pkgconfcli_serialize_object_add_array(object_list, "externalReferences", ext_array))
		goto err;


	sbomtool_cyclondex_component_add_externalReference(client, ext_array, CYCLONEX_COMPONENT_EXT_VCS, "https://github.com/pkgconf/pkgconf");
	sbomtool_cyclondex_component_add_externalReference(client, ext_array, CYCLONEX_COMPONENT_EXT_ISSUE_TRACKER, "https://github.com/pkgconf/pkgconf/issues");
	sbomtool_cyclondex_component_add_externalReference(client, ext_array, CYCLONEX_COMPONENT_EXT_MAILING_LIST, "https://lists.sr.ht/~kaniini/pkgconf");

	return true;
err:
	pkgconf_error(client, "sbomtool_cyclonedx_metadata_add_tools: out of memory");
	pkgconfcli_serialize_array_free(add_array);
	pkgconfcli_serialize_array_free(ext_array);
	pkgconfcli_serialize_object_list_free(object_list);

	return false;
}
