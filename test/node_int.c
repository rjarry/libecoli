/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_pnode *p;
	struct ec_node *node;
	const char *s;
	int testres = 0;
	uint64_t u64;
	int64_t i64;

	node = ec_node_uint(EC_NO_ID, 1, 256, 0);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, -1, "");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "0");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "1");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "256", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "0x100");
	testres |= EC_TEST_CHECK_PARSE(node, 1, " 1");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "-1");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "0x101");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "zzz");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "0x100000000000000000");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "4r");

	p = ec_parse(node, "1");
	s = ec_strvec_val(ec_pnode_get_strvec(p), 0);
	testres |= EC_TEST_CHECK(s != NULL &&
		ec_node_uint_getval(node, s, &u64) == 0 &&
		u64 == 1, "bad integer value");
	ec_pnode_free(p);

	p = ec_parse(node, "10");
	s = ec_strvec_val(ec_pnode_get_strvec(p), 0);
	testres |= EC_TEST_CHECK(s != NULL &&
		ec_node_uint_getval(node, s, &u64) == 0 &&
		u64 == 10, "bad integer value");
	ec_pnode_free(p);
	ec_node_free(node);

	node = ec_node_int(EC_NO_ID, -1, LLONG_MAX, 16);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "0");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "-1");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "7fffffffffffffff");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "0x7fffffffffffffff");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "0x8000000000000000");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "-2");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "zzz");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "4r");

	p = ec_parse(node, "10");
	s = ec_strvec_val(ec_pnode_get_strvec(p), 0);
	testres |= EC_TEST_CHECK(s != NULL &&
		ec_node_int_getval(node, s, &i64) == 0 &&
		i64 == 16, "bad integer value");
	ec_pnode_free(p);
	ec_node_free(node);

	node = ec_node_int(EC_NO_ID, LLONG_MIN, 0, 10);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "0");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "-1");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "-9223372036854775808");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "0x0");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "1");
	ec_node_free(node);

	/* test completion */
	node = ec_node_int(EC_NO_ID, 0, 10, 0);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_VA_END,
		EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"x", EC_VA_END,
		EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"1", EC_VA_END,
		EC_VA_END);
	ec_node_free(node);

	return testres;
}
