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

#include "pkg.h"
#include "bsdstubs.h"

#ifdef _WIN32
#	define PKG_CONFIG_REG_KEY "Software\\pkgconfig\\PKG_CONFIG_PATH"
#	undef PKG_DEFAULT_PATH
#	define PKG_DEFAULT_PATH "../lib/pkgconfig;../share/pkgconfig"
#endif

#define PKG_CONFIG_EXT		".pc"
#define PKG_CONFIG_PATH_SZ	(65535)

/* pkg-config uses ';' on win32 as ':' is part of path */
#ifdef _WIN32
#define PKG_CONFIG_PATH_SEP_S	";"
#else
#define PKG_CONFIG_PATH_SEP_S	":"
#endif

#ifdef _WIN32
#define PKG_DIR_SEP_S	'\\'
#else
#define PKG_DIR_SEP_S	'/'
#endif

typedef struct {
	char *path;
	pkg_node_t node;
} pkg_path_t;

static inline void
path_add(const char *text, pkg_list_t *dirlist)
{
	pkg_path_t *pkg_path;

	pkg_path = calloc(sizeof(pkg_path_t), 1);
	pkg_path->path = strdup(text);

	pkg_node_insert_tail(&pkg_path->node, pkg_path, dirlist);
}

static inline size_t
path_split(const char *text, pkg_list_t *dirlist)
{
	size_t count = 0;
	char *workbuf, *p, *iter;

	if (text == NULL)
		return 0;

	iter = workbuf = strdup(text);
	while ((p = strtok(iter, PKG_CONFIG_PATH_SEP_S)) != NULL)
	{
		path_add(p, dirlist);

		count++, iter = NULL;
	}
	free(workbuf);

	return count;
}

static inline void
path_free(pkg_list_t *dirlist)
{
	pkg_node_t *n, *tn;

	PKG_FOREACH_LIST_ENTRY_SAFE(dirlist->head, tn, n)
	{
		pkg_path_t *pkg_path = n->data;

		free(pkg_path->path);
		free(pkg_path);
	}
}

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
get_pkgconfig_path(void)
{
	const char *env_path;
#ifdef _WIN32
	static char outbuf[MAX_PATH];
	char namebuf[MAX_PATH];
	char *p;
#endif

	env_path = getenv("PKG_CONFIG_LIBDIR");
	if (env_path != NULL)
		return env_path;

#ifdef _WIN32
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
	strlcpy(outbuf, namebuf, sizeof outbuf);
	strlcat(outbuf, "/", sizeof outbuf);
	strlcat(outbuf, "../lib/pkgconfig", sizeof outbuf);
	strlcat(outbuf, ";", sizeof outbuf);
	strlcat(outbuf, namebuf, sizeof outbuf);
	strlcat(outbuf, "/", sizeof outbuf);
	strlcat(outbuf, "../share/pkgconfig", sizeof outbuf);

	return outbuf;
#endif

	return PKG_DEFAULT_PATH;
}

static const char *
pkg_get_parent_dir(pkg_t *pkg)
{
	static char buf[PKG_BUFSIZE];
	char *pathbuf;

	strlcpy(buf, pkg->filename, sizeof buf);
	pathbuf = strrchr(buf, PKG_DIR_SEP_S);
	if (pathbuf == NULL)
		pathbuf = strrchr(buf, '/');
	if (pathbuf != NULL)
		pathbuf[0] = '\0';

	return buf;
}

static pkg_list_t pkg_dir_list = PKG_LIST_INITIALIZER;

void
pkg_dir_list_build(unsigned int flags)
{
	const char *env_path;

	if (pkg_dir_list.head != NULL || pkg_dir_list.tail != NULL)
		return;

	/* PKG_CONFIG_PATH has to take precedence */
	env_path = getenv("PKG_CONFIG_PATH");
	if (env_path)
		path_split(env_path, &pkg_dir_list);

	if (!(flags & PKGF_ENV_ONLY))
	{
		env_path = get_pkgconfig_path();
		path_split(env_path, &pkg_dir_list);
	}
}

void
pkg_dir_list_free(void)
{
	path_free(&pkg_dir_list);
}

/*
 * pkg_new_from_file(filename, file, flags)
 *
 * Parse a .pc file into a pkg_t object structure.
 */
pkg_t *
pkg_new_from_file(const char *filename, FILE *f, unsigned int flags)
{
	pkg_t *pkg;
	char readbuf[PKG_BUFSIZE];
	char *idptr;

	pkg = calloc(sizeof(pkg_t), 1);
	pkg->filename = strdup(filename);
	pkg_tuple_add(&pkg->vars, "pcfiledir", pkg_get_parent_dir(pkg));

	/* make module id */
	if ((idptr = strrchr(pkg->filename, PKG_DIR_SEP_S)) != NULL)
		idptr++;
	else
		idptr = pkg->filename;

	pkg->id = strdup(idptr);
	idptr = strrchr(pkg->id, '.');
	if (idptr)
		*idptr = '\0';

	while (pkg_fgetline(readbuf, PKG_BUFSIZE, f) != NULL)
	{
		char op, *p, *key, *value;

		p = readbuf;
		while (*p && (isalpha(*p) || isdigit(*p) || *p == '_' || *p == '.'))
			p++;

		key = readbuf;
		if (!isalpha(*key) && !isdigit(*p))
			continue;

		while (*p && isspace(*p)) {
			/* set to null to avoid trailing spaces in key */
			*p = '\0';
			p++;
		}

		op = *p;
		*p = '\0';
		p++;

		while (*p && isspace(*p))
			p++;

		value = p;

		switch (op)
		{
		case ':':
			if (!strcmp(key, "Name"))
				pkg->realname = pkg_tuple_parse(&pkg->vars, value);
			else if (!strcmp(key, "Description"))
				pkg->description = pkg_tuple_parse(&pkg->vars, value);
			else if (!strcmp(key, "Version"))
				pkg->version = pkg_tuple_parse(&pkg->vars, value);
			else if (!strcasecmp(key, "CFLAGS"))
				pkg_fragment_parse(&pkg->cflags, &pkg->vars, value, flags);
			else if (!strcasecmp(key, "CFLAGS.private"))
				pkg_fragment_parse(&pkg->cflags_private, &pkg->vars, value, flags);
			else if (!strcasecmp(key, "LIBS"))
				pkg_fragment_parse(&pkg->libs, &pkg->vars, value, flags);
			else if (!strcasecmp(key, "LIBS.private"))
				pkg_fragment_parse(&pkg->libs_private, &pkg->vars, value, flags);
			else if (!strcmp(key, "Requires"))
				pkg_dependency_parse(pkg, &pkg->requires, value);
			else if (!strcmp(key, "Requires.private"))
				pkg_dependency_parse(pkg, &pkg->requires_private, value);
			else if (!strcmp(key, "Conflicts"))
				pkg_dependency_parse(pkg, &pkg->conflicts, value);
			break;
		case '=':
			pkg_tuple_add(&pkg->vars, key, value);
			break;
		default:
			break;
		}
	}

	fclose(f);
	return pkg_ref(pkg);
}

void
pkg_free(pkg_t *pkg)
{
	if (pkg == NULL || pkg->flags & PKG_PROPF_VIRTUAL)
		return;

	pkg_cache_remove(pkg);

	pkg_dependency_free(&pkg->requires);
	pkg_dependency_free(&pkg->requires_private);
	pkg_dependency_free(&pkg->conflicts);

	pkg_fragment_free(&pkg->cflags);
	pkg_fragment_free(&pkg->cflags_private);
	pkg_fragment_free(&pkg->libs);
	pkg_fragment_free(&pkg->libs_private);

	pkg_tuple_free(&pkg->vars);

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

pkg_t *
pkg_ref(pkg_t *pkg)
{
	pkg->refcount++;
	return pkg;
}

void
pkg_unref(pkg_t *pkg)
{
	pkg->refcount--;
	if (pkg->refcount <= 0)
		pkg_free(pkg);
}

static inline pkg_t *
pkg_try_specific_path(const char *path, const char *name, unsigned int flags)
{
	pkg_t *pkg = NULL;
	FILE *f;
	char locbuf[PKG_CONFIG_PATH_SZ];
	char uninst_locbuf[PKG_CONFIG_PATH_SZ];

	snprintf(locbuf, sizeof locbuf, "%s/%s" PKG_CONFIG_EXT, path, name);
	snprintf(uninst_locbuf, sizeof uninst_locbuf, "%s/%s-uninstalled" PKG_CONFIG_EXT, path, name);

	if (!(flags & PKGF_NO_UNINSTALLED) && (f = fopen(uninst_locbuf, "r")) != NULL)
	{
		pkg = pkg_new_from_file(uninst_locbuf, f, flags);
		pkg->flags |= PKG_PROPF_UNINSTALLED;
	}
	else if ((f = fopen(locbuf, "r")) != NULL)
		pkg = pkg_new_from_file(locbuf, f, flags);

	return pkg;
}

static void
pkg_scan_dir(const char *path, pkg_iteration_func_t func)
{
	DIR *dir;
	struct dirent *dirent;

	dir = opendir(path);
	if (dir == NULL)
		return;

	for (dirent = readdir(dir); dirent != NULL; dirent = readdir(dir))
	{
		static char filebuf[PKG_BUFSIZE];
		pkg_t *pkg;
		FILE *f;
		struct stat st;

		strlcpy(filebuf, path, sizeof filebuf);
		strlcat(filebuf, "/", sizeof filebuf);
		strlcat(filebuf, dirent->d_name, sizeof filebuf);

		stat(filebuf, &st);
		if (!(S_ISREG(st.st_mode)))
			continue;

		f = fopen(filebuf, "r");
		if (f == NULL)
			continue;

		pkg = pkg_new_from_file(filebuf, f, 0);
		if (pkg != NULL)
		{
			func(pkg);
			pkg_unref(pkg);
		}
	}

	closedir(dir);
}

void
pkg_scan_all(pkg_iteration_func_t func)
{
	pkg_node_t *n;

	pkg_dir_list_build(0);

	PKG_FOREACH_LIST_ENTRY(pkg_dir_list.head, n)
	{
		pkg_path_t *pkg_path = n->data;

		pkg_scan_dir(pkg_path->path, func);
	}
}

#ifdef _WIN32
pkg_t *
pkg_find_in_registry_key(HKEY hkey, const char *name, unsigned int flags)
{
	pkg_t *pkg = NULL;

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
			pkg = pkg_try_specific_path(pathbuf, name, flags);
			if (pkg != NULL)
				break;
		}

		bufsize = sizeof buf;
	}

	RegCloseKey(key);
	return pkg;
}
#endif

pkg_t *
pkg_find(const char *name, unsigned int flags)
{
	pkg_t *pkg = NULL;
	pkg_node_t *n;
	FILE *f;

	pkg_dir_list_build(flags);

	/* name might actually be a filename. */
	if (str_has_suffix(name, PKG_CONFIG_EXT))
	{
		if ((f = fopen(name, "r")) != NULL)
		{
			pkg_t *pkg;

			pkg = pkg_new_from_file(name, f, flags);
			path_add(pkg_get_parent_dir(pkg), &pkg_dir_list);

			return pkg;
		}
	}

	/* check cache */
	if (!(flags & PKGF_NO_CACHE))
	{
		if ((pkg = pkg_cache_lookup(name)) != NULL)
		{
			pkg->flags |= PKG_PROPF_CACHED;
			return pkg;
		}
	}

	PKG_FOREACH_LIST_ENTRY(pkg_dir_list.head, n)
	{
		pkg_path_t *pkg_path = n->data;

		pkg = pkg_try_specific_path(pkg_path->path, name, flags);
		if (pkg != NULL)
			goto out;
	}

#ifdef _WIN32
	/* support getting PKG_CONFIG_PATH from registry */
	pkg = pkg_find_in_registry_key(HKEY_CURRENT_USER, name, flags);
	if (!pkg)
		pkg = pkg_find_in_registry_key(HKEY_LOCAL_MACHINE, name, flags);
#endif

out:
	pkg_cache_add(pkg);

	return pkg;
}

/*
 * pkg_compare_version(a, b)
 *
 * compare versions using RPM version comparison rules as described in the LSB.
 */
int
pkg_compare_version(const char *a, const char *b)
{
	char oldch1, oldch2;
	char buf1[PKG_BUFSIZE], buf2[PKG_BUFSIZE];
	char *str1, *str2;
	char *one, *two;
	int ret;
	bool isnum;

	/* optimization: if version matches then it's the same version. */
	if (!strcasecmp(a, b))
		return 0;

	strlcpy(buf1, a, sizeof buf1);
	strlcpy(buf2, b, sizeof buf2);

	one = str1 = buf1;
	two = str2 = buf2;

	while (*one || *two)
	{
		while (*one && !isalnum(*one) && *one != '~')
			one++;
		while (*two && !isalnum(*two) && *two != '~')
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

		if (isdigit(*str1))
		{
			while (*str1 && isdigit(*str1))
				str1++;

			while (*str2 && isdigit(*str2))
				str2++;

			isnum = true;
		}
		else
		{
			while (*str1 && isalpha(*str1))
				str1++;

			while (*str2 && isalpha(*str2))
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

static pkg_t pkg_config_virtual = {
	.id = "pkg-config",
	.realname = "pkg-config",
	.description = "virtual package defining pkg-config API version supported",
	.url = PACKAGE_BUGREPORT,
	.version = PKG_PKGCONFIG_VERSION_EQUIV,
	.flags = PKG_PROPF_VIRTUAL,
	.vars = {
		.head = &(pkg_node_t){
			.prev = NULL,
			.next = NULL,
			.data = &(pkg_tuple_t){
				.key = "pc_path",
				.value = PKG_DEFAULT_PATH,
			},
		},
		.tail = NULL,
	},
};

typedef bool (*pkg_vercmp_res_func_t)(pkg_t *pkg, pkg_dependency_t *pkgdep);

typedef struct {
	const char *name;
	pkg_comparator_t compare;
} pkg_comparator_name_t;

static pkg_comparator_name_t pkg_comparator_names[PKG_CMP_SIZE + 1] = {
	{"<",		PKG_LESS_THAN},
	{">",		PKG_GREATER_THAN},
	{"<=",		PKG_LESS_THAN_EQUAL},
	{">=",		PKG_GREATER_THAN_EQUAL},
	{"=",		PKG_EQUAL},
	{"!=",		PKG_NOT_EQUAL},
	{"(any)",	PKG_ANY},
	{"???",		PKG_CMP_SIZE},
};

static bool pkg_comparator_lt(pkg_t *pkg, pkg_dependency_t *pkgdep)
{
	return (pkg_compare_version(pkg->version, pkgdep->version) < 0);
}

static bool pkg_comparator_gt(pkg_t *pkg, pkg_dependency_t *pkgdep)
{
	return (pkg_compare_version(pkg->version, pkgdep->version) > 0);
}

static bool pkg_comparator_lte(pkg_t *pkg, pkg_dependency_t *pkgdep)
{
	return (pkg_compare_version(pkg->version, pkgdep->version) <= 0);
}

static bool pkg_comparator_gte(pkg_t *pkg, pkg_dependency_t *pkgdep)
{
	return (pkg_compare_version(pkg->version, pkgdep->version) >= 0);
}

static bool pkg_comparator_eq(pkg_t *pkg, pkg_dependency_t *pkgdep)
{
	return (pkg_compare_version(pkg->version, pkgdep->version) == 0);
}

static bool pkg_comparator_ne(pkg_t *pkg, pkg_dependency_t *pkgdep)
{
	return (pkg_compare_version(pkg->version, pkgdep->version) != 0);
}

static bool pkg_comparator_any(pkg_t *pkg, pkg_dependency_t *pkgdep)
{
	(void) pkg;
	(void) pkgdep;

	return true;
}

static bool pkg_comparator_unimplemented(pkg_t *pkg, pkg_dependency_t *pkgdep)
{
	(void) pkg;
	(void) pkgdep;

	return false;
}

static pkg_vercmp_res_func_t pkg_comparator_impls[PKG_CMP_SIZE + 1] = {
	[PKG_ANY]			= pkg_comparator_any,
	[PKG_LESS_THAN]			= pkg_comparator_lt,
	[PKG_GREATER_THAN]		= pkg_comparator_gt,
	[PKG_LESS_THAN_EQUAL]		= pkg_comparator_lte,
	[PKG_GREATER_THAN_EQUAL]	= pkg_comparator_gte,
	[PKG_EQUAL]			= pkg_comparator_eq,
	[PKG_NOT_EQUAL]			= pkg_comparator_ne,
	[PKG_CMP_SIZE]			= pkg_comparator_unimplemented,
};

/*
 * pkg_get_comparator(pkgdep)
 *
 * returns the comparator used in a depgraph dependency node as a string.
 */
const char *
pkg_get_comparator(pkg_dependency_t *pkgdep)
{
	const pkg_comparator_name_t *i;

	for (i = pkg_comparator_names; i->compare != PKG_CMP_SIZE; i++)
	{
		if (i->compare == pkgdep->compare)
			return i->name;
	}

	return "???";
}

/*
 * pkg_comparator_lookup_by_name(name)
 *
 * look up the appropriate comparator bytecode in the comparator set (defined
 * above, see pkg_comparator_names and pkg_comparator_impls).
 *
 * XXX: on error return PKG_ANY or maybe we should return PKG_CMP_SIZE which
 * is poisoned?
 */
pkg_comparator_t
pkg_comparator_lookup_by_name(const char *name)
{
	const pkg_comparator_name_t *i;

	for (i = pkg_comparator_names; i->compare != PKG_CMP_SIZE; i++)
	{
		if (!strcmp(i->name, name))
			return i->compare;
	}

	return PKG_ANY;
}

/*
 * pkg_verify_dependency(pkgdep, flags)
 *
 * verify a pkg_dependency_t node in the depgraph.  if the dependency is solvable,
 * return the appropriate pkg_t object, else NULL.
 */
pkg_t *
pkg_verify_dependency(pkg_dependency_t *pkgdep, unsigned int flags, unsigned int *eflags)
{
	pkg_t *pkg = &pkg_config_virtual;

	if (eflags != NULL)
		*eflags = PKG_ERRF_OK;

	/* pkg-config package name is special cased. */
	if (strcasecmp(pkgdep->package, "pkg-config"))
	{
		pkg = pkg_find(pkgdep->package, flags);
		if (pkg == NULL)
		{
			if (eflags != NULL)
				*eflags |= PKG_ERRF_PACKAGE_NOT_FOUND;

			return NULL;
		}
	}

	if (pkg->id == NULL)
		pkg->id = strdup(pkgdep->package);

	if (pkg_comparator_impls[pkgdep->compare](pkg, pkgdep) == true)
		return pkg;

	if (eflags != NULL)
		*eflags |= PKG_ERRF_PACKAGE_VER_MISMATCH;

	return pkg;
}

/*
 * pkg_verify_graph(root, depth)
 *
 * verify the graph dependency nodes are satisfiable by walking the tree using
 * pkg_traverse().
 */
unsigned int
pkg_verify_graph(pkg_t *root, int depth, unsigned int flags)
{
	return pkg_traverse(root, NULL, NULL, depth, flags);
}

static unsigned int
pkg_report_graph_error(pkg_t *parent, pkg_t *pkg, pkg_dependency_t *node, unsigned int eflags)
{
	static bool already_sent_notice = false;

	if (eflags & PKG_ERRF_PACKAGE_NOT_FOUND)
	{
		if (!already_sent_notice)
		{
			fprintf(error_msgout, "Package %s was not found in the pkg-config search path.\n", node->package);
			fprintf(error_msgout, "Perhaps you should add the directory containing `%s.pc'\n", node->package);
			fprintf(error_msgout, "to the PKG_CONFIG_PATH environment variable\n");
			already_sent_notice = true;
		}

		fprintf(error_msgout, "Package '%s', required by '%s', not found\n", node->package, parent->id);
	}
	else if (eflags & PKG_ERRF_PACKAGE_VER_MISMATCH)
	{
		fprintf(error_msgout, "Package dependency requirement '%s %s %s' could not be satisfied.\n",
			node->package, pkg_get_comparator(node), node->version);

		if (pkg != NULL)
			fprintf(error_msgout, "Package '%s' has version '%s', required version is '%s %s'\n",
				node->package, pkg->version, pkg_get_comparator(node), node->version);
	}

	if (pkg != NULL)
		pkg_unref(pkg);

	return eflags;
}

static inline unsigned int
pkg_walk_list(pkg_t *parent,
	pkg_list_t *deplist,
	pkg_traverse_func_t func,
	void *data,
	int depth,
	unsigned int flags)
{
	unsigned int eflags = PKG_ERRF_OK;
	pkg_node_t *node;

	PKG_FOREACH_LIST_ENTRY(deplist->head, node)
	{
		unsigned int eflags_local = PKG_ERRF_OK;
		pkg_dependency_t *depnode = node->data;
		pkg_t *pkgdep;

		if (*depnode->package == '\0')
			continue;

		pkgdep = pkg_verify_dependency(depnode, flags, &eflags_local);

		eflags |= eflags_local;
		if (eflags_local != PKG_ERRF_OK && !(flags & PKGF_SKIP_ERRORS))
		{
			pkg_report_graph_error(parent, pkgdep, depnode, eflags_local);
			continue;
		}
		if (pkgdep == NULL)
			continue;

		if (pkgdep->flags & PKG_PROPF_SEEN)
		{
			pkg_unref(pkgdep);
			continue;
		}

		pkgdep->flags |= PKG_PROPF_SEEN;
		eflags |= pkg_traverse(pkgdep, func, data, depth - 1, flags);
		pkgdep->flags &= ~PKG_PROPF_SEEN;
		pkg_unref(pkgdep);
	}

	return eflags;
}

static inline unsigned int
pkg_walk_conflicts_list(pkg_t *root, pkg_list_t *deplist, unsigned int flags)
{
	unsigned int eflags;
	pkg_node_t *node, *childnode;

	PKG_FOREACH_LIST_ENTRY(deplist->head, node)
	{
		pkg_dependency_t *parentnode = node->data;

		if (*parentnode->package == '\0')
			continue;

		PKG_FOREACH_LIST_ENTRY(root->requires.head, childnode)
		{
			pkg_t *pkgdep;
			pkg_dependency_t *depnode = childnode->data;

			if (*depnode->package == '\0' || strcmp(depnode->package, parentnode->package))
				continue;

			pkgdep = pkg_verify_dependency(parentnode, flags, &eflags);
			if (eflags == PKG_ERRF_OK)
			{
				fprintf(error_msgout, "Version '%s' of '%s' conflicts with '%s' due to satisfying conflict rule '%s %s%s%s'.\n",
					pkgdep->version, pkgdep->realname, root->realname, parentnode->package, pkg_get_comparator(parentnode),
					parentnode->version != NULL ? " " : "", parentnode->version != NULL ? parentnode->version : "");
				fprintf(error_msgout, "It may be possible to ignore this conflict and continue, try the\n");
				fprintf(error_msgout, "PKG_CONFIG_IGNORE_CONFLICTS environment variable.\n");

				pkg_unref(pkgdep);

				return PKG_ERRF_PACKAGE_CONFLICT;
			}

			pkg_unref(pkgdep);
		}
	}

	return PKG_ERRF_OK;
}

/*
 * pkg_traverse(root, func, data, maxdepth, flags)
 *
 * walk the dependency graph up to maxdepth levels.  -1 means infinite recursion.
 */
unsigned int
pkg_traverse(pkg_t *root,
	pkg_traverse_func_t func,
	void *data,
	int maxdepth,
	unsigned int flags)
{
	unsigned int rflags = flags & ~PKGF_SKIP_ROOT_VIRTUAL;
	unsigned int eflags = PKG_ERRF_OK;

	if (maxdepth == 0)
		return eflags;

	if ((root->flags & PKG_PROPF_VIRTUAL) != PKG_PROPF_VIRTUAL || (flags & PKGF_SKIP_ROOT_VIRTUAL) != PKGF_SKIP_ROOT_VIRTUAL)
	{
		if (func != NULL)
			func(root, data, flags);
	}

	if (!(flags & PKGF_SKIP_CONFLICTS))
	{
		eflags = pkg_walk_conflicts_list(root, &root->conflicts, rflags);
		if (eflags != PKG_ERRF_OK)
			return eflags;
	}

	eflags = pkg_walk_list(root, &root->requires, func, data, maxdepth, rflags);
	if (eflags != PKG_ERRF_OK)
		return eflags;

	if (flags & PKGF_SEARCH_PRIVATE)
	{
		eflags = pkg_walk_list(root, &root->requires_private, func, data, maxdepth, rflags | PKGF_ITER_PKG_IS_PRIVATE);
		if (eflags != PKG_ERRF_OK)
			return eflags;
	}

	return eflags;
}

static void
pkg_cflags_collect(pkg_t *pkg, void *data, unsigned int flags)
{
	pkg_list_t *list = data;
	pkg_node_t *node;
	(void) flags;

	PKG_FOREACH_LIST_ENTRY(pkg->cflags.head, node)
	{
		pkg_fragment_t *frag = node->data;
		pkg_fragment_copy(list, frag, flags, false);
	}
}

static void
pkg_cflags_private_collect(pkg_t *pkg, void *data, unsigned int flags)
{
	pkg_list_t *list = data;
	pkg_node_t *node;
	(void) flags;

	PKG_FOREACH_LIST_ENTRY(pkg->cflags_private.head, node)
	{
		pkg_fragment_t *frag = node->data;

		pkg_fragment_copy(list, frag, flags, true);
	}
}

int
pkg_cflags(pkg_t *root, pkg_list_t *list, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkg_traverse(root, pkg_cflags_collect, list, maxdepth, flags);
	if (eflag != PKG_ERRF_OK)
		pkg_fragment_free(list);

	if (flags & PKGF_MERGE_PRIVATE_FRAGMENTS)
	{
		eflag = pkg_traverse(root, pkg_cflags_private_collect, list, maxdepth, flags);
		if (eflag != PKG_ERRF_OK)
			pkg_fragment_free(list);
	}

	return eflag;
}

static void
pkg_libs_collect(pkg_t *pkg, void *data, unsigned int flags)
{
	pkg_list_t *list = data;
	pkg_node_t *node;
	(void) flags;

	PKG_FOREACH_LIST_ENTRY(pkg->libs.head, node)
	{
		pkg_fragment_t *frag = node->data;
		pkg_fragment_copy(list, frag, flags, (flags & PKGF_ITER_PKG_IS_PRIVATE) != 0);
	}

	if (flags & PKGF_MERGE_PRIVATE_FRAGMENTS)
	{
		PKG_FOREACH_LIST_ENTRY(pkg->libs_private.head, node)
		{
			pkg_fragment_t *frag = node->data;
			pkg_fragment_copy(list, frag, flags, true);
		}
	}
}

int
pkg_libs(pkg_t *root, pkg_list_t *list, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkg_traverse(root, pkg_libs_collect, list, maxdepth, flags);

	if (eflag != PKG_ERRF_OK)
	{
		pkg_fragment_free(list);
		return eflag;
	}

	return eflag;
}
