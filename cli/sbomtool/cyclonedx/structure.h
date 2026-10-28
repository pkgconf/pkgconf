/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2025 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include "serialize.h"

#ifndef CLI__SBOMTOOL__CYCLONEX__STRUCTURE_H
#define CLI__SBOMTOOL__CYCLONEX__STRUCTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#define CYCLONEDX_SPECVERSION "1.7"
#define CYCLONEDX_BOMFORMAT "CycloneDX"
#define CYCLONEDX_SCHEMA "http://cyclonedx.org/schema/bom-1.7.schema.json"

pkgconfcli_serialize_value_t *
sbomtool_cyclonedx_structure_new(pkgconf_client_t *client, int numversion);

pkgconfcli_serialize_value_t *
sbomtool_cyclonedx_structure_add_metadata(pkgconf_client_t *client, pkgconfcli_serialize_value_t *cyclonedx_doc, const char *timestamp, const char *manufacturer_name, const char *manufacturer_url, bool add_tool);

#ifdef __cplusplus
}
#endif

#endif
