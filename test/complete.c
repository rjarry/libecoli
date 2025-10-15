/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_strvec *vec1 = NULL, *vec2 = NULL;
	struct ec_node *node = NULL;
	struct ec_comp *c = NULL;
	struct ec_comp_item *item;
	FILE *f = NULL;
	char *buf = NULL;
	size_t buflen = 0;
	int testres = 0;

	node = ec_node_sh_lex(EC_NO_ID,
			EC_NODE_OR(EC_NO_ID,
				ec_node_str("id_x", "xx"),
				ec_node_str("id_y", "yy")));
	if (node == NULL)
		goto fail;

	c = ec_complete(node, "xcdscds");
	testres |= EC_TEST_CHECK(
		c != NULL && ec_comp_count(c, EC_COMP_ALL) == 0,
		"complete count should is not 0\n");
	ec_comp_free(c);

	c = ec_complete(node, "x");
	testres |= EC_TEST_CHECK(
		c != NULL && ec_comp_count(c, EC_COMP_ALL) == 1,
		"complete count should is not 1\n");
	ec_comp_free(c);

	c = ec_complete(node, "");
	testres |= EC_TEST_CHECK(
		c != NULL && ec_comp_count(c, EC_COMP_ALL) == 2,
		"complete count should is not 2\n");

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_comp_dump(f, NULL);
	fclose(f);
	f = NULL;

	testres |= EC_TEST_CHECK(
		strstr(buf, "no completion"), "bad dump\n");
	free(buf);
	buf = NULL;

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_comp_dump(f, c);
	fclose(f);
	f = NULL;

	testres |= EC_TEST_CHECK(
		strstr(buf, "comp=<xx>"), "bad dump\n");
	testres |= EC_TEST_CHECK(
		strstr(buf, "comp=<yy>"), "bad dump\n");
	free(buf);
	buf = NULL;

	item = ec_comp_iter_first(c, EC_COMP_ALL);
	if (item == NULL)
		goto fail;

	testres |= EC_TEST_CHECK(
		!strcmp(ec_comp_item_get_display(item), "xx"),
		"bad item display\n");
	testres |= EC_TEST_CHECK(
		ec_comp_item_get_type(item) == EC_COMP_FULL,
		"bad item type\n");
	testres |= EC_TEST_CHECK(
		!strcmp(ec_node_id(ec_comp_item_get_node(item)), "id_x"),
		"bad item node\n");

	item = ec_comp_iter_next(item, EC_COMP_ALL);
	if (item == NULL)
		goto fail;

	testres |= EC_TEST_CHECK(
		!strcmp(ec_comp_item_get_display(item), "yy"),
		"bad item display\n");
	testres |= EC_TEST_CHECK(
		ec_comp_item_get_type(item) == EC_COMP_FULL,
		"bad item type\n");
	testres |= EC_TEST_CHECK(
		!strcmp(ec_node_id(ec_comp_item_get_node(item)), "id_y"),
		"bad item node\n");

	item = ec_comp_iter_next(item, EC_COMP_ALL);
	testres |= EC_TEST_CHECK(item == NULL, "should be the last item\n");

	ec_comp_free(c);
	ec_node_free(node);

	node = EC_NODE_SEQ(EC_NO_ID,
		ec_node_str("id_x", "xxx"),
		ec_node_str("id_y", "yyyyyy"),
		ec_node_str("id_z", "zzzzzzzzzzz"));
	testres |= EC_TEST_CHECK(node != NULL, "null node");
	vec1 = EC_STRVEC("x", "y", "z");
	testres |= EC_TEST_CHECK(vec1 != NULL, "null vec");
	vec2 = ec_complete_strvec_expand(node, EC_COMP_ALL, vec1);
	testres |= EC_TEST_CHECK(vec2 != NULL, "expand failed");
	ec_strvec_free(vec1);
	vec1 = EC_STRVEC("xxx", "yyyyyy", "zzzzzzzzzzz");
	testres |= EC_TEST_CHECK(vec2 != NULL, "expand failed");
	testres |= EC_TEST_CHECK(ec_strvec_cmp(vec2, vec1) == 0, "expand invalid");
	ec_strvec_free(vec1);
	ec_strvec_free(vec2);
	ec_node_free(node);

	return testres;

fail:
	ec_strvec_free(vec1);
	ec_strvec_free(vec2);
	ec_comp_free(c);
	ec_node_free(node);
	if (f != NULL)
		fclose(f);
	free(buf);

	return -1;
}
