/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <histedit.h>

#include <ecoli/complete.h>
#include <ecoli/dict.h>
#include <ecoli/interact.h>
#include <ecoli/node.h>
#include <ecoli/parse.h>
#include <ecoli/string.h>
#include <ecoli/strvec.h>
#include <ecoli/utils.h>

/* used by qsort below */
static int strcasecmp_cb(const void *p1, const void *p2)
{
	return strcasecmp(*(char *const *)p1, *(char *const *)p2);
}

/* Show the matches as a multi-columns list */
int ec_interact_print_cols(FILE *out, unsigned int width, char const *const *matches, size_t n)
{
	size_t max_strlen = 0, len, i, j, ncols;
	const char *space;
	char **matches_copy = NULL;

	fprintf(out, "\n");
	if (n == 0)
		return 0;

	/* duplicate the matches table, and sort it */
	matches_copy = calloc(n, sizeof(const char *));
	if (matches_copy == NULL)
		return -1;
	memcpy(matches_copy, matches, sizeof(const char *) * n);
	qsort(matches_copy, n, sizeof(char *), strcasecmp_cb);

	/* get max string length */
	for (i = 0; i < n; i++) {
		len = strlen(matches_copy[i]);
		if (len > max_strlen)
			max_strlen = len;
	}

	/* write the columns */
	ncols = width / (max_strlen + 4);
	if (ncols == 0)
		ncols = 1;
	for (i = 0; i < n; i += ncols) {
		for (j = 0; j < ncols; j++) {
			if (i + j >= n)
				break;
			if (j == 0)
				space = "";
			else
				space = "    ";
			fprintf(out, "%s%-*s", space, (int)max_strlen, matches[i + j]);
		}
		fprintf(out, "\n");
	}

	free(matches_copy);
	return 0;
}

/* Show the helps on command line output */
int ec_interact_print_helps(
	FILE *out,
	unsigned int width,
	const struct ec_interact_help *helps,
	size_t len
)
{
	char *wrapped_help = NULL;
	size_t i;

	for (i = 0; i < len; i++) {
		wrapped_help = ec_str_wrap(helps[i].help, width, 23);
		if (wrapped_help == NULL)
			goto fail;
		if (strlen(helps[i].desc) > 20) {
			if (fprintf(out, "  %s\n", helps[i].desc) < 0)
				goto fail;
			if (fprintf(out, "  %-20s %s\n", "", wrapped_help) < 0)
				goto fail;
		} else {
			if (fprintf(out, "  %-20s %s\n", helps[i].desc, wrapped_help) < 0)
				goto fail;
		}
		free(wrapped_help);
		wrapped_help = NULL;
	}

	return 0;

fail:
	free(wrapped_help);
	return -1;
}

void ec_interact_free_helps(struct ec_interact_help *helps, size_t n)
{
	size_t i;

	if (helps == NULL)
		return;
	for (i = 0; i < n; i++) {
		free(helps[i].desc);
		free(helps[i].help);
	}
	free(helps);
}

void ec_interact_free_completions(char **matches, size_t len)
{
	size_t i;

	if (matches == NULL)
		return;
	for (i = 0; i < len; i++)
		free(matches[i]);
	free(matches);
}

ssize_t ec_interact_get_completions(
	const struct ec_comp *cmpl,
	char ***matches_out,
	enum ec_comp_type type_mask
)
{
	struct ec_comp_item *item;
	char **matches = NULL;
	size_t count = 0;

	EC_COMP_FOREACH (item, cmpl, type_mask) {
		char **tmp;

		tmp = realloc(matches, (count + 1) * sizeof(char *));
		if (tmp == NULL)
			goto fail;
		matches = tmp;
		matches[count] = strdup(ec_comp_item_get_display(item));
		if (matches[count] == NULL)
			goto fail;
		count++;
	}

	*matches_out = matches;
	return count;

fail:
	ec_interact_free_completions(matches, count);
	*matches_out = NULL;
	return -1;
}

char *ec_interact_append_chars(const struct ec_comp *cmpl)
{
	struct ec_comp_item *item;
	const char *append;
	char *ret = NULL;
	size_t n;

	EC_COMP_FOREACH (item, cmpl, EC_COMP_FULL | EC_COMP_PARTIAL) {
		append = ec_comp_item_get_completion(item);
		if (ret == NULL) {
			ret = strdup(append);
			if (ret == NULL)
				goto fail;
		} else {
			n = ec_strcmp_count(ret, append);
			ret[n] = '\0';
		}
	}

	return ret;

fail:
	free(ret);

	return NULL;
}

/* this function builds the help string */
static int get_node_help(const struct ec_comp_item *item, struct ec_interact_help *help)
{
	const struct ec_comp_group *grp;
	const struct ec_pnode *pstate;
	const struct ec_node *node;
	const char *node_help = NULL;
	const char *node_desc = NULL;
	char *desc_to_free = NULL;

	help->desc = NULL;
	help->help = NULL;

	grp = ec_comp_item_get_grp(item);

	for (pstate = ec_comp_group_get_pstate(grp); pstate != NULL;
	     pstate = ec_pnode_get_parent(pstate)) {
		node = ec_pnode_get_node(pstate);
		if (node_help == NULL)
			node_help = ec_dict_get(ec_node_attrs(node), EC_INTERACT_HELP_ATTR);
		if (node_desc == NULL)
			node_desc = ec_dict_get(ec_node_attrs(node), EC_INTERACT_DESC_ATTR);
		if (node_desc == NULL) {
			node_desc = ec_node_desc(node);
			if (node_desc == NULL)
				goto fail;
			desc_to_free = (char *)node_desc;
		}
		if (node_desc == NULL)
			goto fail;
	}

	if (node_desc == NULL)
		goto fail;
	if (node_help == NULL)
		node_help = "";
	help->desc = strdup(node_desc);
	help->help = strdup(node_help);
	if (help->help == NULL || help->desc == NULL)
		goto fail;

	free(desc_to_free);
	return 0;

fail:
	free(desc_to_free);
	free(help->help);
	free(help->desc);
	help->desc = NULL;
	help->help = NULL;
	return -1;
}

ssize_t ec_interact_get_helps(
	const struct ec_node *node,
	const char *line,
	struct ec_interact_help **helps_out
)
{
	const struct ec_comp_group *grp, *prev_grp = NULL;
	struct ec_comp_item *item;
	struct ec_comp *cmpl = NULL;
	struct ec_pnode *parse = NULL;
	unsigned int count = 0;
	struct ec_interact_help *helps = NULL;

	*helps_out = NULL;

	/* check if the current line matches */
	parse = ec_parse(node, line);
	if (ec_pnode_matches(parse))
		count = 1;
	ec_pnode_free(parse);
	parse = NULL;

	/* complete at current cursor position */
	cmpl = ec_complete(node, line);
	if (cmpl == NULL)
		goto fail;

	helps = calloc(1, sizeof(*helps));
	if (helps == NULL)
		goto fail;
	if (count == 1) {
		helps[0].desc = strdup("<return>");
		if (helps[0].desc == NULL)
			goto fail;
		helps[0].help = strdup("Validate command.");
		if (helps[0].help == NULL)
			goto fail;
	}

	/* let's display one contextual help per node */
	EC_COMP_FOREACH (item, cmpl, EC_COMP_UNKNOWN | EC_COMP_FULL | EC_COMP_PARTIAL) {
		struct ec_interact_help *tmp = NULL;

		/* keep one help per group, skip other items  */
		grp = ec_comp_item_get_grp(item);
		if (grp == prev_grp)
			continue;

		prev_grp = grp;

		tmp = realloc(helps, (count + 1) * sizeof(*helps));
		if (tmp == NULL)
			goto fail;
		helps = tmp;
		if (get_node_help(item, &helps[count]) < 0)
			goto fail;
		count++;
	}

	ec_comp_free(cmpl);
	*helps_out = helps;

	return count;

fail:
	ec_pnode_free(parse);
	ec_comp_free(cmpl);
	if (helps != NULL) {
		while (count--) {
			free(helps[count].desc);
			free(helps[count].help);
		}
		free(helps);
	}

	return -1;
}

ssize_t ec_interact_get_error_helps(
	const struct ec_node *node,
	const char *line,
	struct ec_interact_help **helps_out,
	size_t *char_idx
)
{
	struct ec_strvec *line_vec_partial = NULL;
	const struct ec_strvec *parsed_vec = NULL;
	struct ec_strvec *line_vec = NULL;
	struct ec_pnode *parse = NULL;
	struct ec_comp *comp = NULL;
	const struct ec_dict *attrs;
	struct ec_node *cmdlist;
	char *line_copy = NULL;
	size_t parsed_vec_len;
	int ret = 0;
	size_t len;
	int i;

	if (helps_out == NULL) {
		errno = EINVAL;
		goto out;
	}

	*helps_out = NULL;

	/* one additional char to add a space at the end */
	line_copy = malloc(strlen(line) + 2);
	if (line_copy == NULL)
		goto fail;
	strcpy(line_copy, line);
	line_copy[strlen(line)] = ' ';
	line_copy[strlen(line) + 1] = '\0';

	if (ec_node_get_child(node, 0, &cmdlist) < 0)
		goto fail;

	line_vec = ec_strvec_sh_lex_str(line, EC_STRVEC_STRICT, NULL);
	if (line_vec == NULL && errno == EBADMSG)
		goto fail;
	if (line_vec == NULL)
		goto fail;

	len = ec_strvec_len(line_vec);
	for (i = len; i >= 0; i--) {
		/* build an strvec from the first i tokens + an empty token */
		line_vec_partial = ec_strvec_ndup(line_vec, 0, i);
		if (line_vec_partial == NULL)
			goto fail;
		if (ec_strvec_add(line_vec_partial, "") < 0)
			goto fail;

		/* try to parse and complete this strvec */
		parse = ec_parse_strvec(cmdlist, line_vec_partial);
		if (parse == NULL)
			goto fail;
		comp = ec_complete_strvec(cmdlist, line_vec_partial);
		if (comp == NULL)
			goto fail;

		/* get the length of the parsed vec, if any */
		parsed_vec = ec_pnode_get_strvec(parse);
		if (parsed_vec != NULL)
			parsed_vec_len = ec_strvec_len(parsed_vec);
		else
			parsed_vec_len = 0;

		/* if it matches or if it completes, return the helps */
		if ((ec_pnode_matches(parse) && (int)parsed_vec_len == i)
		    || ec_comp_count(comp, EC_COMP_ALL) > 0) {
			/* get the position of the error and store it in char_idx */
			if (i == (int)len) {
				attrs = ec_strvec_get_attrs(line_vec, i - 1);
				if (attrs == NULL)
					goto fail;
				*char_idx = (uintptr_t)ec_dict_get(attrs, EC_STRVEC_ATTR_END) + 1;
			} else {
				attrs = ec_strvec_get_attrs(line_vec, i);
				if (attrs == NULL)
					goto fail;
				*char_idx = (uintptr_t)ec_dict_get(attrs, EC_STRVEC_ATTR_START);
			}

			/* build the partial line string */
			line_copy[*char_idx] = '\0';

			ret = ec_interact_get_helps(node, line_copy, helps_out);
			goto out;
		}

		ec_pnode_free(parse);
		parse = NULL;
		ec_comp_free(comp);
		comp = NULL;
		ec_strvec_free(line_vec_partial);
		line_vec_partial = NULL;
	}

	ret = 0;

out:
	ec_strvec_free(line_vec_partial);
	ec_strvec_free(line_vec);
	free(line_copy);
	ec_pnode_free(parse);
	ec_comp_free(comp);
	return ret;

fail:
	ret = -1;
	goto out;
}

int ec_interact_print_error_helps(
	FILE *out,
	unsigned int width,
	const char *line,
	const struct ec_interact_help *helps,
	size_t n,
	size_t char_idx
)
{
	fprintf(out, "  %s", line);
	if (line[strlen(line)] != '\n')
		fprintf(out, "\n");
	fprintf(out, "  %*s^\n", (int)char_idx, "");
	fprintf(out, "Expected:\n");
	if (ec_interact_print_helps(out, width, helps, n) < 0)
		return -1;

	return 0;
}

int ec_interact_set_help(struct ec_node *node, const char *help)
{
	char *copy = strdup(help);

	if (copy == NULL)
		return -1;

	return ec_dict_set(ec_node_attrs(node), EC_INTERACT_HELP_ATTR, copy, free);
}

int ec_interact_set_callback(struct ec_node *node, ec_interact_command_cb_t cb)
{
	return ec_dict_set(ec_node_attrs(node), EC_INTERACT_CB_ATTR, cb, NULL);
}

int ec_interact_set_desc(struct ec_node *node, const char *desc)
{
	char *copy = strdup(desc);

	if (copy == NULL)
		return -1;

	return ec_dict_set(ec_node_attrs(node), EC_INTERACT_DESC_ATTR, copy, free);
}

ec_interact_command_cb_t ec_interact_get_callback(struct ec_pnode *parse)
{
	struct ec_pnode *iter;
	ec_interact_command_cb_t cb;

	for (iter = parse; iter != NULL; iter = EC_PNODE_ITER_NEXT(parse, iter, 1)) {
		cb = ec_dict_get(ec_node_attrs(ec_pnode_get_node(iter)), EC_INTERACT_CB_ATTR);
		if (cb != NULL)
			return cb;
	}

	return NULL;
}
