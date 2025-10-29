/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;

	node = ec_node_sh_lex(
		EC_NO_ID,
		EC_NODE_SEQ(
			EC_NO_ID,
			ec_node_str(EC_NO_ID, "foo"),
			ec_node_option(EC_NO_ID, ec_node_str(EC_NO_ID, "toto")),
			ec_node_str(EC_NO_ID, "bar")
		)
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "  foo   bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "  'foo' \"bar\"");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "  'f'oo 'toto' bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "  foo toto bar'");
	ec_node_free(node);

	/* test completion */
	node = ec_node_sh_lex(
		EC_NO_ID,
		EC_NODE_SEQ(
			EC_NO_ID,
			ec_node_str(EC_NO_ID, "foo"),
			ec_node_option(EC_NO_ID, ec_node_str(EC_NO_ID, "toto")),
			ec_node_str(EC_NO_ID, "bar"),
			ec_node_str(EC_NO_ID, "titi")
		)
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node, "", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, " ", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "f", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo ", EC_VA_END, "bar", "toto", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo t", EC_VA_END, "toto", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo b", EC_VA_END, "bar", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo bar", EC_VA_END, "bar", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo bar ", EC_VA_END, "titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo toto bar ", EC_VA_END, "titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "x", EC_VA_END, EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo barx", EC_VA_END, EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo 'b", EC_VA_END, "'bar'", EC_VA_END);

	ec_node_free(node);
	return testres;
}
