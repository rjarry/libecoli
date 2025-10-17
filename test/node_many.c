/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;

	node = ec_node_many(EC_NO_ID, ec_node_str(EC_NO_ID, "foo"), 0, 0);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	testres |= EC_TEST_CHECK_PARSE(node, 0, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	ec_node_free(node);

	node = ec_node_many(EC_NO_ID, ec_node_str(EC_NO_ID, "foo"), 1, 0);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, -1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1);
	ec_node_free(node);

	node = ec_node_many(EC_NO_ID, ec_node_str(EC_NO_ID, "foo"), 1, 2);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, -1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "foo", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1);
	ec_node_free(node);

	/* test completion */
	node = ec_node_many(EC_NO_ID, ec_node_str(EC_NO_ID, "foo"), 2, 4);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"f", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", "", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", "foo", "", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", "foo", "foo", "", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", "foo", "foo", "foo", "", EC_VA_END,
		EC_VA_END);
	ec_node_free(node);

	return testres;
}
