/*
 * pkg.c
 * higher-level dependency graph compilation, management and manipulation
 *
 * Copyright (c) 2011, 2012 William Pitcock <nenolod@dereferenced.org>.
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

#include "pkg.h"
#include "bsdstubs.h"

#ifdef _WIN32
#	define PKG_CONFIG_REG_KEY "Software\\pkgconfig\\PKG_CONFIG_PATH"
#endif

#define PKG_CONFIG_EXT		".pc"
#define PKG_CONFIG_PATH_SZ	(65535)

/* pkg-config uses ';' on win32 as ':' is part of path */
#ifdef _WIN32
#define PKG_CONFIG_PATH_SEP_S	";"
#else
#define PKG_CONFIG_PATH_SEP_S	":"
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

/*
 * pkg_new_from_file(filename, file)
 *
 * Parse a .pc file into a pkg_t object structure.
 */
pkg_t *
pkg_new_from_file(const char *filename, FILE *f)
{
	pkg_t *pkg;
	char readbuf[BUFSIZ];

	pkg = calloc(sizeof(pkg_t), 1);
	pkg->filename = strdup(filename);

	while (pkg_fgetline(readbuf, BUFSIZ, f) != NULL)
	{
		char op, *p, *key = NULL, *value = NULL;

		readbuf[strlen(readbuf) - 1] = '\0';

		p = readbuf;
		while (*p && (isalpha(*p) || isdigit(*p) || *p == '_' || *p == '.'))
			p++;

		key = strndup(readbuf, p - readbuf);
		if (!isalpha(*key) && !isdigit(*p))
			goto cleanup;

		while (*p && isspace(*p))
			p++;

		op = *p++;

		while (*p && isspace(*p))
			p++;

		value = strdup(p);

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

cleanup:
		if (key)
			free(key);

		if (value)
			free(value);
	}

	fclose(f);
	return pkg;
}

void
pkg_free(pkg_t *pkg)
{
	if (pkg == NULL)
		return;

	pkg_dependency_free(pkg->requires);
	pkg_dependency_free(pkg->requires_private);
	pkg_dependency_free(pkg->conflicts);

	pkg_fragment_free(pkg->cflags);
	pkg_fragment_free(pkg->libs);
	pkg_fragment_free(pkg->libs_private);

	pkg_tuple_free(pkg->vars);

	if (pkg->flags & PKG_PROPF_VIRTUAL)
		return;

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
		BYTE pathbuf[PKG_CONFIG_PATH_SZ];
		DWORD type;
		DWORD pathbuflen = sizeof pathbuf;

		if (RegQueryValueEx(key, buf, NULL, &type, pathbuf, &pathbuflen)
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
	}

	env_path = getenv("PKG_CONFIG_LIBDIR");
	if (env_path == NULL)
		env_path = PKG_DEFAULT_PATH;

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
	char buf1[BUFSIZ], buf2[BUFSIZ];
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

	while (*one && *two)
	{
		while (*one && !isalnum(*one))
			one++;
		while (*two && !isalnum(*two))
			two++;

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
	.version = PKG_PKGCONFIG_VERSION_EQUIV,
	.flags = PKG_PROPF_VIRTUAL,
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
		fprintf(stderr, "Package %s was not found in the pkg-config search path.\n", node->package);
		fprintf(stderr, "Perhaps you should add the directory containing `%s.pc'\n", node->package);
		fprintf(stderr, "to the PKG_CONFIG_PATH environment variable\n");
		fprintf(stderr, "No package '%s' found\n", node->package);
	}
	else if (eflags & PKG_ERRF_PACKAGE_VER_MISMATCH)
	{
		fprintf(stderr, "Package dependency requirement '%s %s %s' could not be satisfied.\n",
			node->package, pkg_get_comparator(node), node->version);

		if (pkg != NULL)
			fprintf(stderr, "Package '%s' has version '%s', required version is '%s %s'\n",
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

		pkg_traverse(pkgdep, func, data, depth - 1, flags);

		pkg_free(pkgdep);
	}

	return eflags;
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

	eflags = pkg_walk_list(root->requires, func, data, maxdepth, rflags);
	if (eflags != PKG_ERRF_OK)
		return eflags;

	if (flags & PKGF_SEARCH_PRIVATE)
	{
		eflags = pkg_walk_list(root->requires_private, func, data, maxdepth, rflags);
		if (eflags != PKG_ERRF_OK)
			return eflags;
	}

	if ((root->flags & PKG_PROPF_VIRTUAL) && (flags & PKGF_SKIP_ROOT_VIRTUAL))
		return eflags;

	if (func != NULL)
		func(root, data, flags);

	return eflags;
}
