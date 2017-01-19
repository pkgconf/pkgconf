/*
 * libpkgconf.h
 * Global include file for everything in libpkgconf.
 *
 * Copyright (c) 2011, 2015 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#ifndef LIBPKGCONF__LIBPKGCONF_H
#define LIBPKGCONF__LIBPKGCONF_H

#include <libpkgconf/stdinc.h>
#include <libpkgconf/iter.h>
#include <libpkgconf/bsdstubs.h>

/* pkg-config uses ';' on win32 as ':' is part of path */
#ifdef _WIN32
#define PKG_CONFIG_PATH_SEP_S   ";"
#else
#define PKG_CONFIG_PATH_SEP_S   ":"
#endif

#ifdef _WIN32
#define PKG_DIR_SEP_S   '\\'
#else
#define PKG_DIR_SEP_S   '/'
#endif

#define PKGCONF_BUFSIZE	(65535)

typedef enum {
	PKGCONF_CMP_NOT_EQUAL,
	PKGCONF_CMP_ANY,
	PKGCONF_CMP_LESS_THAN,
	PKGCONF_CMP_LESS_THAN_EQUAL,
	PKGCONF_CMP_EQUAL,
	PKGCONF_CMP_GREATER_THAN,
	PKGCONF_CMP_GREATER_THAN_EQUAL
} pkgconf_pkg_comparator_t;

#define PKGCONF_CMP_COUNT 7

typedef struct pkgconf_pkg_ pkgconf_pkg_t;
typedef struct pkgconf_dependency_ pkgconf_dependency_t;
typedef struct pkgconf_tuple_ pkgconf_tuple_t;
typedef struct pkgconf_fragment_ pkgconf_fragment_t;
typedef struct pkgconf_path_ pkgconf_path_t;
typedef struct pkgconf_client_ pkgconf_client_t;

#define PKGCONF_ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define PKGCONF_FOREACH_LIST_ENTRY(head, value) \
	for ((value) = (head); (value) != NULL; (value) = (value)->next)

#define PKGCONF_FOREACH_LIST_ENTRY_SAFE(head, nextiter, value) \
	for ((value) = (head), (nextiter) = (head) != NULL ? (head)->next : NULL; (value) != NULL; (value) = (nextiter), (nextiter) = (nextiter) != NULL ? (nextiter)->next : NULL)

#define PKGCONF_FOREACH_LIST_ENTRY_REVERSE(tail, value) \
	for ((value) = (tail); (value) != NULL; (value) = (value)->prev)

struct pkgconf_fragment_ {
	pkgconf_node_t iter;

	char type;
	char *data;
};

struct pkgconf_dependency_ {
	pkgconf_node_t iter;

	char *package;
	pkgconf_pkg_comparator_t compare;
	char *version;
	pkgconf_pkg_t *parent;
};

struct pkgconf_tuple_ {
	pkgconf_node_t iter;

	char *key;
	char *value;
};

struct pkgconf_path_ {
	pkgconf_node_t lnode;

	char *path;
	void *handle_path;
	void *handle_device;
};

#define PKGCONF_PKG_PROPF_NONE			0x0
#define PKGCONF_PKG_PROPF_VIRTUAL		0x1
#define PKGCONF_PKG_PROPF_CACHED		0x2
#define PKGCONF_PKG_PROPF_SEEN			0x4
#define PKGCONF_PKG_PROPF_UNINSTALLED		0x8

struct pkgconf_pkg_ {
	pkgconf_node_t cache_iter;

	int refcount;
	char *id;
	char *filename;
	char *realname;
	char *version;
	char *description;
	char *url;
	char *pc_filedir;

	pkgconf_list_t libs;
	pkgconf_list_t libs_private;
	pkgconf_list_t cflags;
	pkgconf_list_t cflags_private;

	pkgconf_list_t requires;
	pkgconf_list_t requires_private;
	pkgconf_list_t conflicts;
	pkgconf_list_t provides;

	pkgconf_list_t vars;

	unsigned int flags;
};

typedef bool (*pkgconf_pkg_iteration_func_t)(const pkgconf_pkg_t *pkg, void *data);
typedef void (*pkgconf_pkg_traverse_func_t)(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data, unsigned int flags);
typedef bool (*pkgconf_queue_apply_func_t)(pkgconf_client_t *client, pkgconf_pkg_t *world, void *data, int maxdepth);
typedef bool (*pkgconf_error_handler_func_t)(const char *msg, const pkgconf_client_t *client, const void *data);

struct pkgconf_client_ {
	pkgconf_list_t dir_list;
	pkgconf_list_t pkg_cache;

	pkgconf_list_t filter_libdirs;
	pkgconf_list_t filter_includedirs;

	pkgconf_list_t global_vars;

	void *error_handler_data;
	pkgconf_error_handler_func_t error_handler;

	FILE *auditf;

	char *sysroot_dir;
	char *buildroot_dir;

	unsigned int flags;
};

/* client.c */
void pkgconf_client_init(pkgconf_client_t *client, pkgconf_error_handler_func_t error_handler, void *error_handler_data);
pkgconf_client_t *pkgconf_client_new(pkgconf_error_handler_func_t error_handler, void *error_handler_data);
void pkgconf_client_deinit(pkgconf_client_t *client);
void pkgconf_client_free(pkgconf_client_t *client);
const char *pkgconf_client_get_sysroot_dir(const pkgconf_client_t *client);
void pkgconf_client_set_sysroot_dir(pkgconf_client_t *client, const char *sysroot_dir);
const char *pkgconf_client_get_buildroot_dir(const pkgconf_client_t *client);
void pkgconf_client_set_buildroot_dir(pkgconf_client_t *client, const char *buildroot_dir);
unsigned int pkgconf_client_get_flags(const pkgconf_client_t *client);
void pkgconf_client_set_flags(pkgconf_client_t *client, unsigned int flags);

#define PKGCONF_IS_MODULE_SEPARATOR(c) ((c) == ',' || isspace ((unsigned int)(c)))
#define PKGCONF_IS_OPERATOR_CHAR(c) ((c) == '<' || (c) == '>' || (c) == '!' || (c) == '=')

#define PKGCONF_PKG_PKGF_NONE				0x000
#define PKGCONF_PKG_PKGF_SEARCH_PRIVATE			0x001
#define PKGCONF_PKG_PKGF_ENV_ONLY			0x002
#define PKGCONF_PKG_PKGF_NO_UNINSTALLED			0x004
#define PKGCONF_PKG_PKGF_SKIP_ROOT_VIRTUAL		0x008
#define PKGCONF_PKG_PKGF_MERGE_PRIVATE_FRAGMENTS	0x010
#define PKGCONF_PKG_PKGF_SKIP_CONFLICTS			0x020
#define PKGCONF_PKG_PKGF_NO_CACHE			0x040
#define PKGCONF_PKG_PKGF_SKIP_ERRORS			0x080
#define PKGCONF_PKG_PKGF_ITER_PKG_IS_PRIVATE		0x100
#define PKGCONF_PKG_PKGF_SKIP_PROVIDES			0x200

#define PKGCONF_PKG_ERRF_OK			0x0
#define PKGCONF_PKG_ERRF_PACKAGE_NOT_FOUND	0x1
#define PKGCONF_PKG_ERRF_PACKAGE_VER_MISMATCH	0x2
#define PKGCONF_PKG_ERRF_PACKAGE_CONFLICT	0x4
#define PKGCONF_PKG_ERRF_DEPGRAPH_BREAK		0x8

/* pkg.c */
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#define PRINTFLIKE(fmtarg, firstvararg) \
        __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#define DEPRECATED \
        __attribute__((deprecated))
#else
#define PRINTFLIKE(fmtarg, firstvararg)
#define DEPRECATED
#endif /* defined(__INTEL_COMPILER) || defined(__GNUC__) */

bool pkgconf_error(const pkgconf_client_t *client, const char *format, ...) PRINTFLIKE(2, 3);
bool pkgconf_default_error_handler(const char *msg, const pkgconf_client_t *client, const void *data);

pkgconf_pkg_t *pkgconf_pkg_ref(const pkgconf_client_t *client, pkgconf_pkg_t *pkg);
void pkgconf_pkg_unref(pkgconf_client_t *client, pkgconf_pkg_t *pkg);
void pkgconf_pkg_free(pkgconf_client_t *client, pkgconf_pkg_t *pkg);
pkgconf_pkg_t *pkgconf_pkg_find(pkgconf_client_t *client, const char *name, unsigned int flags);
unsigned int pkgconf_pkg_traverse(pkgconf_client_t *client, pkgconf_pkg_t *root, pkgconf_pkg_traverse_func_t func, void *data, int maxdepth, unsigned int flags);
unsigned int pkgconf_pkg_verify_graph(pkgconf_client_t *client, pkgconf_pkg_t *root, int depth, unsigned int flags);
pkgconf_pkg_t *pkgconf_pkg_verify_dependency(pkgconf_client_t *client, pkgconf_dependency_t *pkgdep, unsigned int flags, unsigned int *eflags);
const char *pkgconf_pkg_get_comparator(const pkgconf_dependency_t *pkgdep);
unsigned int pkgconf_pkg_cflags(pkgconf_client_t *client, pkgconf_pkg_t *root, pkgconf_list_t *list, int maxdepth, unsigned int flags);
unsigned int pkgconf_pkg_libs(pkgconf_client_t *client, pkgconf_pkg_t *root, pkgconf_list_t *list, int maxdepth, unsigned int flags);
pkgconf_pkg_comparator_t pkgconf_pkg_comparator_lookup_by_name(const char *name);
pkgconf_pkg_t *pkgconf_builtin_pkg_get(const char *name);

int pkgconf_compare_version(const char *a, const char *b);
pkgconf_pkg_t *pkgconf_scan_all(pkgconf_client_t *client, void *ptr, pkgconf_pkg_iteration_func_t func);
void pkgconf_pkg_dir_list_build(pkgconf_client_t *client, unsigned int flags);

/* parse.c */
pkgconf_pkg_t *pkgconf_pkg_new_from_file(const pkgconf_client_t *client, const char *path, FILE *f);
void pkgconf_dependency_parse_str(pkgconf_list_t *deplist_head, const char *depends);
void pkgconf_dependency_parse(const pkgconf_client_t *client, pkgconf_pkg_t *pkg, pkgconf_list_t *deplist_head, const char *depends);
void pkgconf_dependency_append(pkgconf_list_t *list, pkgconf_dependency_t *tail);
void pkgconf_dependency_free(pkgconf_list_t *list);
pkgconf_dependency_t *pkgconf_dependency_add(pkgconf_list_t *list, const char *package, const char *version, pkgconf_pkg_comparator_t compare);

/* argvsplit.c */
int pkgconf_argv_split(const char *src, int *argc, char ***argv);
void pkgconf_argv_free(char **argv);

/* fragment.c */
typedef bool (*pkgconf_fragment_filter_func_t)(const pkgconf_client_t *client, const pkgconf_fragment_t *frag, void *data, unsigned int flags);
void pkgconf_fragment_parse(const pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_list_t *vars, const char *value);
void pkgconf_fragment_add(const pkgconf_client_t *client, pkgconf_list_t *list, const char *string);
void pkgconf_fragment_copy(pkgconf_list_t *list, const pkgconf_fragment_t *base, unsigned int flags, bool is_private);
void pkgconf_fragment_delete(pkgconf_list_t *list, pkgconf_fragment_t *node);
void pkgconf_fragment_free(pkgconf_list_t *list);
void pkgconf_fragment_filter(const pkgconf_client_t *client, pkgconf_list_t *dest, pkgconf_list_t *src, pkgconf_fragment_filter_func_t filter_func, void *data, unsigned int flags);
size_t pkgconf_fragment_render_len(const pkgconf_list_t *list);
void pkgconf_fragment_render_buf(const pkgconf_list_t *list, char *buf, size_t len);
char *pkgconf_fragment_render(const pkgconf_list_t *list);
bool pkgconf_fragment_has_system_dir(const pkgconf_client_t *client, const pkgconf_fragment_t *frag);

/* fileio.c */
char *pkgconf_fgetline(char *line, size_t size, FILE *stream);

/* tuple.c */
pkgconf_tuple_t *pkgconf_tuple_add(const pkgconf_client_t *client, pkgconf_list_t *parent, const char *key, const char *value, bool parse);
char *pkgconf_tuple_find(const pkgconf_client_t *client, pkgconf_list_t *list, const char *key);
char *pkgconf_tuple_parse(const pkgconf_client_t *client, pkgconf_list_t *list, const char *value);
void pkgconf_tuple_free(pkgconf_list_t *list);
void pkgconf_tuple_free_entry(pkgconf_tuple_t *tuple, pkgconf_list_t *list);
void pkgconf_tuple_add_global(pkgconf_client_t *client, const char *key, const char *value);
char *pkgconf_tuple_find_global(const pkgconf_client_t *client, const char *key);
void pkgconf_tuple_free_global(pkgconf_client_t *client);
void pkgconf_tuple_define_global(pkgconf_client_t *client, const char *kv);

/* queue.c */
void pkgconf_queue_push(pkgconf_list_t *list, const char *package);
bool pkgconf_queue_compile(pkgconf_client_t *client, pkgconf_pkg_t *world, pkgconf_list_t *list);
void pkgconf_queue_free(pkgconf_list_t *list);
bool pkgconf_queue_apply(pkgconf_client_t *client, pkgconf_list_t *list, pkgconf_queue_apply_func_t func, int maxdepth, void *data);
bool pkgconf_queue_validate(pkgconf_client_t *client, pkgconf_list_t *list, int maxdepth);

/* cache.c */
pkgconf_pkg_t *pkgconf_cache_lookup(const pkgconf_client_t *client, const char *id);
void pkgconf_cache_add(pkgconf_client_t *client, pkgconf_pkg_t *pkg);
void pkgconf_cache_remove(pkgconf_client_t *client, pkgconf_pkg_t *pkg);
void pkgconf_cache_free(pkgconf_client_t *client);

/* audit.c */
void pkgconf_audit_set_log(pkgconf_client_t *client, FILE *auditf);
void pkgconf_audit_log(pkgconf_client_t *client, const char *format, ...) PRINTFLIKE(2, 3);
void pkgconf_audit_log_dependency(pkgconf_client_t *client, const pkgconf_pkg_t *dep, const pkgconf_dependency_t *depnode);

/* path.c */
void pkgconf_path_add(const char *text, pkgconf_list_t *dirlist, bool filter);
size_t pkgconf_path_split(const char *text, pkgconf_list_t *dirlist, bool filter);
size_t pkgconf_path_build_from_environ(const char *environ, const char *fallback, pkgconf_list_t *dirlist, bool filter);
bool pkgconf_path_match_list(const char *path, const pkgconf_list_t *dirlist);
void pkgconf_path_free(pkgconf_list_t *dirlist);
bool pkgconf_path_relocate(char *buf, size_t buflen);

#endif
