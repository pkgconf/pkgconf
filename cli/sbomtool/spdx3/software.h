/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2025 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#ifndef CLI__SBOMTOOL__SPDX3_SOFTWARE_H
#define CLI__SBOMTOOL__SPDX3_SOFTWARE_H

#include <stdlib.h>
#include "util.h"
#include "spdx3/structure.h"

#ifdef __cplusplus
extern "C" {
#endif

pkgconfcli_serialize_value_t *
sbomtool_spdx3_software_package(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, pkgconf_pkg_t *pkg, pkgconfcli_serialize_value_t *sbom);

pkgconfcli_serialize_value_t *
sbomtool_spdx3_software_sbom(pkgconf_client_t *client, sbomtool_spdx3_document_t *document, const char *name);

void
sbomtool_spdx3_software_add_package(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *ptr, unsigned int iter_flags);


#ifdef __cplusplus
}
#endif

#endif
