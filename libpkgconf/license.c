/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *​ Copyright (c) 2026 The FreeBSD Foundation
 *​
 *​ Portions of this software were developed by
 * Tuukka Pasanen <tuukka.pasanen@ilmi.fi> under sponsorship from
 * the FreeBSD Foundation
 */

#include <libpkgconf/stdinc.h>
#include <libpkgconf/libpkgconf.h>

static void license_sanitize_string(const char *s, pkgconf_buffer_t *buf)
{
	unsigned int i = 0;
	/*
	 * Allowed chars are:
	 * - a-z
	 * - 0-9
	 * - chars '-', '+', '.', '(' and ')'
	 */
	for (i = 0; i < strlen(s); i ++)
	{
		if (isalnum(s[i]) || s[i] == '-' || s[i] == '+' || s[i] == '(' || s[i] == ')' || s[i] == '.' || s[i] == ':')
		{
			pkgconf_buffer_push_byte(buf, s[i]);
		}
	}
}


/*
 * !doc
 *
 * .. c:function:: pkgconf_license_evaluate_str(pkgconf_client_t *client, pkgconf_list_t *license_list, const char *expression, unsigned int flags)
 *
 * Evaluates SPDX expression strings like:
 * - BSD-3-Clause
 * - LGPL-2.1-only OR MIT
 * - LGPL-2.1-only OR MIT OR BSD-3-Clause
 * - ISC AND (BSD-3-Clause AND BSD-2-Clause)
 *
 * Function parses and sanitizes license strings. Also adding multiple
 * 'License:'-keys is supported like:
 * License: BSD-3-Clause
 * License: BSD-2-Clause
 *
 * Which will evaluate to: BSD-3-Clause AND BSD-2-Clause
 *
 * :param pkgconf_client_t* client: The client object that owns the package this dependency list belongs to.
 * :param pkgconf_list_t* license_list: The dependency list to populate with dependency nodes.
 * :param char* expression: The dependency data to parse.
 * :param uint flags: Any flags to attach to the dependency nodes.
 * :return: nothing
 */
void
pkgconf_license_evaluate_str(pkgconf_client_t *client, pkgconf_list_t *license_list, const char *expression, unsigned int flags)
{
	pkgconf_buffer_t out_buffer = PKGCONF_BUFFER_INITIALIZER;
	unsigned int buf_size = 0;
	char *cur_word = NULL;
	int i, ret, argc;
	char **argv;
	int string_len = 0;

	(void)flags;

	buf_size = strlen(expression) + 1;
	ret = pkgconf_argv_split(expression, &argc, &argv);
	if (!ret)
	{
		i = 0;
		/* This is not the first License:
		 * so add AND
		 */
		if (license_list->head)
		{
			pkgconf_fragment_insert(client, license_list, 'A', "AND", true);
		}

		/*
		 * Fragments have types like this
		 * 'L' is License
		 * 'A' is AND
		 * 'O' is OR
		 * 'W' is WITH
		 * '(' is Bracket open
		 * ')' is Bracket close
		 */
		while (i < argc)
		{
			string_len = strnlen(argv[i], buf_size);
			cur_word = argv[i];
			if (string_len >= 1)
			{
				license_sanitize_string(cur_word, &out_buffer);
				cur_word = (char *)pkgconf_buffer_str_or_empty(&out_buffer);
				if (!strnlen(cur_word, buf_size))
				{
					i ++;
					pkgconf_buffer_finalize(&out_buffer);
					continue;
				}

				if (cur_word[0] == '(')
				{
					pkgconf_fragment_insert(client, license_list, '(', "(", true);
					/* If there is more after '(' like '(BSD-2-Clause'
					 * Then append rest to fragments as license.
					 * This is expression like GPL-2.0-only OR (BSD-2-Clause AND ISC)
					 */
					if (string_len >= 2)
					{
						cur_word ++;
						pkgconf_fragment_insert(client, license_list, 'L', cur_word, true);
					}
				}
				else if (cur_word[string_len - 1] == ')')
				{
					if (string_len >= 2)
					{
						argv[i][string_len - 1] = 0x00;
						pkgconf_fragment_insert(client, license_list, 'L', cur_word, true);
					}
					pkgconf_fragment_insert(client, license_list, ')', ")", true);
				}
				else if (!strncasecmp(cur_word, "and", 3))
				{
					pkgconf_fragment_insert(client, license_list, 'A', "AND", true);
				}
				else if (!strncasecmp(cur_word, "or", 2))
				{
					pkgconf_fragment_insert(client, license_list, 'O', "OR", true);
				}
				else if (!strncasecmp(cur_word, "with", 2))
				{
					pkgconf_fragment_insert(client, license_list, 'W', "WITH", true);
				}
				else
				{
					pkgconf_fragment_insert(client, license_list, 'L', cur_word, true);
				}
				pkgconf_buffer_finalize(&out_buffer);
			}
			i++;
		}
	}

	pkgconf_argv_free(argv);
}

/*
 * !doc
 *
 * .. c:function:: pkgconf_license_evaluate_str(pkgconf_client_t *client, pkgconf_list_t *license_list, const char *expression, unsigned int flags)
 *
 * Evaluates SPDX expression strings like:
 * - BSD-3-Clause
 * - LGPL-2.1-only OR MIT
 * - LGPL-2.1-only OR MIT OR BSD-3-Clause
 * - ISC AND (BSD-3-Clause AND BSD-2-Clause)
 *
 * Function parses and sanitizes license strings. Also adding multiple
 * 'License:'-keys is supported like:
 * License: BSD-3-Clause
 * License: BSD-2-Clause
 *
 * Which will evaluate to: BSD-3-Clause AND BSD-2-Clause
 *
 * :param pkgconf_client_t* client: The client object that owns the package this dependency list belongs to.
 * :param pkgconf_list_t* license_list: The dependency list to populate with dependency nodes.
 * :param char* expression: The dependency data to parse.
 * :param uint flags: Any flags to attach to the dependency nodes.
 * :return: nothing
 */
void
pkgconf_license_evaluate(pkgconf_client_t *client, pkgconf_pkg_t *pkg, pkgconf_list_t *license_list, const char *license_str, unsigned int flags)
{
	char *license_expression = pkgconf_bytecode_eval_str(client, &pkg->vars, license_str, NULL);

	pkgconf_license_evaluate_str(client, license_list, license_expression, flags);
	free(license_expression);
}

/*
 * !doc
 *
 * .. c:function:: void pkgconf_dependency_parse_str(pkgconf_list_t *deplist_head, const char *depends)
 *
 * Renders license fragments back to SPDX expression string. Tries
 * to keep output as close as possible to original input
 *
 * :param pkgconf_client_t* client: The client object that owns the package this dependency list belongs to.
 * :param pkgconf_list_t* deplist_head: The dependency list to populate with dependency nodes.
 * :param char* depends: The dependency data to parse.
 * :param uint flags: Any flags to attach to the dependency nodes.
 * :return: nothing
 */
void
pkgconf_license_fragment_render(pkgconf_client_t *client, const pkgconf_list_t *list, pkgconf_buffer_t *buf)
{
	const pkgconf_buffer_t *frag_string = NULL;
	pkgconf_node_t *node;
	bool is_delim = true;

	(void)client;

	PKGCONF_FOREACH_LIST_ENTRY(list->head, node)
	{
		pkgconf_fragment_t *frag = node->data;
		is_delim = true;
		frag_string = PKGCONF_BUFFER_FROM_STR(frag->data);
		pkgconf_buffer_append(buf, pkgconf_buffer_str_or_empty(frag_string));

		if (frag->type == '(' || (node->next != NULL && ((const pkgconf_fragment_t *)node->next)->type == ')'))
		{
			is_delim = false;
		}

		if (node->next != NULL && is_delim)
		{
			pkgconf_buffer_push_byte(buf, ' ');
		}
	}
}
