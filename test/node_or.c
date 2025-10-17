/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;

	node = EC_NODE_OR(EC_NO_ID,
		ec_node_str(EC_NO_ID, "foo"),
		ec_node_str(EC_NO_ID, "bar")
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1, " ");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foox");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "toto");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "");
	ec_node_free(node);

	/* test completion */
	node = EC_NODE_OR(EC_NO_ID,
		ec_node_str(EC_NO_ID, "foo"),
		ec_node_str(EC_NO_ID, "bar"),
		ec_node_str(EC_NO_ID, "bar2"),
		ec_node_str(EC_NO_ID, "toto"),
		ec_node_str(EC_NO_ID, "titi")
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_VA_END,
		"foo", "bar", "bar2", "toto", "titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"f", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"b", EC_VA_END,
		"bar", "bar2", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"bar", EC_VA_END,
		"bar", "bar2", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"t", EC_VA_END,
		"toto", "titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"to", EC_VA_END,
		"toto", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"x", EC_VA_END,
		EC_VA_END);
	ec_node_free(node);

	return testres;
}
