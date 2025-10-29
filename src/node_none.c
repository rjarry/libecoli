/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

#include <ecoli/complete.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_none.h>
#include <ecoli/parse.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(node_none);

struct ec_node_none {
};

static int ec_node_none_parse(const struct ec_node *node,
			struct ec_pnode *state,
			const struct ec_strvec *strvec)
{
	(void)node;
	(void)state;
	(void)strvec;

	return EC_PARSE_NOMATCH;
}

static int
ec_node_none_complete(const struct ec_node *node,
			struct ec_comp *comp,
			const struct ec_strvec *strvec)
{
	(void)node;
	(void)comp;
	(void)strvec;

	return 0;
}

static struct ec_node_type ec_node_none_type = {
	.name = "none",
	.parse = ec_node_none_parse,
	.complete = ec_node_none_complete,
	.size = sizeof(struct ec_node_none),
};

EC_NODE_TYPE_REGISTER(ec_node_none_type);
