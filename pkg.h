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
#include "iter.h"

#define PKG_BUFSIZE	(65535)

/* we are compatible with 0.28 of pkg-config */
#define PKG_PKGCONFIG_VERSION_EQUIV	"0.28"

typedef enum {
	PKG_ANY = 0,
	PKG_LESS_THAN,
	PKG_GREATER_THAN,
	PKG_LESS_THAN_EQUAL,
	PKG_GREATER_THAN_EQUAL,
	PKG_EQUAL,
	PKG_NOT_EQUAL,
	PKG_ALWAYS_MATCH,
	PKG_CMP_SIZE
} pkg_comparator_t;

typedef struct pkg_ pkg_t;
typedef struct pkg_dependency_ pkg_dependency_t;
typedef struct pkg_tuple_ pkg_tuple_t;
typedef struct pkg_fragment_ pkg_fragment_t;

#define PKG_FOREACH_LIST_ENTRY(head, value) \
	for ((value) = (head); (value) != NULL; (value) = (value)->next)

#define PKG_FOREACH_LIST_ENTRY_SAFE(head, nextiter, value) \
	for ((value) = (head), (nextiter) = (head) != NULL ? (head)->next : NULL; (value) != NULL; (value) = (nextiter), (nextiter) = (nextiter) != NULL ? (nextiter)->next : NULL)

#define PKG_FOREACH_LIST_ENTRY_REVERSE(tail, value) \
	for ((value) = (tail); (value) != NULL; (value) = (value)->prev)

#define PKG_MIN(a,b) (((a) < (b)) ? (a) : (b))
#define PKG_MAX(a,b) (((a) > (b)) ? (a) : (b))

struct pkg_fragment_ {
	pkg_node_t iter;

	char type;
	char *data;
};

struct pkg_dependency_ {
	pkg_node_t iter;

	char *package;
	pkg_comparator_t compare;
	char *version;
	pkg_t *parent;
};

struct pkg_tuple_ {
	pkg_node_t iter;

	char *key;
	char *value;
};

#define PKG_PROPF_NONE			0x0
#define PKG_PROPF_VIRTUAL		0x1
#define PKG_PROPF_CACHED		0x2
#define PKG_PROPF_SEEN			0x4
#define PKG_PROPF_UNINSTALLED		0x8

struct pkg_ {
	pkg_node_t cache_iter;

	int refcount;
	char *id;
	char *filename;
	char *realname;
	char *version;
	char *description;
	char *url;
	char *pc_filedir;

	pkg_list_t libs;
	pkg_list_t libs_private;
	pkg_list_t cflags;
	pkg_list_t cflags_private;

	pkg_list_t requires;
	pkg_list_t requires_private;
	pkg_list_t conflicts;

	pkg_list_t vars;

	unsigned int flags;
};

#define PKG_MODULE_SEPARATOR(c) ((c) == ',' || isspace ((c)))
#define PKG_OPERATOR_CHAR(c) ((c) == '<' || (c) == '>' || (c) == '!' || (c) == '=')

#define PKGF_NONE			0x000
#define PKGF_SEARCH_PRIVATE		0x001
#define PKGF_ENV_ONLY			0x002
#define PKGF_NO_UNINSTALLED		0x004
#define PKGF_SKIP_ROOT_VIRTUAL		0x008
#define PKGF_MERGE_PRIVATE_FRAGMENTS	0x010
#define PKGF_SKIP_CONFLICTS		0x020
#define PKGF_NO_CACHE			0x040
#define PKGF_MUNGE_SYSROOT_PREFIX	0x080
#define PKGF_SKIP_ERRORS		0x100
#define PKGF_ITER_PKG_IS_PRIVATE	0x200

#define PKG_ERRF_OK			0x0
#define PKG_ERRF_PACKAGE_NOT_FOUND	0x1
#define PKG_ERRF_PACKAGE_VER_MISMATCH	0x2
#define PKG_ERRF_PACKAGE_CONFLICT	0x4
#define PKG_ERRF_DEPGRAPH_BREAK		0x8

typedef void (*pkg_iteration_func_t)(const pkg_t *pkg);
typedef void (*pkg_traverse_func_t)(pkg_t *pkg, void *data, unsigned int flags);
typedef bool (*pkg_queue_apply_func_t)(pkg_t *world, void *data, int maxdepth, unsigned int flags);

/* pkg.c */
pkg_t *pkg_ref(pkg_t *pkg);
void pkg_unref(pkg_t *pkg);
void pkg_free(pkg_t *pkg);
pkg_t *pkg_find(const char *name, unsigned int flags);
void pkg_scan_all(pkg_iteration_func_t func);
unsigned int pkg_traverse(pkg_t *root, pkg_traverse_func_t func, void *data, int maxdepth, unsigned int flags);
unsigned int pkg_verify_graph(pkg_t *root, int depth, unsigned int flags);
int pkg_compare_version(const char *a, const char *b);
pkg_t *pkg_verify_dependency(pkg_dependency_t *pkgdep, unsigned int flags, unsigned int *eflags);
const char *pkg_get_comparator(pkg_dependency_t *pkgdep);
int pkg_cflags(pkg_t *root, pkg_list_t *list, int maxdepth, unsigned int flags);
int pkg_libs(pkg_t *root, pkg_list_t *list, int maxdepth, unsigned int flags);
pkg_comparator_t pkg_comparator_lookup_by_name(const char *name);

/* parse.c */
pkg_t *pkg_new_from_file(const char *path, FILE *f, unsigned int flags);
void pkg_dependency_parse_str(pkg_list_t *deplist_head, const char *depends);
void pkg_dependency_parse(pkg_t *pkg, pkg_list_t *deplist_head, const char *depends);
void pkg_dependency_append(pkg_list_t *list, pkg_dependency_t *tail);
void pkg_dependency_free(pkg_list_t *list);

/* argvsplit.c */
int pkg_argv_split(const char *src, int *argc, char ***argv);
void pkg_argv_free(char **argv);

/* fragment.c */
void pkg_fragment_parse(pkg_list_t *list, pkg_list_t *vars, const char *value, unsigned int flags);
void pkg_fragment_add(pkg_list_t *list, const char *string, unsigned int flags);
void pkg_fragment_copy(pkg_list_t *list, pkg_fragment_t *base, unsigned int flags, bool is_private);
void pkg_fragment_delete(pkg_list_t *list, pkg_fragment_t *node);
void pkg_fragment_free(pkg_list_t *list);

/* fileio.c */
char *pkg_fgetline(char *line, size_t size, FILE *stream);

/* tuple.c */
pkg_tuple_t *pkg_tuple_add(pkg_list_t *parent, const char *key, const char *value);
char *pkg_tuple_find(pkg_list_t *list, const char *key);
char *pkg_tuple_parse(pkg_list_t *list, const char *value);
void pkg_tuple_free(pkg_list_t *list);
void pkg_tuple_add_global(const char *key, const char *value);
char *pkg_tuple_find_global(const char *key);
void pkg_tuple_free_global(void);
void pkg_tuple_define_global(const char *kv);

/* main.c */
extern FILE *error_msgout;

/* queue.c */
void pkg_queue_push(pkg_list_t *list, const char *package);
bool pkg_queue_compile(pkg_t *world, pkg_list_t *list);
void pkg_queue_free(pkg_list_t *list);
bool pkg_queue_apply(pkg_list_t *list, pkg_queue_apply_func_t func, int maxdepth, unsigned int flags, void *data);
bool pkg_queue_validate(pkg_list_t *list, int maxdepth, unsigned int flags);

/* cache.c */
pkg_t *pkg_cache_lookup(const char *id);
void pkg_cache_add(pkg_t *pkg);
void pkg_cache_remove(pkg_t *pkg);
void pkg_cache_free(void);

#endif
