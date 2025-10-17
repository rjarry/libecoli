/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2017, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ecoli_log.h>
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
