/*
 * pkg.c
 * higher-level dependency graph compilation, management and manipulation
 *
 * Copyright (c) 2011, 2012 pkgconf authors (see AUTHORS).
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
#	define PKG_DEFAULT_PATH "../lib/pkgconfig:../share/pkgconfig"
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

static inline size_t
path_split(const char *text, char ***parv)
{
	size_t count = 0;
	char *workbuf, *p, *iter;

	if (text == NULL)
		return 0;

	*parv = malloc(sizeof (void *));

	iter = workbuf = strdup(text);
	while ((p = strtok(iter, PKG_CONFIG_PATH_SEP_S)) != NULL)
	{
		(*parv)[count] = strdup(p);
		count++, iter = NULL;

		*parv = realloc(*parv, sizeof (void *) * (count + 1));
	}
	free(workbuf);

	return count;
}

static inline void
path_free(char **parv, size_t count)
{
	size_t iter;

	if (parv == NULL)
		return;

	for (iter = 0; iter < count; iter++)
		free(parv[iter]);

	free(parv);
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
	GetModuleFileName(NULL, namebuf, sizeof namebuf);

	p = strrchr(namebuf, '\\');
	if (p == NULL)
		p = strrchr(namebuf, '/');
	if (p == NULL)
		return PKG_DEFAULT_PATH;

	*p = '\0';
	strlcpy(outbuf, namebuf, sizeof outbuf);
	strlcat(outbuf, "../lib/pkgconfig", sizeof outbuf);
	strlcat(outbuf, ":", sizeof outbuf);
	strlcat(outbuf, namebuf, sizeof outbuf);
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

/*
 * pkg_new_from_file(filename, file)
 *
 * Parse a .pc file into a pkg_t object structure.
 */
pkg_t *
pkg_new_from_file(const char *filename, FILE *f)
{
	pkg_t *pkg;
	char readbuf[PKG_BUFSIZE];
	char *idptr;

	pkg = calloc(sizeof(pkg_t), 1);
	pkg->filename = strdup(filename);
	pkg->vars = pkg_tuple_add(pkg->vars, "pcfiledir", pkg_get_parent_dir(pkg));

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

		readbuf[strlen(readbuf) - 1] = '\0';

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
			if (!strcasecmp(key, "Name"))
				pkg->realname = pkg_tuple_parse(pkg->vars, value);
			else if (!strcasecmp(key, "Description"))
				pkg->description = pkg_tuple_parse(pkg->vars, value);
			else if (!strcasecmp(key, "Version"))
				pkg->version = pkg_tuple_parse(pkg->vars, value);
			else if (!strcasecmp(key, "CFLAGS"))
				pkg->cflags = pkg_fragment_parse(pkg->cflags, pkg->vars, value);
			else if (!strcasecmp(key, "LIBS"))
				pkg->libs = pkg_fragment_parse(pkg->libs, pkg->vars, value);
			else if (!strcasecmp(key, "LIBS.private"))
				pkg->libs_private = pkg_fragment_parse(pkg->libs_private, pkg->vars, value);
			else if (!strcasecmp(key, "Requires"))
				pkg->requires = pkg_dependency_parse(pkg, value);
			else if (!strcasecmp(key, "Requires.private"))
				pkg->requires_private = pkg_dependency_parse(pkg, value);
			else if (!strcasecmp(key, "Conflicts"))
				pkg->conflicts = pkg_dependency_parse(pkg, value);
			break;
		case '=':
			pkg->vars = pkg_tuple_add(pkg->vars, key, value);
			break;
		default:
			break;
		}
	}

	fclose(f);
	return pkg;
}

void
pkg_free(pkg_t *pkg)
{
	if (pkg == NULL || pkg->flags & PKG_PROPF_VIRTUAL)
		return;

	pkg_dependency_free(pkg->requires);
	pkg_dependency_free(pkg->requires_private);
	pkg_dependency_free(pkg->conflicts);

	pkg_fragment_free(pkg->cflags);
	pkg_fragment_free(pkg->libs);
	pkg_fragment_free(pkg->libs_private);

	pkg_tuple_free(pkg->vars);

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
		pkg = pkg_new_from_file(uninst_locbuf, f);
		pkg->uninstalled = true;
	}
	else if ((f = fopen(locbuf, "r")) != NULL)
		pkg = pkg_new_from_file(locbuf, f);

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

		pkg = pkg_new_from_file(filebuf, f);
		if (pkg != NULL)
		{
			func(pkg);
			pkg_free(pkg);
		}
	}

	closedir(dir);
}

void
pkg_scan(const char *search_path, pkg_iteration_func_t func)
{
	char **path = NULL;
	size_t count = 0, iter = 0;

	/* PKG_CONFIG_PATH has to take precedence */
	if (search_path == NULL)
		return;

	count = path_split(search_path, &path);

	for (iter = 0; iter < count; iter++)
		pkg_scan_dir(path[iter], func);

	path_free(path, count);
}

void
pkg_scan_all(pkg_iteration_func_t func)
{
	char *path;

	path = getenv("PKG_CONFIG_PATH");
	if (path)
	{
		pkg_scan(path, func);
		return;
	}

	pkg_scan(get_pkgconfig_path(), func);
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
	char **path = NULL;
	size_t count = 0, iter = 0;
	const char *env_path;
	pkg_t *pkg = NULL;
	FILE *f;

	/* name might actually be a filename. */
	if (str_has_suffix(name, PKG_CONFIG_EXT))
	{
		if ((f = fopen(name, "r")) != NULL)
			return pkg_new_from_file(name, f);
	}

	/* PKG_CONFIG_PATH has to take precedence */
	env_path = getenv("PKG_CONFIG_PATH");
	if (env_path)
	{
		count = path_split(env_path, &path);

		for (iter = 0; iter < count; iter++)
		{
			pkg = pkg_try_specific_path(path[iter], name, flags);
			if (pkg != NULL)
				goto out;
		}

		path_free(path, count);
	}

	env_path = get_pkgconfig_path();
	if (!(flags & PKGF_ENV_ONLY))
	{
		count = path_split(env_path, &path);

		for (iter = 0; iter < count; iter++)
		{
			pkg = pkg_try_specific_path(path[iter], name, flags);
			if (pkg != NULL)
				goto out;
		}
	}

#ifdef _WIN32
	/* support getting PKG_CONFIG_PATH from registry */
	pkg = pkg_find_in_registry_key(HKEY_CURRENT_USER, name, flags);
	if (!pkg)
		pkg = pkg_find_in_registry_key(HKEY_LOCAL_MACHINE, name, flags);
#endif

out:
	path_free(path, count);
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

/*
 * pkg_get_comparator(pkgdep)
 *
 * returns the comparator used in a depgraph dependency node as a string.
 */
const char *
pkg_get_comparator(pkg_dependency_t *pkgdep)
{
	switch(pkgdep->compare)
	{
	case PKG_LESS_THAN:
		return "<";
	case PKG_GREATER_THAN:
		return ">";
	case PKG_LESS_THAN_EQUAL:
		return "<=";
	case PKG_GREATER_THAN_EQUAL:
		return ">=";
	case PKG_EQUAL:
		return "=";
	case PKG_NOT_EQUAL:
		return "!=";
	case PKG_ANY:
		return "(any)";
	default:
		return "???";
	}

	return "???";
}

static pkg_t pkg_config_virtual = {
	.id = "pkg-config",
	.realname = "pkg-config",
	.description = "virtual package defining pkg-config API version supported",
	.url = PACKAGE_BUGREPORT,
	.version = PKG_PKGCONFIG_VERSION_EQUIV,
	.flags = PKG_PROPF_VIRTUAL,
	.vars = &(pkg_tuple_t){
		.key = "pc_path",
		.value = PKG_DEFAULT_PATH,
	},
};

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

	switch(pkgdep->compare)
	{
	case PKG_LESS_THAN:
		if (pkg_compare_version(pkg->version, pkgdep->version) < 0)
			return pkg;
		break;
	case PKG_GREATER_THAN:
		if (pkg_compare_version(pkg->version, pkgdep->version) > 0)
			return pkg;
		break;
	case PKG_LESS_THAN_EQUAL:
		if (pkg_compare_version(pkg->version, pkgdep->version) <= 0)
			return pkg;
		break;
	case PKG_GREATER_THAN_EQUAL:
		if (pkg_compare_version(pkg->version, pkgdep->version) >= 0)
			return pkg;
		break;
	case PKG_EQUAL:
		if (pkg_compare_version(pkg->version, pkgdep->version) == 0)
			return pkg;
		break;
	case PKG_NOT_EQUAL:
		if (pkg_compare_version(pkg->version, pkgdep->version) != 0)
			return pkg;
		break;
	case PKG_ANY:
	default:
		return pkg;
	}

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

unsigned int
pkg_report_graph_error(pkg_t *pkg, pkg_dependency_t *node, unsigned int eflags)
{
	if (eflags & PKG_ERRF_PACKAGE_NOT_FOUND)
	{
		fprintf(error_msgout, "Package %s was not found in the pkg-config search path.\n", node->package);
		fprintf(error_msgout, "Perhaps you should add the directory containing `%s.pc'\n", node->package);
		fprintf(error_msgout, "to the PKG_CONFIG_PATH environment variable\n");
		fprintf(error_msgout, "No package '%s' found\n", node->package);
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
		pkg_free(pkg);

	return eflags;
}

static inline unsigned int
pkg_walk_list(pkg_dependency_t *deplist,
	pkg_traverse_func_t func,
	void *data,
	int depth,
	unsigned int flags)
{
	unsigned int eflags = PKG_ERRF_OK;
	pkg_dependency_t *node;

	PKG_FOREACH_LIST_ENTRY(deplist, node)
	{
		pkg_t *pkgdep;

		if (*node->package == '\0')
			continue;

		pkgdep = pkg_verify_dependency(node, flags, &eflags);
		if (eflags != PKG_ERRF_OK)
			return pkg_report_graph_error(pkgdep, node, eflags);

		eflags = pkg_traverse(pkgdep, func, data, depth - 1, flags);
		pkg_free(pkgdep);

		/* optimization: if a break has been found in the depgraph, quit walking it */
		if (eflags != PKG_ERRF_OK)
			break;
	}

	return eflags;
}

static inline unsigned int
pkg_walk_conflicts_list(pkg_t *root, pkg_dependency_t *deplist, unsigned int flags)
{
	unsigned int eflags;
	pkg_dependency_t *node, *depnode;

	PKG_FOREACH_LIST_ENTRY(deplist, node)
	{
		if (*node->package == '\0')
			continue;

		PKG_FOREACH_LIST_ENTRY(root->requires, depnode)
		{
			pkg_t *pkgdep;

			if (*depnode->package == '\0' || strcmp(depnode->package, node->package))
				continue;

			pkgdep = pkg_verify_dependency(node, flags, &eflags);
			if (eflags == PKG_ERRF_OK)
			{
				fprintf(error_msgout, "Version '%s' of '%s' conflicts with '%s' due to satisfying conflict rule '%s %s%s%s'.\n",
					pkgdep->version, pkgdep->realname, root->realname, node->package, pkg_get_comparator(node),
					node->version != NULL ? " " : "", node->version != NULL ? node->version : "");
				fprintf(error_msgout, "It may be possible to ignore this conflict and continue, try the\n");
				fprintf(error_msgout, "PKG_CONFIG_IGNORE_CONFLICTS environment variable.\n");

				pkg_free(pkgdep);

				return PKG_ERRF_PACKAGE_CONFLICT;
			}

			pkg_free(pkgdep);
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
		eflags = pkg_walk_conflicts_list(root, root->conflicts, rflags);
		if (eflags != PKG_ERRF_OK)
			return eflags;
	}

	eflags = pkg_walk_list(root->requires, func, data, maxdepth, rflags);
	if (eflags != PKG_ERRF_OK)
		return eflags;

	if (flags & PKGF_SEARCH_PRIVATE)
	{
		eflags = pkg_walk_list(root->requires_private, func, data, maxdepth, rflags);
		if (eflags != PKG_ERRF_OK)
			return eflags;
	}

	return eflags;
}

static void
pkg_cflags_collect(pkg_t *pkg, void *data, unsigned int flags)
{
	pkg_fragment_t **list = data;
	pkg_fragment_t *frag;
	(void) flags;

	PKG_FOREACH_LIST_ENTRY(pkg->cflags, frag)
		*list = pkg_fragment_copy(*list, frag);
}

int
pkg_cflags(pkg_t *root, pkg_fragment_t **list, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkg_traverse(root, pkg_cflags_collect, list, maxdepth, flags);

	if (eflag != PKG_ERRF_OK)
	{
		pkg_fragment_free(*list);
		*list = NULL;
	}

	return eflag;
}

static void
pkg_libs_collect(pkg_t *pkg, void *data, unsigned int flags)
{
	pkg_fragment_t **list = data;
	pkg_fragment_t *frag;
	(void) flags;

	PKG_FOREACH_LIST_ENTRY(pkg->libs, frag)
		*list = pkg_fragment_copy(*list, frag);
}

static void
pkg_libs_private_collect(pkg_t *pkg, void *data, unsigned int flags)
{
	pkg_fragment_t **list = data;
	pkg_fragment_t *frag;
	(void) flags;

	PKG_FOREACH_LIST_ENTRY(pkg->libs_private, frag)
		*list = pkg_fragment_copy(*list, frag);
}

int
pkg_libs(pkg_t *root, pkg_fragment_t **list, int maxdepth, unsigned int flags)
{
	int eflag;

	eflag = pkg_traverse(root, pkg_libs_collect, list, maxdepth, flags);

	if (eflag != PKG_ERRF_OK)
	{
		pkg_fragment_free(*list);
		*list = NULL;
		return eflag;
	}

	if (flags & PKGF_MERGE_PRIVATE_FRAGMENTS)
	{
		eflag = pkg_traverse(root, pkg_libs_private_collect, list, maxdepth, flags);
		if (eflag != PKG_ERRF_OK)
		{
			pkg_fragment_free(*list);
			*list = NULL;
		}
	}

	return eflag;
}
