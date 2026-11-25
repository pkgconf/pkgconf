/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2026 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include <libpkgconfcli/serialize.h>
#include "spdx3/simplelicensing.h"
#include "util.h"


/*
 * !doc
 *
 * .. c:function:: pkgconfcli_serialize_value_t *sbomtool_spdx3_simplelicensing_licenseExpression(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *spdx_expression)
 *
 *    Serialize /SimpleLicensing/LicenseExpression as a JSON value tree.
 *
 *    :param pkgconf_client_t *client: The pkgconf client being accessed.
 *    :param sbomtool_spdx3_core_spdx_document_t *spdx: SpdxDocument to which generated relationships are added.
 *    :param const char *spdx_license_expression: SDPX license expression or short SPDX license string
 *    :return: pkgconfcli_serialize_value_t * representing the LicenseExpression object.
 */
pkgconfcli_serialize_value_t *
sbomtool_spdx3_simplelicensing_licenseExpression(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *spdx_license_expression)
{
	pkgconfcli_serialize_value_t *ret = NULL;
	pkgconfcli_serialize_object_list_t *object_list = pkgconfcli_serialize_object_list_new();
	char *spdx_id = sbomtool_util_get_sbom_id_string(client, "simplelicensing_LicenseExpression", spdx_license_expression);

	if (!object_list || !spdx_id)
		goto err;

	if (!(pkgconfcli_serialize_object_add_string(object_list, "type", "simplelicensing_LicenseExpression") &&
	        pkgconfcli_serialize_object_add_string(object_list, "creationInfo", document->creation_info_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "spdxId", spdx_id) &&
	        pkgconfcli_serialize_object_add_string(object_list, "simplelicensing_licenseExpression", spdx_license_expression)))
		goto err;

	ret = pkgconfcli_serialize_value_object(object_list);
	object_list = NULL;

err:
	if (!ret)
		pkgconf_error(client, "sbomtool_spdx3_simplelicensing_licenseExpression_to_object: out of memory");

	free(spdx_id);

	pkgconfcli_serialize_object_list_free(object_list);
	return ret;
}
