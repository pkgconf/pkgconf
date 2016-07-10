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

static FILE *pkgconf_auditf = NULL;

void
pkgconf_audit_open_log(FILE *auditf)
{
	pkgconf_auditf = auditf;
}

void
pkgconf_audit_log(const char *format, ...)
{
	va_list va;

	if (pkgconf_auditf == NULL)
		return;

	va_start(va, format);
	vfprintf(pkgconf_auditf, format, va);
	va_end(va);
}

void
pkgconf_audit_log_dependency(const pkgconf_pkg_t *dep, const pkgconf_dependency_t *depnode)
{
	if (pkgconf_auditf == NULL)
		return;

	fprintf(pkgconf_auditf, "%s ", dep->id);
	if (depnode->version != NULL && depnode->compare != PKGCONF_CMP_ANY)
	{
		fprintf(pkgconf_auditf, "%s %s ", pkgconf_pkg_get_comparator(depnode), depnode->version);
	}

	fprintf(pkgconf_auditf, "[%s]\n", dep->version);
}
