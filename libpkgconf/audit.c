/*
 * audit.c
 * package audit log functions
 *
 * Copyright (c) 2016 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include <libpkgconf/libpkgconf.h>

void
pkgconf_audit_set_log(pkgconf_client_t *client, FILE *auditf)
{
	client->auditf = auditf;
}

void
pkgconf_audit_log(pkgconf_client_t *client, const char *format, ...)
{
	va_list va;

	if (client->auditf == NULL)
		return;

	va_start(va, format);
	vfprintf(client->auditf, format, va);
	va_end(va);
}

void
pkgconf_audit_log_dependency(pkgconf_client_t *client, const pkgconf_pkg_t *dep, const pkgconf_dependency_t *depnode)
{
	if (client->auditf == NULL)
		return;

	fprintf(client->auditf, "%s ", dep->id);
	if (depnode->version != NULL && depnode->compare != PKGCONF_CMP_ANY)
	{
		fprintf(client->auditf, "%s %s ", pkgconf_pkg_get_comparator(depnode), depnode->version);
	}

	fprintf(client->auditf, "[%s]\n", dep->version);
}
