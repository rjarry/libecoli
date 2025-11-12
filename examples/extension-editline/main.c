/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli.h>

#define ID_BOOL_TUPLE "id_bool_tuple"

/* stop program when true */
static bool done;

static int convert_cb(const struct ec_pnode *parse)
{
	const char *bool_tuple;
	const char *next_false;
	const char *next_true;
	unsigned int val = 0;
	const char *t;

	bool_tuple = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_BOOL_TUPLE)), 0);
	t = bool_tuple;

	/* convert bool tuple into an integer */
	while (1) {
		next_true = strstr(t, "true");
		next_false = strstr(t, "false");

		if (next_true && (next_false == NULL || next_false > next_true)) {
			val = (val << 1) | 1;
			t = next_true + 1;
		} else if (next_false && (next_true == NULL || next_true > next_false)) {
			val = val << 1;
			t = next_false + 1;
		} else {
			break;
		}
	}
	printf("Integer value for %s is %u\n", bool_tuple, val);

	return 0;
}

static int exit_cb(const struct ec_pnode *parse)
{
	(void)parse;

	printf("Exit !\n");
	done = true;

	return 0;
}

static int check_exit(void *opaque)
{
	(void)opaque;
	return done;
}

static struct ec_node *create_commands(void)
{
	struct ec_node *cmdlist = NULL, *cmd = NULL;
	int ret;

	/* the top node containing the list of commands */
	cmdlist = ec_node("or", EC_NO_ID);
	if (cmdlist == NULL)
		goto fail;

	/* the convert command */
	cmd = EC_NODE_SEQ(
		EC_NO_ID, ec_node_str(EC_NO_ID, "convert"), ec_node("bool_tuple", ID_BOOL_TUPLE)
	);
	if (cmd == NULL)
		goto fail;
	if (ec_interact_set_callback(cmd, convert_cb) < 0)
		goto fail;
	if (ec_interact_set_help(cmd, "Convert a tuple of boolean into its integer representation")
	    < 0)
		goto fail;
	if (ec_interact_set_help(
		    ec_node_find(cmd, ID_BOOL_TUPLE),
		    "A tuple of booleans. Example: \"(true,false,true)\""
	    )
	    < 0)
		goto fail;
	ret = ec_node_or_add(cmdlist, cmd);
	cmd = NULL; /* already freed, even on error */
	if (ret < 0)
		goto fail;

	/* the exit command */
	cmd = ec_node_str(EC_NO_ID, "exit");
	if (ec_interact_set_callback(cmd, exit_cb) < 0)
		goto fail;
	if (ec_interact_set_help(cmd, "exit program") < 0)
		goto fail;
	ret = ec_node_or_add(cmdlist, cmd);
	cmd = NULL; /* already freed, even on error */
	if (ret < 0)
		goto fail;

	/* the lexer, added above the command list */
	cmdlist = ec_node_sh_lex(EC_NO_ID, cmdlist);
	if (cmdlist == NULL)
		goto fail;

	return cmdlist;

fail:
	fprintf(stderr, "cannot initialize nodes\n");
	ec_node_free(cmdlist);
	ec_node_free(cmd);
	return NULL;
}

int main(void)
{
	struct ec_editline *editline = NULL;
	struct ec_node *node = NULL;

	if (ec_init() < 0) {
		fprintf(stderr, "cannot init ecoli: %s\n", strerror(errno));
		return 1;
	}

	node = create_commands();
	if (node == NULL) {
		fprintf(stderr, "failed to create commands: %s\n", strerror(errno));
		goto fail;
	}

	editline = ec_editline("extension-editline", stdin, stdout, stderr, 0);
	if (editline == NULL) {
		fprintf(stderr, "Failed to initialize editline\n");
		goto fail;
	}

	if (ec_editline_set_prompt(editline, "extension> ") < 0) {
		fprintf(stderr, "Failed to set prompt\n");
		goto fail;
	}
	ec_editline_set_node(editline, node);

	if (ec_editline_interact(editline, check_exit, NULL) < 0)
		goto fail;

	ec_editline_free(editline);
	ec_node_free(node);
	return 0;

fail:
	ec_editline_free(editline);
	ec_node_free(node);
	return 1;
}
