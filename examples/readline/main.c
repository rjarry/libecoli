/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

#include <ecoli.h>

static struct ec_node *commands;

static char *my_completion_entry(const char *s, int state)
{
	static struct ec_comp *c;
	static struct ec_comp_item *item;
	enum ec_comp_type item_type;
	const char *item_str, *item_display;

	(void)s;

	/* Don't append a quote. Note: there are still some bugs when
	 * completing a quoted token. */
	rl_completion_suppress_quote = 1;
	rl_completer_quote_characters = "\"'";

	if (state == 0) {
		char *line;

		ec_comp_free(c);
		line = strdup(rl_line_buffer);
		if (line == NULL)
			return NULL;
		line[rl_point] = '\0';

		c = ec_complete(commands, line);
		free(line);
		if (c == NULL)
			return NULL;

		free(item);
		item = ec_comp_iter_first(c, EC_COMP_FULL | EC_COMP_PARTIAL);
		if (item == NULL)
			return NULL;
	} else {
		item = ec_comp_iter_next(item, EC_COMP_FULL | EC_COMP_PARTIAL);
		if (item == NULL)
			return NULL;
	}

	item_str = ec_comp_item_get_str(item);
	if (ec_comp_count(c, EC_COMP_FULL | EC_COMP_PARTIAL) == 1) {
		/* don't add the trailing space for partial completions */
		if (state == 0) {
			item_type = ec_comp_item_get_type(item);
			if (item_type == EC_COMP_FULL)
				rl_completion_suppress_append = 0;
			else
				rl_completion_suppress_append = 1;
		}

		return strdup(item_str);
	} else if (rl_completion_type == '?') {
		/* on second try only show the display part */
		item_display = ec_comp_item_get_display(item);
		return strdup(item_display);
	}

	return strdup(item_str);
}

static char **my_attempted_completion(const char *text, int start, int end)
{
	(void)start;
	(void)end;

	/* remove default file completion */
	rl_attempted_completion_over = 1;

	return rl_completion_matches(text, my_completion_entry);
}

/* this function builds the help string */
static char *get_node_help(const struct ec_comp_item *item)
{
	const struct ec_comp_group *grp;
	const struct ec_pnode *pstate;
	const struct ec_node *node;
	char *help = NULL;
	const char *node_help = NULL;
	char *node_desc = NULL;

	grp = ec_comp_item_get_grp(item);
	for (pstate = ec_comp_group_get_pstate(grp); pstate != NULL;
	     pstate = ec_pnode_get_parent(pstate)) {
		node = ec_pnode_get_node(pstate);
		if (node_help == NULL)
			node_help = ec_dict_get(ec_node_attrs(node), "help");
		if (node_desc == NULL)
			node_desc = ec_node_desc(node);
	}

	if (node_help == NULL)
		node_help = "-";
	if (node_desc == NULL)
		return NULL;

	if (asprintf(&help, "%-20s %s", node_desc, node_help) < 0)
		return NULL;

	free(node_desc);

	return help;
}

static int show_help(int ignore, int invoking_key)
{
	const struct ec_comp_group *grp, *prev_grp = NULL;
	struct ec_comp_item *item = NULL;
	struct ec_comp *c = NULL;
	struct ec_pnode *p = NULL;
	char *line = NULL;
	size_t count = 0;
	char **helps = NULL;
	int match = 0;
	int ret = 1;
	int cols;

	(void)ignore;
	(void)invoking_key;

	line = strdup(rl_line_buffer);
	if (line == NULL)
		goto fail;

	/* check if the current line matches */
	p = ec_parse(commands, line);
	if (ec_pnode_matches(p))
		match = 1;
	ec_pnode_free(p);
	p = NULL;

	/* complete at current cursor position */
	line[rl_point] = '\0';
	c = ec_complete(commands, line);
	free(line);
	line = NULL;
	if (c == NULL)
		goto fail;

	/* strangely, rl_display_match_list() expects first index at 1 */
	helps = calloc(match + 1, sizeof(char *));
	if (helps == NULL)
		goto fail;
	if (match)
		helps[1] = "<return>";

	/* let's display one contextual help per node */
	EC_COMP_FOREACH (item, c, EC_COMP_UNKNOWN | EC_COMP_FULL | EC_COMP_PARTIAL) {
		char **tmp;

		/* keep one help per group, skip other items  */
		grp = ec_comp_item_get_grp(item);
		if (grp == prev_grp)
			continue;

		prev_grp = grp;

		tmp = realloc(helps, (count + match + 2) * sizeof(char *));
		if (tmp == NULL)
			goto fail;
		helps = tmp;
		helps[count + match + 1] = get_node_help(item);
		count++;
	}

	free(item);
	ec_comp_free(c);
	/* ensure not more than 1 entry per line */
	rl_get_screen_size(NULL, &cols);
	rl_display_match_list(helps, count + match, cols);
	rl_forced_update_display();

	ret = 0;

fail:
	free(item);
	ec_pnode_free(p);
	free(line);
	ec_comp_free(c);
	if (helps != NULL) {
		while (count--)
			free(helps[count + match + 1]);
	}
	free(helps);

	return ret;
}

static int create_commands(void)
{
	struct ec_node *cmdlist = NULL, *cmd = NULL;

	cmdlist = ec_node("or", EC_NO_ID);
	if (cmdlist == NULL)
		goto fail;

	cmd = EC_NODE_SEQ(
		EC_NO_ID,
		ec_node_str(EC_NO_ID, "hello"),
		EC_NODE_OR(
			"name",
			ec_node_str("john", "john"),
			ec_node_str(EC_NO_ID, "johnny"),
			ec_node_str(EC_NO_ID, "mike")
		),
		ec_node_option(EC_NO_ID, ec_node_int("int", 0, 10, 10))
	);
	if (cmd == NULL)
		goto fail;
	ec_dict_set(ec_node_attrs(cmd), "help", "say hello to someone several times", NULL);
	ec_dict_set(
		ec_node_attrs(ec_node_find(cmd, "john")), "help", "specific help for john", NULL
	);
	ec_dict_set(
		ec_node_attrs(ec_node_find(cmd, "name")), "help", "the name of the person", NULL
	);
	ec_dict_set(ec_node_attrs(ec_node_find(cmd, "int")), "help", "an integer (0-10)", NULL);
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	cmd = EC_NODE_CMD(
		EC_NO_ID,
		"good morning name [count]",
		EC_NODE_CMD("name", "bob|bobby|michael"),
		ec_node_int("count", 0, 10, 10)
	);
	if (cmd == NULL)
		goto fail;
	ec_dict_set(ec_node_attrs(cmd), "help", "say good morning to someone several times", NULL);
	ec_dict_set(ec_node_attrs(ec_node_find(cmd, "name")), "help", "the person to greet", NULL);
	ec_dict_set(
		ec_node_attrs(ec_node_find(cmd, "count")),
		"help",
		"how many times to greet (0-10)",
		NULL
	);
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	cmd = EC_NODE_CMD(EC_NO_ID, "buy potatoes,carrots,pumpkins");
	if (cmd == NULL)
		goto fail;
	ec_dict_set(ec_node_attrs(cmd), "help", "buy some vegetables", NULL);
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	cmd = EC_NODE_CMD(
		EC_NO_ID,
		"eat vegetables",
		ec_node_many(
			"vegetables",
			EC_NODE_OR(
				EC_NO_ID,
				ec_node_str(EC_NO_ID, "potatoes"),
				ec_node_once(EC_NO_ID, ec_node_str(EC_NO_ID, "carrots")),
				ec_node_once(EC_NO_ID, ec_node_str(EC_NO_ID, "pumpkins"))
			),
			1,
			0
		)
	);
	if (cmd == NULL)
		goto fail;
	ec_dict_set(ec_node_attrs(cmd), "help", "eat vegetables (take some more potatoes)", NULL);
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	cmd = EC_NODE_SEQ(EC_NO_ID, ec_node_str(EC_NO_ID, "bye"));
	ec_dict_set(ec_node_attrs(cmd), "help", "say bye", NULL);
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	cmd = EC_NODE_SEQ(EC_NO_ID, ec_node_str(EC_NO_ID, "load"), ec_node("file", EC_NO_ID));
	ec_dict_set(ec_node_attrs(cmd), "help", "load a file", NULL);
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	commands = ec_node_sh_lex(EC_NO_ID, cmdlist);
	if (commands == NULL)
		goto fail;

	return 0;

fail:
	fprintf(stderr, "cannot initialize nodes\n");
	ec_node_free(cmdlist);
	return -1;
}

int main(void)
{
	struct ec_pnode *p;
	char *line;

	if (ec_init() < 0) {
		fprintf(stderr, "cannot init ecoli: %s\n", strerror(errno));
		return 1;
	}

	if (create_commands() < 0)
		return 1;

	rl_bind_key('?', show_help);
	rl_attempted_completion_function = my_attempted_completion;

	while (1) {
		line = readline("> ");
		if (line == NULL)
			break;

		p = ec_parse(commands, line);
		ec_pnode_dump(stdout, p);
		add_history(line);
		ec_pnode_free(p);
	}

	ec_node_free(commands);
	return 0;
}
