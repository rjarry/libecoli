/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;

	node = ec_node("space", EC_NO_ID);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, " ");
	testres |= EC_TEST_CHECK_PARSE(node, 1, " ", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "");
	testres |= EC_TEST_CHECK_PARSE(node, -1, " foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo ");
	ec_node_free(node);

	/* test completion */
	node = ec_node("space", EC_NO_ID);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	/* never completes whatever the input */
	testres |= EC_TEST_CHECK_COMPLETE(node, "", EC_VA_END, EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, " ", EC_VA_END, EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo", EC_VA_END, EC_VA_END);
	ec_node_free(node);

	return testres;
}
