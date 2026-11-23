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
#include <string.h>
#include "serialize.h"

#ifndef CLI__SBOMTOOL__SPDX3__STRUCTURE_H
#define CLI__SBOMTOOL__SPDX3__STRUCTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#define SPDX3_CONTEXT "https://spdx.org/rdf/3.0.1/spdx-context.jsonld"

typedef struct sbomtool_spdx3_document
{
	const char *creation_info_id;
	const char *timestamp;
	const char *agent_name;
	const char *agent_spdx_id;
	const char *tool_spdx_id;
	pkgconf_list_t licsenses;
	pkgconfcli_serialize_value_t *document_root;
	pkgconfcli_serialize_value_t *sbom_document;
	pkgconfcli_serialize_array_t *graph;
	pkgconfcli_serialize_array_t *documents;
	pkgconfcli_serialize_array_t *licenses;
	pkgconfcli_serialize_array_t *elements;
	pkgconfcli_serialize_array_t *root_elements;
	pkgconfcli_serialize_array_t *relationships;
	pkgconfcli_serialize_array_t *packages;
	pkgconfcli_serialize_array_t *software_sbom;
	pkgconfcli_serialize_array_t *maintainers;
} sbomtool_spdx3_document_t;

pkgconfcli_serialize_value_t *
sbomtool_spdx3_structure_new(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, bool is_tool);

#ifdef __cplusplus
}
#endif

#endif
