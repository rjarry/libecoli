/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_node *node = NULL;
	struct ec_pnode *p = NULL, *p2 = NULL;
	const struct ec_pnode *pc;
	FILE *f = NULL;
	char *buf = NULL;
	size_t buflen = 0;
	int testres = 0;
	int ret;

	node = ec_node_sh_lex(EC_NO_ID,
			EC_NODE_SEQ(EC_NO_ID,
				ec_node_str("id_x", "x"),
				ec_node_str("id_y", "y")));
	if (node == NULL)
		goto fail;

	p = ec_parse(node, "xcdscds");
	testres |= EC_TEST_CHECK(
		p != NULL && !ec_pnode_matches(p),
		"parse should not match\n");

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_pnode_dump(f, p);
	fclose(f);
	f = NULL;

	testres |= EC_TEST_CHECK(
		strstr(buf, "no match"), "bad dump\n");
	free(buf);
	buf = NULL;
	ec_pnode_free(p);

	p = ec_parse(node, "x y");
	testres |= EC_TEST_CHECK(
		p != NULL && ec_pnode_matches(p),
		"parse should match\n");
	testres |= EC_TEST_CHECK(
		ec_pnode_len(p) == 1, "bad parse len\n");

	ret = ec_dict_set(ec_pnode_get_attrs(p), "key", "val", NULL);
	testres |= EC_TEST_CHECK(ret == 0,
		"cannot set parse attribute\n");

	p2 = ec_pnode_dup(p);
	testres |= EC_TEST_CHECK(
		p2 != NULL && ec_pnode_matches(p2),
		"parse should match\n");
	ec_pnode_free(p2);
	p2 = NULL;

	pc = ec_pnode_find(p, "id_x");
	testres |= EC_TEST_CHECK(pc != NULL, "cannot find id_x");
	testres |= EC_TEST_CHECK(pc != NULL &&
		ec_pnode_get_parent(pc) != NULL &&
		ec_pnode_get_parent(ec_pnode_get_parent(pc)) == p,
		"invalid parent\n");

	pc = ec_pnode_find(p, "id_y");
	testres |= EC_TEST_CHECK(pc != NULL, "cannot find id_y");
	pc = ec_pnode_find(p, "id_dezdezdez");
	testres |= EC_TEST_CHECK(pc == NULL, "should not find bad id");


	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_pnode_dump(f, p);
	fclose(f);
	f = NULL;

	testres |= EC_TEST_CHECK(
		strstr(buf, "type=sh_lex id=") &&
		strstr(buf, "type=seq id=") &&
		strstr(buf, "type=str id=id_x") &&
		strstr(buf, "type=str id=id_x"),
		"bad dump\n");
	free(buf);
	buf = NULL;

	ec_pnode_free(p);
	ec_node_free(node);
	return testres;

fail:
	ec_pnode_free(p2);
	ec_pnode_free(p);
	ec_node_free(node);
	if (f != NULL)
		fclose(f);
	free(buf);

	return -1;
}
