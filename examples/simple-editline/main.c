/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli.h>

#define ID_NAME "id_name"
#define ID_JOHN "id_john"
#define ID_COUNT "id_count"

/* stop program when true */
static bool done;

static int hello_cb(const struct ec_pnode *parse)
{
	const char *count;
	const char *name;

	name = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_NAME)), 0);
	count = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_COUNT)), 0);

	printf("you say hello to %s", name);
	if (count)
		printf(" %s times", count);
	printf("\n");

	return 0;
}

static int bye_cb(const struct ec_pnode *parse)
{
	const char *name;

	name = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_NAME)), 0);
	printf("you say bye to %s\n", name);

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
	struct ec_node *names = NULL;

	/* the top node containing the list of commands */
	cmdlist = ec_node("or", EC_NO_ID);
	if (cmdlist == NULL)
		goto fail;

	/* a common subtree containing a list of names */
	names = EC_NODE_OR(
		ID_NAME,
		ec_node_str(ID_JOHN, "john"),
		ec_node_str(EC_NO_ID, "johnny"),
		ec_node_str(EC_NO_ID, "mike")
	);
	if (names == NULL)
		goto fail;

	/* the hello command */
	cmd = EC_NODE_SEQ(
		EC_NO_ID,
		ec_node_str(EC_NO_ID, "hello"),
		ec_node_clone(names),
		ec_node_option(EC_NO_ID, ec_node_int(ID_COUNT, 0, 10, 10))
	);
	if (cmd == NULL)
		goto fail;
	if (ec_editline_set_callback(cmd, hello_cb) < 0)
		goto fail;
	if (ec_editline_set_help(cmd, "say hello to someone several times") < 0)
		goto fail;
	if (ec_editline_set_help(ec_node_find(cmd, ID_JOHN), "specific help for john") < 0)
		goto fail;
	if (ec_editline_set_help(ec_node_find(cmd, ID_NAME), "the name of the person") < 0)
		goto fail;
	if (ec_editline_set_help(ec_node_find(cmd, ID_COUNT), "an integer (0-10)") < 0)
		goto fail;
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	/* the bye command */
	cmd = EC_NODE_SEQ(EC_NO_ID, ec_node_str(EC_NO_ID, "bye"), ec_node_clone(names));
	if (ec_editline_set_callback(cmd, bye_cb) < 0)
		goto fail;
	if (ec_editline_set_help(cmd, "say bye") < 0)
		goto fail;
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	/* the exit command */
	cmd = ec_node_str(EC_NO_ID, "exit");
	if (ec_editline_set_callback(cmd, exit_cb) < 0)
		goto fail;
	if (ec_editline_set_help(cmd, "exit program") < 0)
		goto fail;
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	/* the lexer, added above the command list */
	cmdlist = ec_node_sh_lex(EC_NO_ID, cmdlist);
	if (cmdlist == NULL)
		goto fail;

	ec_node_free(names);

	return cmdlist;

fail:
	fprintf(stderr, "cannot initialize nodes\n");
	ec_node_free(names);
	ec_node_free(cmdlist);
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

	editline = ec_editline("simple-editline", stdin, stdout, stderr, 0);
	if (editline == NULL) {
		fprintf(stderr, "Failed to initialize editline\n");
		goto fail;
	}

	if (ec_editline_set_prompt(editline, "simple> ") < 0) {
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
