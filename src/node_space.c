/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli/complete.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_space.h>
#include <ecoli/parse.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(node_space);

struct ec_node_space { };

static int ec_node_space_parse(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
)
{
	const char *str;
	size_t len = 0;

	(void)pstate;
	(void)node;

	if (ec_strvec_len(strvec) == 0)
		return EC_PARSE_NOMATCH;

	str = ec_strvec_val(strvec, 0);
	while (isspace(str[len]))
		len++;
	if (len == 0 || len != strlen(str))
		return EC_PARSE_NOMATCH;

	return 1;
}

static struct ec_node_type ec_node_space_type = {
	.name = "space",
	.parse = ec_node_space_parse,
	.size = sizeof(struct ec_node_space),
};

EC_NODE_TYPE_REGISTER(ec_node_space_type);
