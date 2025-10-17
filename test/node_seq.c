/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node = NULL;
	int testres = 0;

	node = EC_NODE_SEQ(EC_NO_ID,
		ec_node_str(EC_NO_ID, "foo"),
		ec_node_str(EC_NO_ID, "bar")
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "bar", "toto");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foox", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo", "barx");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "bar", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "", "foo");

	testres |= (ec_node_seq_add(node, ec_node_str(EC_NO_ID, "grr")) < 0);
	testres |= EC_TEST_CHECK_PARSE(node, 3, "foo", "bar", "grr");

	ec_node_free(node);

	/* test completion */
	node = EC_NODE_SEQ(EC_NO_ID,
		ec_node_str(EC_NO_ID, "foo"),
		ec_node_option(EC_NO_ID, ec_node_str(EC_NO_ID, "toto")),
		ec_node_str(EC_NO_ID, "bar")
	);
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
		"bar", "toto", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", "t", EC_VA_END,
		"toto", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", "b", EC_VA_END,
		"bar", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", "bar", EC_VA_END,
		"bar", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"x", EC_VA_END,
		EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foobarx", EC_VA_END,
		EC_VA_END);
	ec_node_free(node);

	return testres;
}
