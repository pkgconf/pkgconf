/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2025 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#ifndef SPDXTOOL_GENERATE_H
#define SPDXTOOL_GENERATE_H

#include <stdio.h>
#include <libpkgconf/libpkgconf.h>
#include "serialize.h"

/*
 * Build an SPDX SBOM for a solved dependency graph and write it to *out*.
 *
 * The dependency graph rooted at *world* must already have been solved (e.g.
 * with pkgconf_queue_solve) so that every package's match is populated.  The
 * spdxtool_util_set_* configuration (URI root, separator, version, license)
 * must have been applied to *client* beforehand.
 */
bool spdxtool_generate(pkgconf_client_t *client, pkgconf_pkg_t *world, FILE *out,
	int maxdepth, const char *creation_time, const char *creation_id,
	const char *agent_name);


pkgconfcli_serialize_value_t *
spdxtool_generate_sbom(pkgconf_client_t *client, spdxtool_core_agent_t *agent, spdxtool_core_tool_t *tool, spdxtool_core_creation_info_t *creation, spdxtool_core_spdx_document_t *spdx);

#endif
