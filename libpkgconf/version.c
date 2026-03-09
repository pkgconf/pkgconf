/*
 * version.c
 * version comparison
 *
 * Copyright (c) 2011-2026 pkgconf authors (see AUTHORS).
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * This software is provided 'as is' and without any warranty, express or
 * implied.  In no event shall the authors be liable for any damages arising
 * from the use of this software.
 */

#include <libpkgconf/config.h>
#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>

/*
 * !doc
 *
 * .. c:function:: int pkgconf_compare_version(const char *a, const char *b)
 *
 *    Compare versions using RPM version comparison rules as described in the LSB.
 *
 *    :param char* a: The first version to compare in the pair.
 *    :param char* b: The second version to compare in the pair.
 *    :return: -1 if the first version is less than, 0 if both versions are equal, 1 if the second version is less than.
 *    :rtype: int
 */
int
pkgconf_compare_version(const char *a, const char *b)
{
	char oldch1, oldch2;
	char buf1[PKGCONF_ITEM_SIZE], buf2[PKGCONF_ITEM_SIZE];
	char *str1, *str2;
	char *one, *two;
	int ret;
	bool isnum;

	/* optimization: if version matches then it's the same version. */
	if (a == NULL)
		return -1;

	if (b == NULL)
		return 1;

	if (!strcasecmp(a, b))
		return 0;

	pkgconf_strlcpy(buf1, a, sizeof buf1);
	pkgconf_strlcpy(buf2, b, sizeof buf2);

	one = buf1;
	two = buf2;

	while (*one || *two)
	{
		while (*one && !isalnum((unsigned char)*one) && *one != '~')
			one++;
		while (*two && !isalnum((unsigned char)*two) && *two != '~')
			two++;

		if (*one == '~' || *two == '~')
		{
			if (*one != '~')
				return 1;
			if (*two != '~')
				return -1;

			one++;
			two++;
			continue;
		}

		if (!(*one && *two))
			break;

		str1 = one;
		str2 = two;

		if (isdigit((unsigned char)*str1))
		{
			while (*str1 && isdigit((unsigned char)*str1))
				str1++;

			while (*str2 && isdigit((unsigned char)*str2))
				str2++;

			isnum = true;
		}
		else
		{
			while (*str1 && isalpha((unsigned char)*str1))
				str1++;

			while (*str2 && isalpha((unsigned char)*str2))
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
			size_t onelen, twolen;

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
		if (ret != 0)
			return ret < 0 ? -1 : 1;

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
