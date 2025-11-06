/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli.h>

#include "ip_pool.h"

#define POOL_REGEXP "[A-Za-z][-_a-zA-Z0-9]+"
#define IP_REGEXP                                                                                  \
	"((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\\.){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-" \
	"9]|[1-9][0-9]|[0-9])"
#define ID_POOL_NAME "id_pool_name"
#define ID_ADDR "id_addr"

/* stop program when true */
static bool done;

static struct ec_node *with_help(struct ec_node *node, const char *help)
{
	if (node == NULL)
		return NULL;
	if (ec_editline_set_help(node, help) < 0) {
		ec_node_free(node);
		return NULL;
	}
	return node;
}

static struct ec_node *with_cb(struct ec_node *node, ec_editline_command_cb_t cb)
{
	if (node == NULL)
		return NULL;
	if (ec_editline_set_callback(node, cb) < 0) {
		ec_node_free(node);
		return NULL;
	}
	return node;
}

static struct ec_node *with_desc(struct ec_node *node, const char *desc)
{
	if (node == NULL)
		return NULL;
	if (ec_editline_set_desc(node, desc) < 0) {
		ec_node_free(node);
		return NULL;
	}
	return node;
}

static int pool_list_cb(const struct ec_pnode *parse)
{
	struct ec_strvec *names = NULL;
	size_t len;
	size_t i;

	(void)parse;

	names = ip_pool_list();
	if (names == NULL) {
		fprintf(stderr, "Failed to list pools\n");
		return -1;
	}

	len = ec_strvec_len(names);
	if (len == 0) {
		printf("No pool\n");
	} else {
		for (i = 0; i < len; i++)
			printf("%s\n", ec_strvec_val(names, i));
	}

	ec_strvec_free(names);

	return 0;
}

static int pool_add_cb(const struct ec_pnode *parse)
{
	const char *pool_name;

	pool_name = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_POOL_NAME)), 0);
	if (ip_pool(pool_name) == NULL) {
		fprintf(stderr, "Failed to add pool\n");
		return -1;
	}

	return 0;
}

static int pool_del_cb(const struct ec_pnode *parse)
{
	const char *pool_name;

	pool_name = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_POOL_NAME)), 0);
	ip_pool_free(pool_name);

	return 0;
}

static int addr_list_cb(const struct ec_pnode *parse)
{
	struct ec_strvec *addrs = NULL;
	const struct ip_pool *pool;
	const char *pool_name;
	size_t len;
	size_t i;

	pool_name = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_POOL_NAME)), 0);
	pool = ip_pool_lookup(pool_name);
	addrs = ip_pool_addr_list(pool);
	if (addrs == NULL) {
		fprintf(stderr, "Failed to list pool addresses\n");
		return -1;
	}

	len = ec_strvec_len(addrs);
	if (len == 0) {
		printf("No address\n");
	} else {
		for (i = 0; i < len; i++)
			printf("%s\n", ec_strvec_val(addrs, i));
	}

	ec_strvec_free(addrs);

	return 0;
}

static int addr_add_cb(const struct ec_pnode *parse)
{
	const char *pool_name;
	struct ip_pool *pool;
	const char *addr;

	pool_name = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_POOL_NAME)), 0);
	pool = ip_pool_lookup(pool_name);
	addr = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_ADDR)), 0);
	if (ip_pool_addr_add(pool, addr) < 0) {
		fprintf(stderr, "Failed to add address to pool\n");
		return -1;
	}

	return 0;
}

static int addr_del_cb(const struct ec_pnode *parse)
{
	const char *pool_name;
	struct ip_pool *pool;
	const char *addr;

	pool_name = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_POOL_NAME)), 0);
	pool = ip_pool_lookup(pool_name);
	addr = ec_strvec_val(ec_pnode_get_strvec(ec_pnode_find(parse, ID_ADDR)), 0);
	if (ip_pool_addr_del(pool, addr) < 0) {
		fprintf(stderr, "Failed to delete address from pool\n");
		return -1;
	}

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

static struct ec_strvec *list_pools(struct ec_pnode *pstate, void *opaque)
{
	(void)pstate;
	(void)opaque;

	return ip_pool_list();
}

static struct ec_strvec *list_addrs(struct ec_pnode *pstate, void *opaque)
{
	const char *pool_name;
	struct ip_pool *pool;

	(void)opaque;

	pool_name = ec_strvec_val(
		ec_pnode_get_strvec(ec_pnode_find(ec_pnode_get_root(pstate), ID_POOL_NAME)), 0
	);
	if (pool_name == NULL)
		return ec_strvec();
	pool = ip_pool_lookup(pool_name);
	if (pool == NULL)
		return ec_strvec();

	return ip_pool_addr_list(pool);
}

static struct ec_node *create_pool_commands(void)
{
	struct ec_node *cmdlist = NULL;

	/* the list of pool subcommands */
	cmdlist = EC_NODE_OR(
		EC_NO_ID,
		with_cb(with_help(ec_node_str(EC_NO_ID, "list"), "Display the list of IP pools"),
			pool_list_cb),
		with_cb(EC_NODE_SEQ(
				EC_NO_ID,
				with_help(ec_node_str(EC_NO_ID, "add"), "Create an IP pool"),
				with_help(
					with_desc(
						ec_node_dynlist(
							ID_POOL_NAME,
							list_pools,
							NULL,
							POOL_REGEXP,
							DYNLIST_MATCH_REGEXP | DYNLIST_EXCLUDE_LIST
						),
						"<pool-name>"
					),
					"The name of the pool to create"
				)
			),
			pool_add_cb),
		with_cb(EC_NODE_SEQ(
				EC_NO_ID,
				with_help(ec_node_str(EC_NO_ID, "del"), "Delete an IP pool"),
				with_help(
					with_desc(
						ec_node_dynlist(
							ID_POOL_NAME,
							list_pools,
							NULL,
							POOL_REGEXP,
							DYNLIST_MATCH_LIST
						),
						"<pool-name>"
					),
					"The name of the pool to delete"
				)
			),
			pool_del_cb)
	);

	/* the pool command */
	return EC_NODE_SEQ(
		EC_NO_ID,
		with_help(ec_node_str(EC_NO_ID, "pool"), "Add, delete, or list pools"),
		cmdlist
	);
}

static struct ec_node *create_addr_commands(void)
{
	struct ec_node *cmdlist = NULL;

	/* the list of addr subcommands */
	cmdlist = EC_NODE_OR(
		EC_NO_ID,
		with_cb(with_help(
				EC_NODE_SEQ(EC_NO_ID, ec_node_str(EC_NO_ID, "list")),
				"Display the list of IP addresses in a pool"
			),
			addr_list_cb),
		EC_NODE_SEQ(
			EC_NO_ID,
			with_cb(with_help(
					ec_node_str(EC_NO_ID, "add"),
					"Add an IP address into a pool"
				),
				addr_add_cb),

			with_help(
				with_desc(
					ec_node_dynlist(
						ID_ADDR,
						list_addrs,
						NULL,
						IP_REGEXP,
						DYNLIST_MATCH_REGEXP | DYNLIST_EXCLUDE_LIST
					),
					"<a.b.c.d>"
				),
				"The IP to add"
			)
		),
		EC_NODE_SEQ(
			EC_NO_ID,
			with_cb(with_help(
					ec_node_str(EC_NO_ID, "del"),
					"Delete an IP address from a pool"
				),
				addr_del_cb),

			with_help(
				with_desc(
					ec_node_dynlist(
						ID_ADDR,
						list_addrs,
						NULL,
						IP_REGEXP,
						DYNLIST_MATCH_LIST
					),
					"<a.b.c.d>"
				),
				"The existing IP to delete"
			)
		)

	);

	return EC_NODE_SEQ(
		EC_NO_ID,
		with_help(ec_node_str(EC_NO_ID, "addr"), "Add, delete, list addresses in pool"),
		with_help(ec_node_str(EC_NO_ID, "pool"), "Specify the pool for this operation"),
		with_help(
			with_desc(
				ec_node_dynlist(
					ID_POOL_NAME,
					list_pools,
					NULL,
					POOL_REGEXP,
					DYNLIST_MATCH_LIST
				),
				"<pool-name>"
			),
			"The name of the pool (must exist)"
		),
		cmdlist
	);
}

static struct ec_node *create_commands(void)
{
	struct ec_node *cmdlist = NULL;
	struct ec_node *cmd = NULL;

	/* the top node containing the list of commands */
	cmdlist = ec_node("or", EC_NO_ID);
	if (cmdlist == NULL)
		goto fail;

	/* the exit command */
	cmd = ec_node_str(EC_NO_ID, "exit");
	if (ec_editline_set_callback(cmd, exit_cb) < 0)
		goto fail;
	if (ec_editline_set_help(cmd, "Exit program") < 0)
		goto fail;
	if (ec_node_or_add(cmdlist, cmd) < 0)
		goto fail;

	/* the pool commands */
	if (ec_node_or_add(cmdlist, create_pool_commands()) < 0)
		goto fail;

	/* the addr commands */
	if (ec_node_or_add(cmdlist, create_addr_commands()) < 0)
		goto fail;

	/* the lexer, added above the command list */
	cmdlist = ec_node_sh_lex(EC_NO_ID, cmdlist);
	if (cmdlist == NULL)
		goto fail;

	return cmdlist;

fail:
	fprintf(stderr, "cannot initialize nodes\n");
	ec_node_free(cmdlist);
	return NULL;
}

int main(void)
{
	struct ec_editline *editline = NULL;
	struct ec_node *node = NULL;

	if (ip_pool_init() < 0) {
		fprintf(stderr, "cannot init IP pools: %s\n", strerror(errno));
		return 1;
	}

	if (ec_init() < 0) {
		fprintf(stderr, "cannot init ecoli: %s\n", strerror(errno));
		return 1;
	}

	node = create_commands();
	if (node == NULL) {
		fprintf(stderr, "failed to create commands: %s\n", strerror(errno));
		goto fail;
	}

	editline = ec_editline("pool-editline", stdin, stdout, stderr, 0);
	if (editline == NULL) {
		fprintf(stderr, "Failed to initialize editline\n");
		goto fail;
	}

	if (ec_editline_set_prompt(editline, "pool> ") < 0) {
		fprintf(stderr, "Failed to set prompt\n");
		goto fail;
	}
	ec_editline_set_node(editline, node);

	if (ec_editline_interact(editline, check_exit, NULL) < 0)
		goto fail;

	ec_editline_free(editline);
	ec_node_free(node);
	ip_pool_exit();

	return 0;

fail:
	ec_editline_free(editline);
	ec_node_free(node);
	ip_pool_exit();
	return 1;
}
