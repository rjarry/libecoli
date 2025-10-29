/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node;
	int ret, testres = 0;

	node = ec_node_re_lex(
		EC_NO_ID,
		ec_node_many(
			EC_NO_ID,
			EC_NODE_OR(
				EC_NO_ID,
				ec_node_str(EC_NO_ID, "foo"),
				ec_node_str(EC_NO_ID, "bar"),
				ec_node_int(EC_NO_ID, 0, 1000, 0)
			),
			0,
			0
		)
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}

	ret = ec_node_re_lex_add(node, "[a-zA-Z]+", 1, NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "[0-9]+", 1, NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "=", 1, NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "-", 1, NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "\\+", 1, NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "[ \t]+", 0, NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	if (ret != 0) {
		EC_LOG(EC_LOG_ERR, "cannot add regexp to node\n");
		ec_node_free(node);
		return -1;
	}

	testres |= EC_TEST_CHECK_PARSE(node, 1, "  foo bar  324 bar234");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo bar324");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foobar");

	/* no completion */
	testres |= EC_TEST_CHECK_COMPLETE(node, "", EC_VA_END, EC_VA_END);

	ec_node_free(node);

	return testres;
}
