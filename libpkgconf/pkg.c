/*
 * pkg.c
 * higher-level dependency graph compilation, management and manipulation
 *
 * Copyright (c) 2011, 2012, 2013 pkgconf authors (see AUTHORS).
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

bool
pkgconf_error(const pkgconf_client_t *client, const char *format, ...)
{
	char errbuf[PKGCONF_BUFSIZE];
	va_list va;

	va_start(va, format);
	vsnprintf(errbuf, sizeof errbuf, format, va);
	va_end(va);

	return client->error_handler(errbuf, client, client->error_handler_data);
}

bool
pkgconf_default_error_handler(const char *msg, const pkgconf_client_t *client, const void *data)
{
	(void) msg;
	(void) client;
	(void) data;

	return true;
}

#ifdef _WIN32
#	define PKG_CONFIG_REG_KEY "Software\\pkgconfig\\PKG_CONFIG_PATH"
#	undef PKG_DEFAULT_PATH
#	define PKG_DEFAULT_PATH "../lib/pkgconfig;../share/pkgconfig"
#endif

#define PKG_CONFIG_EXT		".pc"
#define PKG_CONFIG_PATH_SZ	(65535)

static inline bool
str_has_suffix(const char *str, const char *suffix)
{
	size_t str_len = strlen(str);
	size_t suf_len = strlen(suffix);

	if (str_len < suf_len)
		return false;

	return !strncasecmp(str + str_len - suf_len, suffix, suf_len);
}

static inline const char *
get_default_pkgconfig_path(void)
{
#ifdef _WIN32
	static char outbuf[MAX_PATH];
	char namebuf[MAX_PATH];
	char *p;

	int sizepath = GetModuleFileName(NULL, namebuf, sizeof namebuf);
	char * winslash;
	namebuf[sizepath] = '\0';
	while ((winslash = strchr (namebuf, '\\')) != NULL)
	{
		*winslash = '/';
	}
	p = strrchr(namebuf, '/');
	if (p == NULL)
		return PKG_DEFAULT_PATH;

	*p = '\0';
	pkgconf_strlcpy(outbuf, namebuf, sizeof outbuf);
	pkgconf_strlcat(outbuf, "/", sizeof outbuf);
	pkgconf_strlcat(outbuf, "../lib/pkgconfig", sizeof outbuf);
	pkgconf_strlcat(outbuf, ";", sizeof outbuf);
	pkgconf_strlcat(outbuf, namebuf, sizeof outbuf);
	pkgconf_strlcat(outbuf, "/", sizeof outbuf);
	pkgconf_strlcat(outbuf, "../share/pkgconfig", sizeof outbuf);

	return outbuf;
#endif

	return PKG_DEFAULT_PATH;
}

static const char *
pkg_get_parent_dir(pkgconf_pkg_t *pkg)
{
	static char buf[PKGCONF_BUFSIZE];
	char *pathbuf;

	pkgconf_strlcpy(buf, pkg->filename, sizeof buf);
	pathbuf = strrchr(buf, PKG_DIR_SEP_S);
	if (pathbuf == NULL)
		pathbuf = strrchr(buf, '/');
	if (pathbuf != NULL)
		pathbuf[0] = '\0';

	return buf;
}

void
pkgconf_pkg_dir_list_build(pkgconf_client_t *client, unsigned int flags)
{
	if (client->dir_list.head != NULL || client->dir_list.tail != NULL)
		return;

	pkgconf_path_build_from_environ("PKG_CONFIG_PATH", NULL, &client->dir_list);

	if (!(flags & PKGCONF_PKG_PKGF_ENV_ONLY))
		pkgconf_path_build_from_environ("PKG_CONFIG_LIBDIR", get_default_pkgconfig_path(), &client->dir_list);
}

typedef void (*pkgconf_pkg_parser_keyword_func_t)(const pkgconf_client_t *client, pkgconf_pkg_t *pkg, const ptrdiff_t offset, char *value);
typedef struct {
	const char *keyword;
	const pkgconf_pkg_parser_keyword_func_t func;
	const ptrdiff_t offset;
} pkgconf_pkg_parser_keyword_pair_t;

static int pkgconf_pkg_parser_keyword_pair_cmp(const void *key, const void *ptr)
{
	const pkgconf_pkg_parser_keyword_pair_t *pair = ptr;
	return strcasecmp(key, pair->keyword);
}

static void
pkgconf_pkg_parser_tuple_func(const pkgconf_client_t *client, pkgconf_pkg_t *pkg, const ptrdiff_t offset, char *value)
{
	char **dest = ((void *) pkg + offset);
	*dest = pkgconf_tuple_parse(client, &pkg->vars, value);
}

static void
pkgconf_pkg_parser_fragment_func(const pkgconf_client_t *client, pkgconf_pkg_t *pkg, const ptrdiff_t offset, char *value)
{
	pkgconf_list_t *dest = ((void *) pkg + offset);
	pkgconf_fragment_parse(client, dest, &pkg->vars, value);
}

static void
pkgconf_pkg_parser_dependency_func(const pkgconf_client_t *client, pkgconf_pkg_t *pkg, const ptrdiff_t offset, char *value)
{
	pkgconf_list_t *dest = ((void *) pkg + offset);
	pkgconf_dependency_parse(client, pkg, dest, value);
}

/* keep this in alphabetical order */
static const pkgconf_pkg_parser_keyword_pair_t pkgconf_pkg_parser_keyword_funcs[] = {
	{"CFLAGS", pkgconf_pkg_parser_fragment_func, offsetof(pkgconf_pkg_t, cflags)},
	{"CFLAGS.private", pkgconf_pkg_parser_fragment_func, offsetof(pkgconf_pkg_t, cflags_private)},
	{"Conflicts", pkgconf_pkg_parser_dependency_func, offsetof(pkgconf_pkg_t, conflicts)},
	{"Description", pkgconf_pkg_parser_tuple_func, offsetof(pkgconf_pkg_t, description)},
	{"LIBS", pkgconf_pkg_parser_fragment_func, offsetof(pkgconf_pkg_t, libs)},
	{"LIBS.private", pkgconf_pkg_parser_fragment_func, offsetof(pkgconf_pkg_t, libs_private)},
	{"Name", pkgconf_pkg_parser_tuple_func, offsetof(pkgconf_pkg_t, realname)},
	{"Provides", pkgconf_pkg_parser_dependency_func, offsetof(pkgconf_pkg_t, provides)},
	{"Requires", pkgconf_pkg_parser_dependency_func, offsetof(pkgconf_pkg_t, requires)},
	{"Requires.private", pkgconf_pkg_parser_dependency_func, offsetof(pkgconf_pkg_t, requires_private)},
	{"Version", pkgconf_pkg_parser_tuple_func, offsetof(pkgconf_pkg_t, version)},
};

static bool
pkgconf_pkg_parser_keyword_set(const pkgconf_client_t *client, pkgconf_pkg_t *pkg, const char *keyword, char *value)
{
	const pkgconf_pkg_parser_keyword_pair_t *pair = bsearch(keyword,
		pkgconf_pkg_parser_keyword_funcs, PKGCONF_ARRAY_SIZE(pkgconf_pkg_parser_keyword_funcs),
		sizeof(pkgconf_pkg_parser_keyword_pair_t), pkgconf_pkg_parser_keyword_pair_cmp);

	if (pair == NULL || pair->func == NULL)
		return false;

	pair->func(client, pkg, pair->offset, value);
	return true;
}

/*
 * pkgconf_pkg_new_from_file(client, filename, file)
 *
 * Parse a .pc file into a pkgconf_pkg_t object structure.
 */
pkgconf_pkg_t *
pkgconf_pkg_new_from_file(const pkgconf_client_t *client, const char *filename, FILE *f)
{
	pkgconf_pkg_t *pkg;
	char readbuf[PKGCONF_BUFSIZE];
	char *idptr;

	pkg = calloc(sizeof(pkgconf_pkg_t), 1);
	pkg->filename = strdup(filename);
	pkgconf_tuple_add(client, &pkg->vars, "pcfiledir", pkg_get_parent_dir(pkg), true);

	/* make module id */
	if ((idptr = strrchr(pkg->filename, PKG_DIR_SEP_S)) != NULL)
		idptr++;
	else
		idptr = pkg->filename;

	pkg->id = strdup(idptr);
	idptr = strrchr(pkg->id, '.');
	if (idptr)
		*idptr = '\0';

	while (pkgconf_fgetline(readbuf, PKGCONF_BUFSIZE, f) != NULL)
	{
		char op, *p, *key, *value;

		p = readbuf;
		while (*p && (isalpha((unsigned int)*p) || isdigit((unsigned int)*p) || *p == '_' || *p == '.'))
			p++;

		key = readbuf;
		if (!isalpha((unsigned int)*key) && !isdigit((unsigned int)*p))
			continue;

		while (*p && isspace((unsigned int)*p)) {
			/* set to null to avoid trailing spaces in key */
			*p = '\0';
			p++;
		}

		op = *p;
		*p = '\0';
		p++;

		while (*p && isspace((unsigned int)*p))
			p++;

		value = p;

		switch (op)
		{
		case ':':
			pkgconf_pkg_parser_keyword_set(client, pkg, key, value);
			break;
		case '=':
			pkgconf_tuple_add(client, &pkg->vars, key, value, true);
			break;
		default:
			break;
		}
	}

	fclose(f);

	pkgconf_dependency_add(&pkg->provides, pkg->id, pkg->version, PKGCONF_CMP_EQUAL);

	return pkgconf_pkg_ref(client, pkg);
}

void
pkgconf_pkg_free(pkgconf_client_t *client, pkgconf_pkg_t *pkg)
{
	if (pkg == NULL || pkg->flags & PKGCONF_PKG_PROPF_VIRTUAL)
		return;

	pkgconf_cache_remove(client, pkg);

	pkgconf_dependency_free(&pkg->requires);
	pkgconf_dependency_free(&pkg->requires_private);
	pkgconf_dependency_free(&pkg->conflicts);
	pkgconf_dependency_free(&pkg->provides);

	pkgconf_fragment_free(&pkg->cflags);
	pkgconf_fragment_free(&pkg->cflags_private);
	pkgconf_fragment_free(&pkg->libs);
	pkgconf_fragment_free(&pkg->libs_private);

	pkgconf_tuple_free(&pkg->vars);

	if (pkg->id != NULL)
		free(pkg->id);

	if (pkg->filename != NULL)
		free(pkg->filename);

	if (pkg->realname != NULL)
		free(pkg->realname);

	if (pkg->version != NULL)
		free(pkg->version);

	if (pkg->description != NULL)
		free(pkg->description);

	if (pkg->url != NULL)
		free(pkg->url);

	if (pkg->pc_filedir != NULL)
		free(pkg->pc_filedir);

	free(pkg);
}

pkgconf_pkg_t *
pkgconf_pkg_ref(const pkgconf_client_t *client, pkgconf_pkg_t *pkg)
{
	(void) client;

	pkg->refcount++;
	return pkg;
}

void
pkgconf_pkg_unref(pkgconf_client_t *client, pkgconf_pkg_t *pkg)
{
	pkg->refcount--;
	if (pkg->refcount <= 0)
		pkgconf_pkg_free(client, pkg);
}

static inline pkgconf_pkg_t *
pkgconf_pkg_try_specific_path(const pkgconf_client_t *client, const char *path, const char *name, unsigned int flags)
{
	pkgconf_pkg_t *pkg = NULL;
	FILE *f;
	char locbuf[PKG_CONFIG_PATH_SZ];
	char uninst_locbuf[PKG_CONFIG_PATH_SZ];

	snprintf(locbuf, sizeof locbuf, "%s/%s" PKG_CONFIG_EXT, path, name);
	snprintf(uninst_locbuf, sizeof uninst_locbuf, "%s/%s-uninstalled" PKG_CONFIG_EXT, path, name);

	if (!(flags & PKGCONF_PKG_PKGF_NO_UNINSTALLED) && (f = fopen(uninst_locbuf, "r")) != NULL)
	{
		pkg = pkgconf_pkg_new_from_file(client, uninst_locbuf, f);
		pkg->flags |= PKGCONF_PKG_PROPF_UNINSTALLED;
	}
	else if ((f = fopen(locbuf, "r")) != NULL)
		pkg = pkgconf_pkg_new_from_file(client, locbuf, f);

	return pkg;
}

static pkgconf_pkg_t *
pkgconf_pkg_scan_dir(pkgconf_client_t *client, const char *path, void *data, pkgconf_pkg_iteration_func_t func)
{
	DIR *dir;
	struct dirent *dirent;
	pkgconf_pkg_t *outpkg = NULL;

	dir = opendir(path);
	if (dir == NULL)
		return NULL;

	for (dirent = readdir(dir); dirent != NULL; dirent = readdir(dir))
	{
		static char filebuf[PKGCONF_BUFSIZE];
		pkgconf_pkg_t *pkg;
		FILE *f;
		struct stat st;

		pkgconf_strlcpy(filebuf, path, sizeof filebuf);
		pkgconf_strlcat(filebuf, "/", sizeof filebuf);
		pkgconf_strlcat(filebuf, dirent->d_name, sizeof filebuf);

		stat(filebuf, &st);
		if (!(S_ISREG(st.st_mode)))
			continue;

		f = fopen(filebuf, "r");
		if (f == NULL)
			continue;

		pkg = pkgconf_pkg_new_from_file(client, filebuf, f);
		if (pkg != NULL)
		{
			if (func(pkg, data))
			{
				outpkg = pkg;
				goto out;
			}

			pkgconf_pkg_unref(client, pkg);
		}
	}

out:
	closedir(dir);
	return outpkg;
}

pkgconf_pkg_t *
pkgconf_scan_all(pkgconf_client_t *client, void *data, pkgconf_pkg_iteration_func_t func)
{
	pkgconf_node_t *n;
	pkgconf_pkg_t *pkg;

	PKGCONF_FOREACH_LIST_ENTRY(client->dir_list.head, n)
	{
		pkgconf_path_t *pnode = n->data;

		if ((pkg = pkgconf_pkg_scan_dir(client, pnode->path, data, func)) != NULL)
			return pkg;
	}

	return NULL;
}

#ifdef _WIN32
pkgconf_pkg_t *
pkgconf_pkg_find_in_registry_key(HKEY hkey, const char *name, unsigned int flags)
{
	pkgconf_pkg_t *pkg = NULL;

	HKEY key;
	int i = 0;

	char buf[16384]; /* per registry limits */
	DWORD bufsize = sizeof buf;
	if (RegOpenKeyEx(hkey, PKG_CONFIG_REG_KEY,
				0, KEY_READ, &key) != ERROR_SUCCESS)
		return NULL;

	while (RegEnumValue(key, i++, buf, &bufsize, NULL, NULL, NULL, NULL)
			== ERROR_SUCCESS)
	{
		char pathbuf[PKG_CONFIG_PATH_SZ];
		DWORD type;
		DWORD pathbuflen = sizeof pathbuf;

		if (RegQueryValueEx(key, buf, NULL, &type, (LPBYTE) pathbuf, &pathbuflen)
				== ERROR_SUCCESS && type == REG_SZ)
		{
			pkg = pkgconf_pkg_try_specific_path(pathbuf, name, flags);
			if (pkg != NULL)
				break;
		}

		bufsize = sizeof buf;
	}

	RegCloseKey(key);
	return pkg;
}
#endif

pkgconf_pkg_t *
pkgconf_pkg_find(pkgconf_client_t *client, const char *name, unsigned int flags)
{
	pkgconf_pkg_t *pkg = NULL;
	pkgconf_node_t *n;
	FILE *f;

	/* name might actually be a filename. */
	if (str_has_suffix(name, PKG_CONFIG_EXT))
	{
		if ((f = fopen(name, "r")) != NULL)
		{
			pkgconf_pkg_t *pkg;

			pkg = pkgconf_pkg_new_from_file(client, name, f);
			pkgconf_path_add(pkg_get_parent_dir(pkg), &client->dir_list);

			return pkg;
		}
	}

	/* check builtins */
	if ((pkg = pkgconf_builtin_pkg_get(name)) != NULL)
		return pkg;

	/* check cache */
	if (!(flags & PKGCONF_PKG_PKGF_NO_CACHE))
	{
		if ((pkg = pkgconf_cache_lookup(client, name)) != NULL)
		{
			pkg->flags |= PKGCONF_PKG_PROPF_CACHED;
			return pkg;
		}
	}

	PKGCONF_FOREACH_LIST_ENTRY(client->dir_list.head, n)
	{
		pkgconf_path_t *pnode = n->data;

		pkg = pkgconf_pkg_try_specific_path(client, pnode->path, name, flags);
		if (pkg != NULL)
			goto out;
	}

#ifdef _WIN32
	/* support getting PKG_CONFIG_PATH from registry */
	pkg = pkgconf_pkg_find_in_registry_key(HKEY_CURRENT_USER, name, flags);
	if (!pkg)
		pkg = pkgconf_pkg_find_in_registry_key(HKEY_LOCAL_MACHINE, name, flags);
#endif

out:
	pkgconf_cache_add(client, pkg);

	return pkg;
}

/*
 * pkgconf_compare_version(a, b)
 *
 * compare versions using RPM version comparison rules as described in the LSB.
 */
int
pkgconf_compare_version(const char *a, const char *b)
{
	char oldch1, oldch2;
	char buf1[PKGCONF_BUFSIZE], buf2[PKGCONF_BUFSIZE];
	char *str1, *str2;
	char *one, *two;
	int ret;
	bool isnum;

	/* optimization: if version matches then it's the same version. */
	if (a == NULL)
		return 1;

	if (b == NULL)
		return -1;

	if (!strcasecmp(a, b))
		return 0;

	pkgconf_strlcpy(buf1, a, sizeof buf1);
	pkgconf_strlcpy(buf2, b, sizeof buf2);

	one = str1 = buf1;
	two = str2 = buf2;

	while (*one || *two)
	{
		while (*one && !isalnum((unsigned int)*one) && *one != '~')
			one++;
		while (*two && !isalnum((unsigned int)*two) && *two != '~')
			two++;

		if (*one == '~' || *two == '~')
		{
			if (*one != '~')
				return -1;
			if (*two != '~')
				return 1;

			one++;
			two++;
			continue;
		}

		if (!(*one && *two))
			break;

		str1 = one;
		str2 = two;

		if (isdigit((unsigned int)*str1))
		{
			while (*str1 && isdigit((unsigned int)*str1))
				str1++;

			while (*str2 && isdigit((unsigned int)*str2))
				str2++;

			isnum = true;
		}
		else
		{
			while (*str1 && isalpha((unsigned int)*str1))
				str1++;

			while (*str2 && isalpha((unsigned int)*str2))
				str2++;

			isnum = false;
		}

		oldch1 = *str1;
		oldch2 = *str2;

		*str1 = '\0';
		*str2 = '\0';

		if (one == str1)
			return -1;

		if (two == str2)
			return (isnum ? 1 : -1);

		if (isnum)
		{
			int onelen, twolen;

			while (*one == '0')
				one++;

			while (*two == '0')
				two++;

			onelen = strlen(one);
			twolen = strlen(two);

			if (onelen > twolen)
				return 1;
			else if (twolen > onelen)
				return -1;
		}

		ret = strcmp(one, two);
		if (ret)
			return ret;

		*str1 = oldch1;
		*str2 = oldch2;

		one = str1;
		two = str2;
	}

	if ((!*one) && (!*two))
		return 0;

	if (!*one)
		return -1;

	return 1;
}

static pkgconf_pkg_t pkg_config_virtual = {
	.id = "pkg-config",
	.realname = "pkg-config",
	.description = "virtual package defining pkg-config API version supported",
	.url = PACKAGE_BUGREPORT,
	.version = PACKAGE_VERSION,
	.flags = PKGCONF_PKG_PROPF_VIRTUAL,
	.vars = {
		.head = &(pkgconf_node_t){
			.prev = NULL,
			.next = NULL,
			.data = &(pkgconf_tuple_t){
				.key = "pc_path",
				.value = PKG_DEFAULT_PATH,
			},
		},
		.tail = NULL,
	},
};

static pkgconf_pkg_t pkgconf_virtual = {
	.id = "pkgconf",
	.realname = "pkgconf",
	.description = "virtual package defining pkgconf API version supported",
	.url = PACKAGE_BUGREPORT,
	.version = PACKAGE_VERSION,
	.flags = PKGCONF_PKG_PROPF_VIRTUAL,
	.vars = {
		.head = &(pkgconf_node_t){
			.prev = NULL,
			.next = NULL,
			.data = &(pkgconf_tuple_t){
				.key = "pc_path",
				.value = PKG_DEFAULT_PATH,
			},
		},
		.tail = NULL,
	},
};

typedef struct {
	const char *name;
	pkgconf_pkg_t *pkg;
} pkgconf_builtin_pkg_pair_t;

/* keep these in alphabetical order */
static const pkgconf_builtin_pkg_pair_t pkgconf_builtin_pkg_pair_set[] = {
	{"pkg-config", &pkg_config_virtual},
	{"pkgconf", &pkgconf_virtual},
};

static int pkgconf_builtin_pkg_pair_cmp(const void *key, const void *ptr)
{
	const pkgconf_builtin_pkg_pair_t *pair = ptr;
	return strcasecmp(key, pair->name);
}

pkgconf_pkg_t *pkgconf_builtin_pkg_get(const char *name)
{
	const pkgconf_builtin_pkg_pair_t *pair = bsearch(name, pkgconf_builtin_pkg_pair_set,
		PKGCONF_ARRAY_SIZE(pkgconf_builtin_pkg_pair_set), sizeof(pkgconf_builtin_pkg_pair_t),
		pkgconf_builtin_pkg_pair_cmp);

	return (pair != NULL) ? pair->pkg : NULL;
}

typedef bool (*pkgconf_vercmp_res_func_t)(const char *a, const char *b);

typedef struct {
	const char *name;
	pkgconf_pkg_comparator_t compare;
} pkgconf_pkg_comparator_pair_t;

static const pkgconf_pkg_comparator_pair_t pkgconf_pkg_comparator_names[] = {
	{"!=",		PKGCONF_CMP_NOT_EQUAL},
	{"(any)",	PKGCONF_CMP_ANY},
	{"<",		PKGCONF_CMP_LESS_THAN},
	{"<=",		PKGCONF_CMP_LESS_THAN_EQUAL},
	{"=",		PKGCONF_CMP_EQUAL},
	{">",		PKGCONF_CMP_GREATER_THAN},
	{">=",		PKGCONF_CMP_GREATER_THAN_EQUAL},
};

static int pkgconf_pkg_comparator_pair_namecmp(const void *key, const void *ptr)
{
	const pkgconf_pkg_comparator_pair_t *pair = ptr;
	return strcmp(key, pair->name);
}

static bool pkgconf_pkg_comparator_lt(const char *a, const char *b)
{
	return (pkgconf_compare_version(a, b) < 0);
}

static bool pkgconf_pkg_comparator_gt(const char *a, const char *b)
{
	return (pkgconf_compare_version(a, b) > 0);
}

static bool pkgconf_pkg_comparator_lte(const char *a, const char *b)
{
	return (pkgconf_compare_version(a, b) <= 0);
}

static bool pkgconf_pkg_comparator_gte(const char *a, const char *b)
{
	return (pkgconf_compare_version(a, b) >= 0);
}

static bool pkgconf_pkg_comparator_eq(const char *a, const char *b)
{
	return (pkgconf_compare_version(a, b) == 0);
}

static bool pkgconf_pkg_comparator_ne(const char *a, const char *b)
{
	return (pkgconf_compare_version(a, b) != 0);
}

static bool pkgconf_pkg_comparator_any(const char *a, const char *b)
{
	(void) a;
	(void) b;

	return true;
}

static bool pkgconf_pkg_comparator_none(const char *a, const char *b)
{
	(void) a;
	(void) b;

	return false;
}

static const pkgconf_vercmp_res_func_t pkgconf_pkg_comparator_impls[] = {
	[PKGCONF_CMP_ANY]			= pkgconf_pkg_comparator_any,
	[PKGCONF_CMP_LESS_THAN]			= pkgconf_pkg_comparator_lt,
	[PKGCONF_CMP_GREATER_THAN]		= pkgconf_pkg_comparator_gt,
	[PKGCONF_CMP_LESS_THAN_EQUAL]		= pkgconf_pkg_comparator_lte,
	[PKGCONF_CMP_GREATER_THAN_EQUAL]	= pkgconf_pkg_comparator_gte,
	[PKGCONF_CMP_EQUAL]			= pkgconf_pkg_comparator_eq,
	[PKGCONF_CMP_NOT_EQUAL]			= pkgconf_pkg_comparator_ne,
};

/*
 * pkgconf_pkg_get_comparator(pkgdep)
 *
 * returns the comparator used in a depgraph dependency node as a string.
 */
const char *
pkgconf_pkg_get_comparator(const pkgconf_dependency_t *pkgdep)
{
	if (pkgdep->compare > PKGCONF_ARRAY_SIZE(pkgconf_pkg_comparator_names))
		return "???";

	return pkgconf_pkg_comparator_names[pkgdep->compare].name;
}

/*
 * pkgconf_pkg_comparator_lookup_by_name(name)
 *
 * look up the appropriate comparator bytecode in the comparator set (defined
 * above, see pkgconf_pkg_comparator_names and pkgconf_pkg_comparator_impls).
 */
pkgconf_pkg_comparator_t
pkgconf_pkg_comparator_lookup_by_name(const char *name)
{
	const pkgconf_pkg_comparator_pair_t *p = bsearch(name, pkgconf_pkg_comparator_names,
		PKGCONF_ARRAY_SIZE(pkgconf_pkg_comparator_names), sizeof(pkgconf_pkg_comparator_pair_t),
		pkgconf_pkg_comparator_pair_namecmp);

	return (p != NULL) ? p->compare : PKGCONF_CMP_ANY;
}

typedef struct {
	pkgconf_dependency_t *pkgdep;
	unsigned int flags;
} pkgconf_pkg_scan_providers_ctx_t;

typedef struct {
	const pkgconf_vercmp_res_func_t rulecmp[PKGCONF_CMP_COUNT];
	const pkgconf_vercmp_res_func_t depcmp[PKGCONF_CMP_COUNT];
} pkgconf_pkg_provides_vermatch_rule_t;

static const pkgconf_pkg_provides_vermatch_rule_t pkgconf_pkg_provides_vermatch_rules[] = {
	[PKGCONF_CMP_ANY] = {
		.rulecmp = {},
		.depcmp = {},
	},
	[PKGCONF_CMP_LESS_THAN] = {
		.rulecmp = {
			[PKGCONF_CMP_ANY]			= pkgconf_pkg_comparator_none,
			[PKGCONF_CMP_LESS_THAN]			= pkgconf_pkg_comparator_lt,
			[PKGCONF_CMP_GREATER_THAN]		= pkgconf_pkg_comparator_gt,
			[PKGCONF_CMP_LESS_THAN_EQUAL]		= pkgconf_pkg_comparator_lte,
			[PKGCONF_CMP_GREATER_THAN_EQUAL]	= pkgconf_pkg_comparator_gte,
		},
		.depcmp = {
			[PKGCONF_CMP_GREATER_THAN]		= pkgconf_pkg_comparator_lt,
			[PKGCONF_CMP_GREATER_THAN_EQUAL]	= pkgconf_pkg_comparator_lt,
			[PKGCONF_CMP_EQUAL]			= pkgconf_pkg_comparator_lt,
			[PKGCONF_CMP_NOT_EQUAL]			= pkgconf_pkg_comparator_gte,
		},
	},
	[PKGCONF_CMP_GREATER_THAN] = {
		.rulecmp = {
			[PKGCONF_CMP_ANY]			= pkgconf_pkg_comparator_none,
			[PKGCONF_CMP_LESS_THAN]			= pkgconf_pkg_comparator_lt,
			[PKGCONF_CMP_GREATER_THAN]		= pkgconf_pkg_comparator_gt,
			[PKGCONF_CMP_LESS_THAN_EQUAL]		= pkgconf_pkg_comparator_lte,
			[PKGCONF_CMP_GREATER_THAN_EQUAL]	= pkgconf_pkg_comparator_gte,
		},
		.depcmp = {
			[PKGCONF_CMP_LESS_THAN]			= pkgconf_pkg_comparator_gt,
			[PKGCONF_CMP_LESS_THAN_EQUAL]		= pkgconf_pkg_comparator_gt,
			[PKGCONF_CMP_EQUAL]			= pkgconf_pkg_comparator_gt,
			[PKGCONF_CMP_NOT_EQUAL]			= pkgconf_pkg_comparator_lte,
		},
	},
	[PKGCONF_CMP_LESS_THAN_EQUAL] = {
		.rulecmp = {
			[PKGCONF_CMP_ANY]			= pkgconf_pkg_comparator_none,
			[PKGCONF_CMP_LESS_THAN]			= pkgconf_pkg_comparator_lt,
			[PKGCONF_CMP_GREATER_THAN]		= pkgconf_pkg_comparator_gt,
			[PKGCONF_CMP_LESS_THAN_EQUAL]		= pkgconf_pkg_comparator_lte,
			[PKGCONF_CMP_GREATER_THAN_EQUAL]	= pkgconf_pkg_comparator_gte,
		},
		.depcmp = {
			[PKGCONF_CMP_GREATER_THAN]		= pkgconf_pkg_comparator_lte,
			[PKGCONF_CMP_GREATER_THAN_EQUAL]	= pkgconf_pkg_comparator_lte,
			[PKGCONF_CMP_EQUAL]			= pkgconf_pkg_comparator_lte,
			[PKGCONF_CMP_NOT_EQUAL]			= pkgconf_pkg_comparator_gt,
		},
	},
	[PKGCONF_CMP_GREATER_THAN_EQUAL] = {
		.rulecmp = {
			[PKGCONF_CMP_ANY]			= pkgconf_pkg_comparator_none,
			[PKGCONF_CMP_LESS_THAN]			= pkgconf_pkg_comparator_lt,
			[PKGCONF_CMP_GREATER_THAN]		= pkgconf_pkg_comparator_gt,
			[PKGCONF_CMP_LESS_THAN_EQUAL]		= pkgconf_pkg_comparator_lte,
			[PKGCONF_CMP_GREATER_THAN_EQUAL]	= pkgconf_pkg_comparator_gte,
		},
		.depcmp = {
			[PKGCONF_CMP_LESS_THAN]			= pkgconf_pkg_comparator_gte,
			[PKGCONF_CMP_LESS_THAN_EQUAL]		= pkgconf_pkg_comparator_gte,
			[PKGCONF_CMP_EQUAL]			= pkgconf_pkg_comparator_gte,
			[PKGCONF_CMP_NOT_EQUAL]			= pkgconf_pkg_comparator_lt,
		},
	},
	[PKGCONF_CMP_EQUAL] = {
		.rulecmp = {
			[PKGCONF_CMP_ANY]			= pkgconf_pkg_comparator_none,
			[PKGCONF_CMP_LESS_THAN]			= pkgconf_pkg_comparator_lt,
			[PKGCONF_CMP_GREATER_THAN]		= pkgconf_pkg_comparator_gt,
			[PKGCONF_CMP_LESS_THAN_EQUAL]		= pkgconf_pkg_comparator_lte,
			[PKGCONF_CMP_GREATER_THAN_EQUAL]	= pkgconf_pkg_comparator_gte,
			[PKGCONF_CMP_EQUAL]			= pkgconf_pkg_comparator_eq,
			[PKGCONF_CMP_NOT_EQUAL]			= pkgconf_pkg_comparator_ne
		},
		.depcmp = {},
	},
	[PKGCONF_CMP_NOT_EQUAL] = {
		.rulecmp = {
			[PKGCONF_CMP_ANY]			= pkgconf_pkg_comparator_none,
			[PKGCONF_CMP_LESS_THAN]			= pkgconf_pkg_comparator_gte,
			[PKGCONF_CMP_GREATER_THAN]		= pkgconf_pkg_comparator_lte,
			[PKGCONF_CMP_LESS_THAN_EQUAL]		= pkgconf_pkg_comparator_gt,
			[PKGCONF_CMP_GREATER_THAN_EQUAL]	= pkgconf_pkg_comparator_lt,
			[PKGCONF_CMP_EQUAL]			= pkgconf_pkg_comparator_ne,
			[PKGCONF_CMP_NOT_EQUAL]			= pkgconf_pkg_comparator_eq
		},
		.depcmp = {},
	},
};

/*
 * pkgconf_pkg_scan_provides_vercmp(pkgdep, provider)
 *
 * compare a provides node against the requested dependency node.
 *
 * XXX: maybe handle PKGCONF_CMP_ANY in a versioned comparison
 */
static bool
pkgconf_pkg_scan_provides_vercmp(const pkgconf_dependency_t *pkgdep, const pkgconf_dependency_t *provider)
{
	const pkgconf_pkg_provides_vermatch_rule_t *rule = &pkgconf_pkg_provides_vermatch_rules[pkgdep->compare];

	if (rule == NULL)
		return false;

	if (rule->depcmp[provider->compare] != NULL &&
	    !rule->depcmp[provider->compare](provider->version, pkgdep->version))
		return false;

	if (rule->rulecmp[provider->compare] != NULL &&
	    !rule->rulecmp[provider->compare](pkgdep->version, provider->version))
		return false;

	return true;
}

/*
 * pkgconf_pkg_scan_provides_entry(pkg, ctx)
 *
 * attempt to match a single package's Provides rules against the requested dependency node.
 */
static bool
pkgconf_pkg_scan_provides_entry(const pkgconf_pkg_t *pkg, const pkgconf_pkg_scan_providers_ctx_t *ctx)
{
	const pkgconf_dependency_t *pkgdep = ctx->pkgdep;
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->provides.head, node)
	{
		const pkgconf_dependency_t *provider = node->data;
		if (!strcmp(provider->package, pkgdep->package))
			return pkgconf_pkg_scan_provides_vercmp(pkgdep, provider);
	}

	return false;
}

/*
 * pkgconf_pkg_scan_providers(pkgdep, flags, eflags)
 *
 * scan all available packages to see if a Provides rule matches the pkgdep.
 */
static pkgconf_pkg_t *
pkgconf_pkg_scan_providers(pkgconf_client_t *client, pkgconf_dependency_t *pkgdep, unsigned int flags, unsigned int *eflags)
{
	pkgconf_pkg_t *pkg;
	pkgconf_pkg_scan_providers_ctx_t ctx = {
		.pkgdep = pkgdep,
		.flags = flags
	};

	pkg = pkgconf_scan_all(client, &ctx, (pkgconf_pkg_iteration_func_t) pkgconf_pkg_scan_provides_entry);
	if (pkg != NULL)
		return pkg;

	if (eflags != NULL)
		*eflags |= PKGCONF_PKG_ERRF_PACKAGE_NOT_FOUND;

	return NULL;
}

/*
 * pkg_verify_dependency(pkgdep, flags)
 *
 * verify a pkgconf_dependency_t node in the depgraph.  if the dependency is solvable,
 * return the appropriate pkgconf_pkg_t object, else NULL.
 */
pkgconf_pkg_t *
pkgconf_pkg_verify_dependency(pkgconf_client_t *client, pkgconf_dependency_t *pkgdep, unsigned int flags, unsigned int *eflags)
{
	pkgconf_pkg_t *pkg = NULL;

	if (eflags != NULL)
		*eflags = PKGCONF_PKG_ERRF_OK;

	pkg = pkgconf_pkg_find(client, pkgdep->package, flags);
	if (pkg == NULL)
	{
		if (flags & PKGCONF_PKG_PKGF_SKIP_PROVIDES)
		{
			if (eflags != NULL)
				*eflags |= PKGCONF_PKG_ERRF_PACKAGE_NOT_FOUND;

			return NULL;
		}

		return pkgconf_pkg_scan_providers(client, pkgdep, flags, eflags);
	}

	if (pkg->id == NULL)
		pkg->id = strdup(pkgdep->package);

	if (pkgconf_pkg_comparator_impls[pkgdep->compare](pkg->version, pkgdep->version) == true)
		return pkg;

	if (eflags != NULL)
		*eflags |= PKGCONF_PKG_ERRF_PACKAGE_VER_MISMATCH;

	return pkg;
}

/*
 * pkg_verify_graph(root, depth)
 *
 * verify the graph dependency nodes are satisfiable by walking the tree using
 * pkgconf_pkg_traverse().
 */
unsigned int
pkgconf_pkg_verify_graph(pkgconf_client_t *client, pkgconf_pkg_t *root, int depth, unsigned int flags)
{
	return pkgconf_pkg_traverse(client, root, NULL, NULL, depth, flags);
}

static unsigned int
pkgconf_pkg_report_graph_error(pkgconf_client_t *client, pkgconf_pkg_t *parent, pkgconf_pkg_t *pkg, pkgconf_dependency_t *node, unsigned int eflags)
{
	static bool already_sent_notice = false;

	if (eflags & PKGCONF_PKG_ERRF_PACKAGE_NOT_FOUND)
	{
		if (!already_sent_notice)
		{
			pkgconf_error(client, "Package %s was not found in the pkg-config search path.\n", node->package);
			pkgconf_error(client, "Perhaps you should add the directory containing `%s.pc'\n", node->package);
			pkgconf_error(client, "to the PKG_CONFIG_PATH environment variable\n");
			already_sent_notice = true;
		}

		pkgconf_error(client, "Package '%s', required by '%s', not found\n", node->package, parent->id);
		pkgconf_audit_log(client, "%s NOT-FOUND\n", node->package);
	}
	else if (eflags & PKGCONF_PKG_ERRF_PACKAGE_VER_MISMATCH)
	{
		pkgconf_error(client, "Package dependency requirement '%s %s %s' could not be satisfied.\n",
			node->package, pkgconf_pkg_get_comparator(node), node->version);

		if (pkg != NULL)
			pkgconf_error(client, "Package '%s' has version '%s', required version is '%s %s'\n",
				node->package, pkg->version, pkgconf_pkg_get_comparator(node), node->version);
	}

	if (pkg != NULL)
		pkgconf_pkg_unref(client, pkg);

	return eflags;
}

static inline unsigned int
pkgconf_pkg_walk_list(pkgconf_client_t *client,
	pkgconf_pkg_t *parent,
	pkgconf_list_t *deplist,
	pkgconf_pkg_traverse_func_t func,
	void *data,
	int depth,
	unsigned int flags)
{
	unsigned int eflags = PKGCONF_PKG_ERRF_OK;
	pkgconf_node_t *node;

	PKGCONF_FOREACH_LIST_ENTRY(deplist->head, node)
	{
		unsigned int eflags_local = PKGCONF_PKG_ERRF_OK;
		pkgconf_dependency_t *depnode = node->data;
		pkgconf_pkg_t *pkgdep;

		if (*depnode->package == '\0')
			continue;

		pkgdep = pkgconf_pkg_verify_dependency(client, depnode, flags, &eflags_local);

		eflags |= eflags_local;
		if (eflags_local != PKGCONF_PKG_ERRF_OK && !(flags & PKGCONF_PKG_PKGF_SKIP_ERRORS))
		{
			pkgconf_pkg_report_graph_error(client, parent, pkgdep, depnode, eflags_local);
			continue;
		}
		if (pkgdep == NULL)
			continue;

		if (pkgdep->flags & PKGCONF_PKG_PROPF_SEEN)
		{
			pkgconf_pkg_unref(client, pkgdep);
			continue;
		}

		pkgconf_audit_log_dependency(client, pkgdep, depnode);

		pkgdep->flags |= PKGCONF_PKG_PROPF_SEEN;
		eflags |= pkgconf_pkg_traverse(client, pkgdep, func, data, depth - 1, flags);
		pkgdep->flags &= ~PKGCONF_PKG_PROPF_SEEN;
		pkgconf_pkg_unref(client, pkgdep);
	}

	return eflags;
}

static inline unsigned int
pkgconf_pkg_walk_conflicts_list(pkgconf_client_t *client,
	pkgconf_pkg_t *root, pkgconf_list_t *deplist, unsigned int flags)
{
	unsigned int eflags;
	pkgconf_node_t *node, *childnode;

	PKGCONF_FOREACH_LIST_ENTRY(deplist->head, node)
	{
		pkgconf_dependency_t *parentnode = node->data;

		if (*parentnode->package == '\0')
			continue;

		PKGCONF_FOREACH_LIST_ENTRY(root->requires.head, childnode)
		{
			pkgconf_pkg_t *pkgdep;
			pkgconf_dependency_t *depnode = childnode->data;

			if (*depnode->package == '\0' || strcmp(depnode->package, parentnode->package))
				continue;

			pkgdep = pkgconf_pkg_verify_dependency(client, parentnode, flags, &eflags);
			if (eflags == PKGCONF_PKG_ERRF_OK)
			{
				pkgconf_error(client, "Version '%s' of '%s' conflicts with '%s' due to satisfying conflict rule '%s %s%s%s'.\n",
					pkgdep->version, pkgdep->realname, root->realname, parentnode->package, pkgconf_pkg_get_comparator(parentnode),
					parentnode->version != NULL ? " " : "", parentnode->version != NULL ? parentnode->version : "");
				pkgconf_error(client, "It may be possible to ignore this conflict and continue, try the\n");
				pkgconf_error(client, "PKG_CONFIG_IGNORE_CONFLICTS environment variable.\n");

				pkgconf_pkg_unref(client, pkgdep);

				return PKGCONF_PKG_ERRF_PACKAGE_CONFLICT;
			}

			pkgconf_pkg_unref(client, pkgdep);
		}
	}

	return PKGCONF_PKG_ERRF_OK;
}

/*
 * pkgconf_pkg_traverse(root, func, data, maxdepth, flags)
 *
 * walk the dependency graph up to maxdepth levels.  -1 means infinite recursion.
 */
unsigned int
pkgconf_pkg_traverse(pkgconf_client_t *client,
	pkgconf_pkg_t *root,
	pkgconf_pkg_traverse_func_t func,
	void *data,
	int maxdepth,
	unsigned int flags)
{
	unsigned int rflags = flags & ~PKGCONF_PKG_PKGF_SKIP_ROOT_VIRTUAL;
	unsigned int eflags = PKGCONF_PKG_ERRF_OK;

	if (maxdepth == 0)
		return eflags;

	if ((root->flags & PKGCONF_PKG_PROPF_VIRTUAL) != PKGCONF_PKG_PROPF_VIRTUAL || (flags & PKGCONF_PKG_PKGF_SKIP_ROOT_VIRTUAL) != PKGCONF_PKG_PKGF_SKIP_ROOT_VIRTUAL)
	{
		if (func != NULL)
			func(client, root, data, flags);
	}

	if (!(flags & PKGCONF_PKG_PKGF_SKIP_CONFLICTS))
	{
		eflags = pkgconf_pkg_walk_conflicts_list(client, root, &root->conflicts, rflags);
		if (eflags != PKGCONF_PKG_ERRF_OK)
			return eflags;
	}

	eflags = pkgconf_pkg_walk_list(client, root, &root->requires, func, data, maxdepth, rflags);
	if (eflags != PKGCONF_PKG_ERRF_OK)
		return eflags;

	if (flags & PKGCONF_PKG_PKGF_SEARCH_PRIVATE)
	{
		eflags = pkgconf_pkg_walk_list(client, root, &root->requires_private, func, data, maxdepth, rflags | PKGCONF_PKG_PKGF_ITER_PKG_IS_PRIVATE);
		if (eflags != PKGCONF_PKG_ERRF_OK)
			return eflags;
	}

	return eflags;
}

static void
pkgconf_pkg_cflags_collect(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data, unsigned int flags)
{
	pkgconf_list_t *list = data;
	pkgconf_node_t *node;
	(void) flags;
	(void) client;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->cflags.head, node)
	{
		pkgconf_fragment_t *frag = node->data;
		pkgconf_fragment_copy(list, frag, flags, false);
	}
}

static void
pkgconf_pkg_cflags_private_collect(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data, unsigned int flags)
{
	pkgconf_list_t *list = data;
	pkgconf_node_t *node;
	(void) flags;
	(void) client;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->cflags_private.head, node)
	{
		pkgconf_fragment_t *frag = node->data;

		pkgconf_fragment_copy(list, frag, flags, true);
	}
}

int
pkgconf_pkg_cflags(pkgconf_client_t *client, pkgconf_pkg_t *root, pkgconf_list_t *list, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkgconf_pkg_traverse(client, root, pkgconf_pkg_cflags_collect, list, maxdepth, flags);
	if (eflag != PKGCONF_PKG_ERRF_OK)
		pkgconf_fragment_free(list);

	if (flags & PKGCONF_PKG_PKGF_MERGE_PRIVATE_FRAGMENTS)
	{
		eflag = pkgconf_pkg_traverse(client, root, pkgconf_pkg_cflags_private_collect, list, maxdepth, flags);
		if (eflag != PKGCONF_PKG_ERRF_OK)
			pkgconf_fragment_free(list);
	}

	return eflag;
}

static void
pkgconf_pkg_libs_collect(pkgconf_client_t *client, pkgconf_pkg_t *pkg, void *data, unsigned int flags)
{
	pkgconf_list_t *list = data;
	pkgconf_node_t *node;
	(void) flags;
	(void) client;

	PKGCONF_FOREACH_LIST_ENTRY(pkg->libs.head, node)
	{
		pkgconf_fragment_t *frag = node->data;
		pkgconf_fragment_copy(list, frag, flags, (flags & PKGCONF_PKG_PKGF_ITER_PKG_IS_PRIVATE) != 0);
	}

	if (flags & PKGCONF_PKG_PKGF_MERGE_PRIVATE_FRAGMENTS)
	{
		PKGCONF_FOREACH_LIST_ENTRY(pkg->libs_private.head, node)
		{
			pkgconf_fragment_t *frag = node->data;
			pkgconf_fragment_copy(list, frag, flags, true);
		}
	}
}

int
pkgconf_pkg_libs(pkgconf_client_t *client, pkgconf_pkg_t *root, pkgconf_list_t *list, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkgconf_pkg_traverse(client, root, pkgconf_pkg_libs_collect, list, maxdepth, flags);

	if (eflag != PKGCONF_PKG_ERRF_OK)
	{
		pkgconf_fragment_free(list);
		return eflag;
	}

	return eflag;
}
