/*
 * bytecode.c
 * variable expansion bytecode evaluator
 *
 * Copyright (c) 2026 pkgconf authors (see AUTHORS).
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

/*
 * !doc
 *
 * libpkgconf `bytecode` module
 * ============================
 *
 * The libpkgconf `bytecode` module contains the functions related to
 * evaluating variable expansion bytecode.
 */

static bool
pkgconf_bytecode_eval_internal(pkgconf_bytecode_eval_ctx_t *ctx,
	const pkgconf_bytecode_t *bc,
	pkgconf_buffer_t *out,
	bool *saw_sysroot)
{
	(void) ctx;

	if (bc == NULL || out == NULL)
		return false;

	const uint8_t *p = bc->base;
	const uint8_t *end = bc->base + bc->len;

	while (p < end)
	{
		const pkgconf_bytecode_op_t *op =
			(const pkgconf_bytecode_op_t *)p;

		if ((const uint8_t *)op + sizeof(*op) > end)
			return false;

		if ((const uint8_t *)op + sizeof(*op) + op->size > end)
			return false;

		switch (op->tag)
		{
		case PKGCONF_BYTECODE_OP_TEXT:
			pkgconf_buffer_append_slice(out, op->data, op->size);
			break;

		case PKGCONF_BYTECODE_OP_VAR:
			break;

		case PKGCONF_BYTECODE_OP_SYSROOT:
			if (saw_sysroot != NULL)
				*saw_sysroot = true;
			break;

		default:
			/* reserved/unimplemented */
			return false;
		}

		p = (const uint8_t *)pkgconf_bytecode_op_next(op);
	}

	return true;
}

bool
pkgconf_bytecode_eval(pkgconf_client_t *client,
	const pkgconf_list_t *tuples,
	const pkgconf_bytecode_t *bc,
	pkgconf_buffer_t *out,
	bool *saw_sysroot)
{
	if (client == NULL || bc == NULL || out == NULL)
		return false;

	pkgconf_bytecode_eval_ctx_t ctx = {
		.client = client,
		.tuples = tuples,
	};

	if (saw_sysroot != NULL)
		*saw_sysroot = false;

	return pkgconf_bytecode_eval_internal(&ctx, bc, out, saw_sysroot);
}
