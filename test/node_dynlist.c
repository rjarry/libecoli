/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdlib.h>
#include <string.h>

#include "test.h"

static struct ec_strvec *get_names(struct ec_pnode *pstate, void *opaque)
{
	(void)pstate;
	(void)opaque;
	return EC_STRVEC("foo", "bar", "baz");
}

EC_TEST_MAIN()
{
	struct ec_node *node;
	int testres = 0;

	node = ec_node_dynlist(EC_NO_ID, get_names, NULL, "[a-z]+", DYNLIST_MATCH_LIST);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "pouet");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "pouet");
	testres |= EC_TEST_CHECK_PARSE(node, -1, " foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "");
	ec_node_free(node);

	node = ec_node_dynlist(EC_NO_ID, get_names, NULL, "[a-z]+", DYNLIST_MATCH_REGEXP);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "pouet");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "pouet");
	testres |= EC_TEST_CHECK_PARSE(node, -1, " foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "");
	ec_node_free(node);

	node = ec_node_dynlist(
		EC_NO_ID, get_names, NULL, "[a-z]+", DYNLIST_MATCH_REGEXP | DYNLIST_EXCLUDE_LIST
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo", "pouet");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "pouet");
	testres |= EC_TEST_CHECK_PARSE(node, -1, " foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "");
	ec_node_free(node);

	/* test completion */
	node = ec_node_dynlist(EC_NO_ID, get_names, NULL, "[a-z]+", DYNLIST_MATCH_LIST);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node, EC_VA_END, EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "", EC_VA_END, "foo", "bar", "baz", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "f", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "b", EC_VA_END, "bar", "baz", EC_VA_END);
	ec_node_free(node);

	return testres;
}
