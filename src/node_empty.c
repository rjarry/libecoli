/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <ecoli/complete.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_empty.h>
#include <ecoli/parse.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(node_empty);

struct ec_node_empty { };

static int ec_node_empty_parse(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
)
{
	(void)node;
	(void)pstate;
	(void)strvec;
	return 0;
}

static struct ec_node_type ec_node_empty_type = {
	.name = "empty",
	.parse = ec_node_empty_parse,
	.size = sizeof(struct ec_node_empty),
};

EC_NODE_TYPE_REGISTER(ec_node_empty_type);
