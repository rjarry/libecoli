/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

#define ID_EMPTY "id_empty"

EC_TEST_MAIN()
{
	struct ec_strvec *strvec = NULL;
	struct ec_pnode *pnode = NULL;
	struct ec_node *node = NULL;
	int testres = 0;

	node = ec_node_many(EC_NO_ID, ec_node_str(EC_NO_ID, "foo"), 0, 0);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	testres |= EC_TEST_CHECK_PARSE(node, 0, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	ec_node_free(node);

	node = ec_node_many(EC_NO_ID, ec_node_str(EC_NO_ID, "foo"), 1, 0);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK_PARSE(node, -1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1);
	ec_node_free(node);

	node = ec_node_many(EC_NO_ID, ec_node_str(EC_NO_ID, "foo"), 1, 2);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK_PARSE(node, -1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "foo", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, -1);
	ec_node_free(node);

	node = ec_node_many(EC_NO_ID, ec_node_empty(ID_EMPTY), 0, 0);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	testres |= EC_TEST_CHECK_PARSE(node, 0, "foo");
	strvec = ec_strvec();
	if (strvec == NULL) {
		EC_LOG(EC_LOG_ERR, "failed to create strvec\n");
		goto fail;
	}
	pnode = ec_parse_strvec(node, strvec);
	if (pnode == NULL) {
		EC_LOG(EC_LOG_ERR, "failed to parse strvec\n");
		goto fail;
	}
	if (ec_pnode_find(pnode, ID_EMPTY)) {
		EC_LOG(EC_LOG_ERR, "no ID_EMPTY pnode is expected\n");
		goto fail;
	}
	ec_pnode_free(pnode);
	pnode = NULL;
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_node_free(node);

	node = ec_node_many(EC_NO_ID, ec_node_empty(ID_EMPTY), 0, 5);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	testres |= EC_TEST_CHECK_PARSE(node, 0, "foo");
	strvec = ec_strvec();
	if (strvec == NULL) {
		EC_LOG(EC_LOG_ERR, "failed to create strvec\n");
		goto fail;
	}
	pnode = ec_parse_strvec(node, strvec);
	if (pnode == NULL) {
		EC_LOG(EC_LOG_ERR, "failed to parse strvec\n");
		goto fail;
	}
	if (!ec_pnode_find(pnode, ID_EMPTY)) {
		EC_LOG(EC_LOG_ERR, "ID_EMPTY pnodes are expected\n");
		goto fail;
	}
	ec_pnode_free(pnode);
	pnode = NULL;
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_node_free(node);

	/* test completion */
	node = ec_node_many(EC_NO_ID, ec_node_str(EC_NO_ID, "foo"), 2, 4);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node, "", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "f", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo", "", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node, "foo", "foo", "", EC_VA_END, "foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(
		node, "foo", "foo", "foo", "", EC_VA_END, "foo", EC_VA_END
	);
	testres |= EC_TEST_CHECK_COMPLETE(
		node, "foo", "foo", "foo", "foo", "", EC_VA_END, EC_VA_END
	);
	ec_node_free(node);

	return testres;

fail:
	ec_node_free(node);
	ec_pnode_free(pnode);
	ec_strvec_free(strvec);
	return -1;
}
