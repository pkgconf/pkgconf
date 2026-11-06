/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2025 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#ifndef CLI__SBOMTOOL__UTIL_H
#define CLI__SBOMTOOL__UTIL_H

#include <libpkgconf/libpkgconf.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SBOMTOOL_SEPARATOR_COLON	0x80

void
sbomtool_util_set_key(pkgconf_client_t *client, const char *key, const char *key_value, const char *key_default);

void
sbomtool_util_set_uri_root(pkgconf_client_t *client, const char *uri_root);

void
sbomtool_util_set_uri_separator_colon(pkgconf_client_t *client, bool sep);

char
sbomtool_util_get_uri_separator(pkgconf_client_t *client);

const char *
sbomtool_util_get_uri_root(pkgconf_client_t *client);

void
sbomtool_util_set_sbom_version(pkgconf_client_t *client, const char *sbom_version);

const char *
sbomtool_util_get_sbom_version(pkgconf_client_t *client);

void
sbomtool_util_set_sbom_license(pkgconf_client_t *client, const char *sbom_license);

char *
sbomtool_util_get_sbom_id_int(pkgconf_client_t *client, const char *part);

char *
sbomtool_util_get_sbom_id_string(pkgconf_client_t *client, const char *part, const char *string_id);

char *
sbomtool_util_get_iso8601_time(time_t *wanted_time);

char *
sbomtool_util_get_current_iso8601_time(void);

char *
sbomtool_util_string_correction(char *str);

char *
sbomtool_util_tuple_lookup(pkgconf_client_t *client, pkgconf_list_t *vars, const char *key);

#ifdef __cplusplus
}
#endif

#endif
