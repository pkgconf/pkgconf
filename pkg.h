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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
typedef struct fragment_ pkg_fragment_t;

#define foreach_list_entry(head, value) \
	for ((value) = (head); (value) != NULL; (value) = (value)->next)

#define LOCAL_COPY(a) \
	strcpy(alloca(strlen(a) + 1), a)

#ifndef MIN
# define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
# define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

struct fragment_ {
	struct fragment_ *prev, *next;

	char type;
	char *data;
};

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
	char *id;
	char *filename;
	char *realname;
	char *version;
	char *description;
	char *url;
	char *pc_filedir;

	pkg_fragment_t *libs;
	pkg_fragment_t *libs_private;
	pkg_fragment_t *cflags;

	pkg_dependency_t *requires;
	pkg_dependency_t *requires_private;
	pkg_dependency_t *conflicts;
	pkg_tuple_t *vars;
};

#define PKGF_NONE			0x0
#define PKGF_SEARCH_PRIVATE		0x1
#define PKGF_ENV_ONLY			0x2

#define PKG_ERRF_OK			0x0
#define PKG_ERRF_PACKAGE_NOT_FOUND	0x1
#define PKG_ERRF_PACKAGE_VER_MISMATCH	0x2

pkg_t *pkg_find(const char *name, unsigned int flags);
void pkg_traverse(pkg_t *root, void (*pkg_traverse_func)(pkg_t *package, void *data), void *data, int maxdepth, unsigned int flags);
void pkg_verify_graph(pkg_t *root, int depth, unsigned int flags);
int pkg_compare_version(const char *a, const char *b);
pkg_t *pkg_verify_dependency(pkg_dependency_t *pkgdep, unsigned int flags, unsigned int *eflags);
const char *pkg_get_comparator(pkg_dependency_t *pkgdep);

/* parse.c */
pkg_t *parse_file(const char *path, FILE *f);
char *tuple_find(pkg_tuple_t *head, const char *key);
pkg_dependency_t *parse_deplist(pkg_t *pkg, const char *depends);
pkg_dependency_t *pkg_dependency_append(pkg_dependency_t *head, pkg_dependency_t *tail);

/* argvsplit.c */
int argv_split(const char *src, int *argc, char ***argv);

/* fragment.c */
pkg_fragment_t *pkg_fragment_append(pkg_fragment_t *head, pkg_fragment_t *tail);
pkg_fragment_t *pkg_fragment_add(pkg_fragment_t *head, const char *string);
pkg_fragment_t *pkg_fragment_copy(pkg_fragment_t *head, pkg_fragment_t *base);
void pkg_fragment_delete(pkg_fragment_t *node);
bool pkg_fragment_exists(pkg_fragment_t *head, pkg_fragment_t *base);

/* fileio.c */
char *pkg_fgetline(char *line, size_t size, FILE *stream);

#endif
