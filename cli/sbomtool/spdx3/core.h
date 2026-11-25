/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2026 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#ifndef CLI__SBOMTOOL__SPDX3__CORE_H
#define CLI__SBOMTOOL__SPDX3__CORE_H

#include <stdlib.h>
#include "util.h"
#include "serialize.h"
#include "spdx3/structure.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPDX3_SPECVERSION "3.0.1"

pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_agent(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *name);

pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_tool(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *short_name, const char *name);

pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_creation_info(pkgconf_client_t *client, sbomtool_spdx3_document_t *document);

pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_relationship(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *from, pkgconfcli_serialize_array_t *to, const char *name, const char *relationship_type, const char *scope);

pkgconfcli_serialize_value_t *
sbomtool_spdx3_core_spdx_document(pkgconf_client_t *client, sbomtool_spdx3_document_t *document);


#ifdef __cplusplus
}
#endif

#endif
