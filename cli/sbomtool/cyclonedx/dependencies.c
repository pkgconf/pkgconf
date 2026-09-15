/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2026 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include "dependencies.h"

/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t * sbomtool_cyclonedx_dependencies_new(pkgconf_client_t *client, const char *bom_ref, bool depends_on)*
 *
 *    Create dependency
 *
 *    :param pkgconf_client_t *client: The creationInfo ID string to embed in the object.
 *    :param const char *bom_ref: Bom-ref which this dependency referenceses
 *    :param bool depends_on: Should depends_on be added (If there is dependencies to be claimed it should be true)
 *    :return: pkgconfcli_serialize_value_t * which represents dependency.
 */
pkgconfcli_serialize_value_t *
sbomtool_cyclonedx_dependencies_new(pkgconf_client_t *client, const char *bom_ref, bool depends_on)
{
	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	pkgconfcli_serialize_value_t *ret = NULL;

	if (!object_list || !bom_ref)
		goto err;

	if (!pkgconfcli_serialize_object_add_string(object_list, "ref", bom_ref))
		goto err;

	if (depends_on)
	{
		if (!pkgconfcli_serialize_object_add_array(object_list, "dependsOn", pkgconfcli_serialize_array_new()))
			goto err;
	}

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_cyclonedx_dependencies_new: out of memory");

	pkgconfcli_serialize_object_list_free(object_list);
	return ret;
}
