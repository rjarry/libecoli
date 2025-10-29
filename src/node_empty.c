/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli_log.h>
#include <ecoli_strvec.h>
#include <ecoli_node.h>
#include <ecoli_parse.h>
#include <ecoli_complete.h>
#include <ecoli_node_empty.h>

EC_LOG_TYPE_REGISTER(node_empty);

struct ec_node_empty {
};

static int ec_node_empty_parse(const struct ec_node *node,
			struct ec_pnode *pstate,
			const struct ec_strvec *strvec)
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
