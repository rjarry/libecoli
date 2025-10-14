/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli_malloc.h>
#include <ecoli_log.h>
#include <ecoli_test.h>
#include <ecoli_strvec.h>
#include <ecoli_node.h>
#include <ecoli_parse.h>
#include <ecoli_complete.h>
#include <ecoli_node_none.h>

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

/* LCOV_EXCL_START */
static int ec_node_none_testcase(void)
{
	struct ec_node *node;
	int testres = 0;

	node = ec_node("none", EC_NO_ID);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1);
	ec_node_free(node);

	/* never completes */
	node = ec_node("none", EC_NO_ID);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_VA_END,
		EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", EC_VA_END,
		EC_VA_END);
	ec_node_free(node);

	return testres;
}

static struct ec_test ec_node_none_test = {
	.name = "node_none",
	.test = ec_node_none_testcase,
};

EC_TEST_REGISTER(ec_node_none_test);
/* LCOV_EXCL_STOP */
