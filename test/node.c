/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

static unsigned int test_iter(struct ec_node *node)
{
	struct ec_node_iter *iter_root, *iter;
	unsigned int count = 0;

	iter_root = ec_node_iter(node);
	for (iter = iter_root; iter != NULL; iter = ec_node_iter_next(iter_root, iter, true)) {
		count++;
	}
	ec_node_iter_free(iter_root);

	return count;
}

EC_TEST_MAIN()
{
	struct ec_node *node = NULL, *expr = NULL;
	struct ec_node *expr2 = NULL, *val = NULL, *op = NULL, *seq = NULL;
	const struct ec_node_type *type;
	struct ec_node *child;
	unsigned int count;
	FILE *f = NULL;
	char *buf = NULL;
	char *desc = NULL;
	size_t buflen = 0;
	int testres = 0;
	int ret;

	node = EC_NODE_SEQ(EC_NO_ID, ec_node_str("id_x", "x"), ec_node_str("id_y", "y"));
	if (node == NULL)
		goto fail;

	ec_node_clone(node);
	ec_node_free(node);

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_node_dump(f, node);
	ec_node_type_dump(f);
	ec_node_dump(f, NULL);
	fclose(f);
	f = NULL;

	testres |= EC_TEST_CHECK(strstr(buf, "type=seq id="), "bad dump\n");
	testres |= EC_TEST_CHECK(
		strstr(buf, "type=str id=id_x")
			&& strstr(strstr(buf, "type=str id=id_x") + 1, "type=str id=id_y"),
		"bad dump\n"
	);
	free(buf);
	buf = NULL;

	desc = ec_node_desc(node);
	testres |= EC_TEST_CHECK(
		!strcmp(ec_node_type(node)->name, "seq") && !strcmp(ec_node_id(node), EC_NO_ID)
			&& !strcmp(desc, "<seq>"),
		"bad child 0"
	);
	free(desc);
	desc = NULL;

	testres |= EC_TEST_CHECK(ec_node_get_children_count(node) == 2, "bad children count\n");
	ret = ec_node_get_child(node, 0, &child);
	testres |= EC_TEST_CHECK(
		ret == 0 && child != NULL && !strcmp(ec_node_type(child)->name, "str")
			&& !strcmp(ec_node_id(child), "id_x"),
		"bad child 0"
	);
	ret = ec_node_get_child(node, 1, &child);
	testres |= EC_TEST_CHECK(
		ret == 0 && child != NULL && !strcmp(ec_node_type(child)->name, "str")
			&& !strcmp(ec_node_id(child), "id_y"),
		"bad child 1"
	);
	ret = ec_node_get_child(node, 2, &child);
	testres |= EC_TEST_CHECK(ret != 0, "ret should be != 0");
	testres |= EC_TEST_CHECK(child == NULL, "child 2 should be NULL");

	child = ec_node_find(node, "id_x");
	desc = ec_node_desc(child);
	testres |= EC_TEST_CHECK(
		child != NULL && !strcmp(ec_node_type(child)->name, "str")
			&& !strcmp(ec_node_id(child), "id_x") && !strcmp(desc, "x"),
		"bad child id_x"
	);
	free(desc);
	desc = NULL;

	count = test_iter(node);
	testres |= EC_TEST_CHECK(count == 3, "invalid node count (%u instead if %u)", count, 3);

	child = ec_node_find(node, "id_dezdex");
	testres |= EC_TEST_CHECK(child == NULL, "child with wrong id should be NULL");

	ret = ec_dict_set(ec_node_attrs(node), "key", "val", NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot set node attribute\n");

	type = ec_node_type_lookup("seq");
	testres |= EC_TEST_CHECK(
		type != NULL && ec_node_check_type(node, type) == 0, "cannot get seq node type"
	);
	type = ec_node_type_lookup("str");
	testres |= EC_TEST_CHECK(
		type != NULL && ec_node_check_type(node, type) < 0, "node type should not be str"
	);

	ec_node_free(node);
	node = NULL;

	node = ec_node("deznuindez", EC_NO_ID);
	testres |= EC_TEST_CHECK(node == NULL, "should not be able to create node\n");

	/* test loop */
	expr = ec_node("or", EC_NO_ID);
	val = ec_node_int(EC_NO_ID, 0, 10, 0);
	op = ec_node_str(EC_NO_ID, "!");
	seq = EC_NODE_SEQ(EC_NO_ID, op, ec_node_clone(expr));
	op = NULL;
	if (expr == NULL || val == NULL || seq == NULL)
		goto fail;
	if (ec_node_or_add(expr, ec_node_clone(seq)) < 0)
		goto fail;
	ec_node_free(seq);
	seq = NULL;
	if (ec_node_or_add(expr, ec_node_clone(val)) < 0)
		goto fail;
	ec_node_free(val);
	val = NULL;

	count = test_iter(expr);
	testres |= EC_TEST_CHECK(count == 5, "invalid node count (%u instead if %u)", count, 5);

	child = ec_node_find(expr, "id_dezdex");
	testres |= EC_TEST_CHECK(child == NULL, "child with wrong id should be NULL");

	testres |= EC_TEST_CHECK_PARSE(expr, 1, "1");
	testres |= EC_TEST_CHECK_PARSE(expr, 3, "!", "!", "1");
	testres |= EC_TEST_CHECK_PARSE(expr, -1, "!", "!", "!");

	ec_node_free(expr);
	expr = NULL;

	/* same loop test, but keep some refs (released later) */
	expr = ec_node("or", EC_NO_ID);
	expr2 = ec_node_clone(expr);
	val = ec_node_int(EC_NO_ID, 0, 10, 0);
	op = ec_node_str(EC_NO_ID, "!");
	seq = EC_NODE_SEQ(EC_NO_ID, op, ec_node_clone(expr));
	op = NULL;
	if (expr == NULL || val == NULL || seq == NULL)
		goto fail;
	if (ec_node_or_add(expr, ec_node_clone(seq)) < 0)
		goto fail;
	ec_node_free(seq);
	seq = NULL;
	if (ec_node_or_add(expr, ec_node_clone(val)) < 0)
		goto fail;

	testres |= EC_TEST_CHECK_PARSE(expr, 1, "1");
	testres |= EC_TEST_CHECK_PARSE(expr, 3, "!", "!", "1");
	testres |= EC_TEST_CHECK_PARSE(expr, -1, "!", "!", "!");

	count = test_iter(expr);
	testres |= EC_TEST_CHECK(count == 5, "invalid node count (%u instead if %u)", count, 5);

	ec_node_free(expr2);
	expr2 = NULL;
	ec_node_free(val);
	val = NULL;
	ec_node_free(expr);
	expr = NULL;

	return testres;

fail:
	ec_node_free(expr);
	ec_node_free(expr2);
	ec_node_free(val);
	ec_node_free(seq);
	ec_node_free(node);
	if (f != NULL)
		fclose(f);
	free(buf);

	assert(errno != 0);
	return -1;
}
