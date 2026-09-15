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
	char *ptr_timestamp = (char *)timestamp;

	if (!ptr_timestamp)
	{
		ptr_timestamp = sbomtool_util_get_current_iso8601_time();
		if (!ptr_timestamp)
			return false;
	}

	if (!pkgconfcli_serialize_object_add_string(metadata, "timestamp", ptr_timestamp))
	{
		rtn = false;
	}

	if (!timestamp)
		free(ptr_timestamp);

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

	pkgconfcli_serialize_object_list_t *object_list = NULL;
	pkgconfcli_serialize_array_t *url_array = NULL;
	char *bom_ref = NULL;

	object_list = pkgconfcli_serialize_object_list_new();

	if (!object_list)
		goto err;


	bom_ref = sbomtool_util_get_sbom_id_string(client, "metadata", name);

	if (!bom_ref)
		goto err;

	if (!pkgconfcli_serialize_object_add_string(object_list, "bom-ref", bom_ref))
		goto err;

	free(bom_ref);
	bom_ref = NULL;

	if (!pkgconfcli_serialize_object_add_string(object_list, "name", name))
		goto err;

	url_array = pkgconfcli_serialize_array_new();

	if (!url_array)
		goto err;

	if (!pkgconfcli_serialize_object_add_array(object_list, "url", url_array))
		goto err;

	if (!pkgconfcli_serialize_array_add_string(url_array, url))
		goto err;

	if (!pkgconfcli_serialize_object_add_object(metadata, "manufacturer", object_list))
	{
		object_list = NULL;
		goto err;
	}

	return true;

err:
	pkgconf_error(client, "sbomtool_cyclonedx_metadata_add_manufacturer: out of memory");

	free(bom_ref);
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

	pkgconfcli_serialize_object_list_t *object_list = NULL;
	pkgconfcli_serialize_array_t *add_array = NULL;
	pkgconfcli_serialize_array_t *ext_array = NULL;


	object_list = pkgconfcli_serialize_object_list_new();

	if (!object_list)
	{
		goto err;
	}

	if (!(pkgconfcli_serialize_object_add_string(object_list, "vendor", "pkgconf") &&
	      pkgconfcli_serialize_object_add_string(object_list, "name", PACKAGE_NAME) &&
	      pkgconfcli_serialize_object_add_string(object_list, "version", PACKAGE_VERSION)))
	{
		goto err;
	}

	ext_array = pkgconfcli_serialize_array_new();

	if (!ext_array)
		goto err;

	if (!sbomtool_cyclondex_component_add_externalReference(client, ext_array, CYCLONEX_COMPONENT_EXT_VCS, "https://github.com/pkgconf/pkgconf"))
		goto err;

	if (!sbomtool_cyclondex_component_add_externalReference(client, ext_array, CYCLONEX_COMPONENT_EXT_ISSUE_TRACKER, "https://github.com/pkgconf/pkgconf/issues"))
		goto err;

	if (!sbomtool_cyclondex_component_add_externalReference(client, ext_array, CYCLONEX_COMPONENT_EXT_MAILING_LIST, "https://lists.sr.ht/~kaniini/pkgconf"))
		goto err;

	if (!pkgconfcli_serialize_object_add_array(object_list, "externalReferences", ext_array))
	{
		ext_array = NULL;
		goto err;
	}

	ext_array = NULL;

	add_array = pkgconfcli_serialize_array_new();

	if (!add_array)
	{
		goto err;
	}

	if (!pkgconfcli_serialize_array_add_object(add_array, object_list))
	{
		object_list = NULL;
		goto err;
	}

	object_list = NULL;

	if (!pkgconfcli_serialize_object_add_array(metadata, "tools", add_array))
	{
		add_array = NULL;
		goto err;
	}

	return true;
err:
	pkgconf_error(client, "sbomtool_cyclonedx_metadata_add_tools: out of memory");
	pkgconfcli_serialize_object_list_free(object_list);
	pkgconfcli_serialize_array_free(ext_array);
	pkgconfcli_serialize_array_free(add_array);

	return false;
}
