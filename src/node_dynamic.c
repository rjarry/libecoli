/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2017, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ecoli_log.h>
#include <ecoli_malloc.h>
#include <ecoli_test.h>
#include <ecoli_strvec.h>
#include <ecoli_dict.h>
#include <ecoli_node.h>
#include <ecoli_parse.h>
#include <ecoli_complete.h>
#include <ecoli_string.h>
#include <ecoli_node_str.h>
#include <ecoli_node_many.h>

#include "ecoli_node_dynamic.h"

EC_LOG_TYPE_REGISTER(node_dynamic);

struct ec_node_dynamic {
	ec_node_dynamic_build_t build;
	void *opaque;
};

static int
ec_node_dynamic_parse(const struct ec_node *node,
		struct ec_pnode *parse,
		const struct ec_strvec *strvec)
{
	struct ec_node_dynamic *priv = ec_node_priv(node);
	struct ec_node *child = NULL;
	void (*node_free)(struct ec_node *) = ec_node_free;
	char key[64];
	int ret = -1;

	child = priv->build(parse, priv->opaque);
	if (child == NULL)
		goto fail;

	/* add the node pointer in the attributes, so it will be freed
	 * when parse is freed */
	snprintf(key, sizeof(key), "_dyn_%p", child);
	ret = ec_dict_set(ec_pnode_get_attrs(parse), key, child,
			(void *)node_free);
	if (ret < 0) {
		child = NULL; /* already freed */
		goto fail;
	}

	return ec_parse_child(child, parse, strvec);

fail:
	ec_node_free(child);
	return ret;
}

static int
ec_node_dynamic_complete(const struct ec_node *node,
		struct ec_comp *comp,
		const struct ec_strvec *strvec)
{
	struct ec_node_dynamic *priv = ec_node_priv(node);
	struct ec_pnode *parse;
	struct ec_node *child = NULL;
	void (*node_free)(struct ec_node *) = ec_node_free;
	char key[64];
	int ret = -1;

	parse = ec_comp_get_cur_pstate(comp);
	child = priv->build(parse, priv->opaque);
	if (child == NULL)
		goto fail;

	/* add the node pointer in the attributes, so it will be freed
	 * when parse is freed */
	snprintf(key, sizeof(key), "_dyn_%p", child);
	ret = ec_dict_set(ec_comp_get_attrs(comp), key, child,
			(void *)node_free);
	if (ret < 0) {
		child = NULL; /* already freed */
		goto fail;
	}

	return ec_complete_child(child, comp, strvec);

fail:
	ec_node_free(child);
	return ret;
}

static struct ec_node_type ec_node_dynamic_type = {
	.name = "dynamic",
	.parse = ec_node_dynamic_parse,
	.complete = ec_node_dynamic_complete,
	.size = sizeof(struct ec_node_dynamic),
};

struct ec_node *
ec_node_dynamic(const char *id, ec_node_dynamic_build_t build, void *opaque)
{
	struct ec_node *node = NULL;
	struct ec_node_dynamic *priv;

	if (build == NULL) {
		errno = EINVAL;
		goto fail;
	}

	node = ec_node_from_type(&ec_node_dynamic_type, id);
	if (node == NULL)
		goto fail;

	priv = ec_node_priv(node);
	priv->build = build;
	priv->opaque = opaque;

	return node;

fail:
	ec_node_free(node);
	return NULL;

}

EC_NODE_TYPE_REGISTER(ec_node_dynamic_type);

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

/* LCOV_EXCL_START */
static int ec_node_dynamic_testcase(void)
{
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

static struct ec_test ec_node_dynamic_test = {
	.name = "node_dynamic",
	.test = ec_node_dynamic_testcase,
};

EC_TEST_REGISTER(ec_node_dynamic_test);
/* LCOV_EXCL_STOP */
