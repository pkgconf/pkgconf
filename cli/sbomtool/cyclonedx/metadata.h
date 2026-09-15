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

#ifndef CLI__SBOMTOOL__CYCLONEX__METADATA_H
#define CLI__SBOMTOOL__CYCLONEX__METADATA_H

#ifdef __cplusplus
extern "C" {
#endif

bool
sbomtool_cyclonedx_metadata_add_timestamp(pkgconf_client_t *client, pkgconfcli_serialize_object_list_t *metadatas, const char *timestamp);

bool
sbomtool_cyclonedx_metadata_add_manufacturer(pkgconf_client_t *client, pkgconfcli_serialize_object_list_t *metadata, const char *name, const char *url);

bool
sbomtool_cyclonedx_metadata_add_tools(pkgconf_client_t *client, pkgconfcli_serialize_object_list_t *metadata);


#ifdef __cplusplus
}
#endif

#endif
