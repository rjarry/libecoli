/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;

	node = ec_node_many(
		EC_NO_ID,
		EC_NODE_OR(
			EC_NO_ID,
			ec_node_once(EC_NO_ID, ec_node_str(EC_NO_ID, "foo")),
			ec_node_str(EC_NO_ID, "bar")
		),
		0,
		0
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "foo", "bar", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "bar", "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "bar", "foo", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 0, "foox");

	testres |= EC_TEST_CHECK_COMPLETE(node, "", EC_VA_END, "foo", "bar", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "f", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "b", EC_VA_END, "bar", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo", "", EC_VA_END, "bar", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "bar", "", EC_VA_END, "foo", "bar", EC_VA_END);
	ec_node_free(node);

	return testres;
}
