/*
 * pkg.h
 * Global include file for everything.
 *
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __PKG_H
#define __PKG_H

#include "config.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BUFSIZ
#define BUFSIZ	65535
#endif

typedef enum {
	PKG_ANY = 0,
	PKG_LESS_THAN,
	PKG_GREATER_THAN,
	PKG_LESS_THAN_EQUAL,
	PKG_GREATER_THAN_EQUAL,
	PKG_EQUAL,
	PKG_NOT_EQUAL,
	PKG_ALWAYS_MATCH
} pkg_comparator_t;

typedef struct pkg_ pkg_t;
typedef struct dependency_ pkg_dependency_t;
typedef struct tuple_ pkg_tuple_t;

#define foreach_list_entry(head, value) \
	for ((value) = (head); (value) != NULL; (value) = (value)->next)

struct dependency_ {
	struct dependency_ *prev, *next;

	char *package;
	pkg_comparator_t compare;
	char *version;
	pkg_t *parent;
};

struct tuple_ {
	struct tuple_ *prev, *next;

	char *key;
	char *value;
};

struct pkg_ {
	char *filename;
	char *realname;
	char *version;
	char *description;
	char *url;
	char *pc_filedir;
	char *libs;
	char *cflags;

	pkg_dependency_t *requires;
	pkg_dependency_t *conflicts;
	pkg_tuple_t *vars;
};

pkg_t *pkg_find(const char *name);
void pkg_traverse(pkg_t *root, void (*pkg_traverse_func)(pkg_t *package, void *data), void *data, int maxdepth);

/* parse.c */
pkg_t *parse_file(const char *path);
char *tuple_find(pkg_tuple_t *head, const char *key);
pkg_dependency_t *parse_deplist(pkg_t *pkg, const char *depends);

#endif
