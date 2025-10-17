/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;

	node = EC_NODE_CMD(EC_NO_ID,
		"command [option] (subset1, subset2, subset3, subset4) x|y z*",
		ec_node_int("x", 0, 10, 10),
		ec_node_int("y", 20, 30, 10)
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 2, "command", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "command", "subset1", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 4, "command", "subset3", "subset2",
				"1");
	testres |= EC_TEST_CHECK_PARSE(node, 5, "command", "subset2", "subset3",
				"subset1", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 6, "command", "subset3", "subset1",
				"subset4", "subset2", "4");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "command", "23");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "command", "option", "23");
	testres |= EC_TEST_CHECK_PARSE(node, 5, "command", "option", "23",
				"z", "z");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "command", "15");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo");
	ec_node_free(node);

	node = EC_NODE_CMD(EC_NO_ID, "good morning [count] bob|bobby|michael",
			ec_node_int("count", 0, 10, 10));
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 4, "good", "morning", "1", "bob");

	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_VA_END,
		"good", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"g", EC_VA_END,
		"good", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"good", "morning", "", EC_VA_END,
		"bob", "bobby", "michael", EC_VA_END);

	ec_node_free(node);

	node = EC_NODE_CMD(EC_NO_ID, "[foo [bar]]");
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 0, "x");
	ec_node_free(node);

	return testres;
}
