/*
 * pkg.h
 * Global include file for everything.
 *
 * Copyright (c) 2011 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#ifndef __PKG_H
#define __PKG_H

#include "config.h"
#include "stdinc.h"

#define PKG_BUFSIZE	(65535)

/* we are compatible with 0.27 of pkg-config */
#define PKG_PKGCONFIG_VERSION_EQUIV	"0.27"

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

typedef struct pkg_queue_ {
	struct pkg_queue_ *prev, *next;
	char *package;
} pkg_queue_t;

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

#define PKGF_NONE			0x00
#define PKGF_SEARCH_PRIVATE		0x01
#define PKGF_ENV_ONLY			0x02
#define PKGF_NO_UNINSTALLED		0x04
#define PKGF_SKIP_ROOT_VIRTUAL		0x08
#define PKGF_MERGE_PRIVATE_FRAGMENTS	0x10
#define PKGF_SKIP_CONFLICTS		0x20

#define PKG_ERRF_OK			0x0
#define PKG_ERRF_PACKAGE_NOT_FOUND	0x1
#define PKG_ERRF_PACKAGE_VER_MISMATCH	0x2
#define PKG_ERRF_PACKAGE_CONFLICT	0x4
#define PKG_ERRF_DEPGRAPH_BREAK		0x8

typedef void (*pkg_iteration_func_t)(const pkg_t *pkg);
typedef void (*pkg_traverse_func_t)(pkg_t *pkg, void *data, unsigned int flags);
typedef bool (*pkg_queue_apply_func_t)(pkg_t *world, void *data, int maxdepth, unsigned int flags);

/* pkg.c */
void pkg_free(pkg_t *pkg);
pkg_t *pkg_find(const char *name, unsigned int flags);
void pkg_scan(const char *search_path, pkg_iteration_func_t func);
void pkg_scan_all(pkg_iteration_func_t func);
unsigned int pkg_traverse(pkg_t *root, pkg_traverse_func_t func, void *data, int maxdepth, unsigned int flags);
unsigned int pkg_verify_graph(pkg_t *root, int depth, unsigned int flags);
int pkg_compare_version(const char *a, const char *b);
pkg_t *pkg_verify_dependency(pkg_dependency_t *pkgdep, unsigned int flags, unsigned int *eflags);
const char *pkg_get_comparator(pkg_dependency_t *pkgdep);
int pkg_cflags(pkg_t *root, pkg_fragment_t **list, int maxdepth, unsigned int flags);
int pkg_libs(pkg_t *root, pkg_fragment_t **list, int maxdepth, unsigned int flags);

/* parse.c */
pkg_t *pkg_new_from_file(const char *path, FILE *f);
pkg_dependency_t *pkg_dependency_parse_str(pkg_dependency_t *deplist_head, const char *depends);
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

/* main.c */
extern FILE *error_msgout;

/* queue.c */
pkg_queue_t *pkg_queue_push(pkg_queue_t *parent, const char *package);
bool pkg_queue_compile(pkg_t *world, pkg_queue_t *head);
void pkg_queue_free(pkg_queue_t *head);
bool pkg_queue_apply(pkg_queue_t *head, pkg_queue_apply_func_t func, int maxdepth, unsigned int flags, void *data);
bool pkg_queue_validate(pkg_queue_t *head, int maxdepth, unsigned int flags);

#endif
