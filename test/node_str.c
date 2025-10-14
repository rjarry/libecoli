/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdlib.h>
#include <string.h>

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;
	char *desc = NULL;

	node = ec_node_str(EC_NO_ID, "foo");
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	desc = ec_node_desc(node);
	testres |= EC_TEST_CHECK(!strcmp(desc, "foo"),
		"Invalid node description.");
	free(desc);
	desc = NULL;
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foobar");
	testres |= EC_TEST_CHECK_PARSE(node, -1, " foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "");
	ec_node_free(node);

	node = ec_node_str(EC_NO_ID, "Здравствуйте");
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "Здравствуйте");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "Здравствуйте",
		"John!");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "");
	ec_node_free(node);

	/* an empty string node always matches */
	node = ec_node_str(EC_NO_ID, "");
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo");
	ec_node_free(node);

	/* test completion */
	node = ec_node_str(EC_NO_ID, "foo");
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node,
		EC_VA_END,
		EC_VA_END);
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
		"x", EC_VA_END,
		EC_VA_END);
	ec_node_free(node);

	return testres;
}
