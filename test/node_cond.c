/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;

	node = ec_node_cond(
		EC_NO_ID,
		"cmp(le, count(find(root(), id_node)), 3)",
		ec_node_many(EC_NO_ID, ec_node_str("id_node", "foo"), 0, 0)
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "foo", "foo", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo", "foo", "foo", "foo");
	ec_node_free(node);

	// XXX test completion

	return testres;
}
