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

#ifndef CLI__SBOMTOOL__CYCLONEX__DEPENDENCIES_H
#define CLI__SBOMTOOL__CYCLONEX__DEPENDENCIES_H

#ifdef __cplusplus
extern "C" {
#endif

pkgconfcli_serialize_value_t *
sbomtool_cyclonedx_dependencies_new(pkgconf_client_t *client, const char *bom_ref, bool depends_on);

#ifdef __cplusplus
}
#endif

#endif
