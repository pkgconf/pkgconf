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
#include "util.h"
#include "spdx3/structure.h"

#ifndef CLI__SBOMTOOL__SPDX3__SIMPLELICENSING_H
#define CLI__SBOMTOOL__SPDX3__SIMPLELICENSING_H


#ifdef __cplusplus
extern "C" {
#endif

pkgconfcli_serialize_value_t *
sbomtool_spdx3_simplelicensing_licenseExpression(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *spdx_license_expression);

#ifdef __cplusplus
}
#endif

#endif
