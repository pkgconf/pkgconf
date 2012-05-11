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
#include "stdinc.h"

#ifndef BUFSIZ
#define BUFSIZ	65535
#endif

/* we are compatible with 0.26 of pkg-config */
#define PKG_PKGCONFIG_VERSION_EQUIV	"0.26"

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
typedef struct pkg_dependency_ pkg_dependency_t;
typedef struct pkg_tuple_ pkg_tuple_t;
typedef struct pkg_fragment_ pkg_fragment_t;

#define PKG_FOREACH_LIST_ENTRY(head, value) \
	for ((value) = (head); (value) != NULL; (value) = (value)->next)

#define PKG_FOREACH_LIST_ENTRY_SAFE(head, nextiter, value) \
	for ((value) = (head), (nextiter) = (head) != NULL ? (head)->next : NULL; (value) != NULL; (value) = (nextiter), (nextiter) = (nextiter) != NULL ? (nextiter)->next : NULL)

#define PKG_LOCAL_COPY(a) \
	strcpy(alloca(strlen(a) + 1), a)

#define PKG_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define PKG_MAX(a,b) (((a) > (b)) ? (a) : (b))

struct pkg_fragment_ {
	struct pkg_fragment_ *prev, *next;

	char type;
	char *data;
};

struct pkg_dependency_ {
	struct pkg_dependency_ *prev, *next;

	char *package;
	pkg_comparator_t compare;
	char *version;
	pkg_t *parent;
};

struct pkg_tuple_ {
	struct pkg_tuple_ *prev, *next;

	char *key;
	char *value;
};

#define PKG_PROPF_NONE			0x0
#define PKG_PROPF_VIRTUAL		0x1

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

	bool uninstalled;

	unsigned int flags;
};

#define PKG_MODULE_SEPARATOR(c) ((c) == ',' || isspace ((c)))
#define PKG_OPERATOR_CHAR(c) ((c) == '<' || (c) == '>' || (c) == '!' || (c) == '=')

#define PKGF_NONE			0x0
#define PKGF_SEARCH_PRIVATE		0x1
#define PKGF_ENV_ONLY			0x2
#define PKGF_NO_UNINSTALLED		0x4
#define PKGF_SKIP_ROOT_VIRTUAL		0x8

#define PKG_ERRF_OK			0x0
#define PKG_ERRF_PACKAGE_NOT_FOUND	0x1
#define PKG_ERRF_PACKAGE_VER_MISMATCH	0x2

/* pkg.c */
void pkg_free(pkg_t *pkg);
pkg_t *pkg_find(const char *name, unsigned int flags);
unsigned int pkg_traverse(pkg_t *root, void (*pkg_traverse_func)(pkg_t *package, void *data), void *data, int maxdepth, unsigned int flags);
unsigned int pkg_verify_graph(pkg_t *root, int depth, unsigned int flags);
int pkg_compare_version(const char *a, const char *b);
pkg_t *pkg_verify_dependency(pkg_dependency_t *pkgdep, unsigned int flags, unsigned int *eflags);
const char *pkg_get_comparator(pkg_dependency_t *pkgdep);

/* parse.c */
pkg_t *pkg_new_from_file(const char *path, FILE *f);
pkg_dependency_t *pkg_dependency_parse(pkg_t *pkg, const char *depends);
pkg_dependency_t *pkg_dependency_append(pkg_dependency_t *head, pkg_dependency_t *tail);
void pkg_dependency_free(pkg_dependency_t *head);

/* argvsplit.c */
int pkg_argv_split(const char *src, int *argc, char ***argv);
void pkg_argv_free(char **argv);

/* fragment.c */
pkg_fragment_t *pkg_fragment_parse(pkg_fragment_t *head, pkg_tuple_t *vars, const char *value);
pkg_fragment_t *pkg_fragment_append(pkg_fragment_t *head, pkg_fragment_t *tail);
pkg_fragment_t *pkg_fragment_add(pkg_fragment_t *head, const char *string);
pkg_fragment_t *pkg_fragment_copy(pkg_fragment_t *head, pkg_fragment_t *base);
void pkg_fragment_delete(pkg_fragment_t *node);
bool pkg_fragment_exists(pkg_fragment_t *head, pkg_fragment_t *base);
void pkg_fragment_free(pkg_fragment_t *head);

/* fileio.c */
char *pkg_fgetline(char *line, size_t size, FILE *stream);

/* tuple.c */
pkg_tuple_t *pkg_tuple_add(pkg_tuple_t *parent, const char *key, const char *value);
char *pkg_tuple_find(pkg_tuple_t *head, const char *key);
char *pkg_tuple_parse(pkg_tuple_t *vars, const char *value);
void pkg_tuple_free(pkg_tuple_t *head);
void pkg_tuple_add_global(const char *key, const char *value);
char *pkg_tuple_find_global(const char *key);
void pkg_tuple_free_global(void);
void pkg_tuple_define_global(const char *kv);

#endif
