/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <string.h>
#include <stdlib.h>

#include "test.h"

static struct ec_node *
build_counter(struct ec_pnode *parse, void *opaque)
{
	const struct ec_node *node;
	struct ec_pnode *root, *iter;
	unsigned int count = 0;
	char buf[32];

	(void)opaque;
	root = ec_pnode_get_root(parse);
	for (iter = root; iter != NULL;
	     iter = EC_PNODE_ITER_NEXT(root, iter, 1)) {
		node = ec_pnode_get_node(iter);
		if (!strcmp(ec_node_id(node), "my-id"))
			count++;
	}
	snprintf(buf, sizeof(buf), "count-%u", count);

	return ec_node_str("my-id", buf);
}

EC_TEST_MAIN() {
	struct ec_node *node;
	int testres = 0;

	node = ec_node_many(EC_NO_ID,
			ec_node_dynamic(EC_NO_ID, build_counter, NULL),
			1, 3);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "count-0");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "count-0", "count-1", "count-2");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "count-0", "count-0");

	/* test completion */

	testres |= EC_TEST_CHECK_COMPLETE(node,
		"c", EC_VA_END,
		"count-0", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"count-0", "", EC_VA_END,
		"count-1", EC_VA_END,
		"count-1");
	ec_node_free(node);

	return testres;
}
