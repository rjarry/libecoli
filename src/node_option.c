/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli/complete.h>
#include <ecoli/config.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_option.h>
#include <ecoli/node_str.h>
#include <ecoli/parse.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(node_option);

struct ec_node_option {
	struct ec_node *child;
};

static int ec_node_option_parse(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
)
{
	struct ec_node_option *priv = ec_node_priv(node);
	int ret;

	ret = ec_parse_child(priv->child, pstate, strvec);
	if (ret < 0)
		return ret;

	if (ret == EC_PARSE_NOMATCH)
		return 0;

	return ret;
}

static int ec_node_option_complete(
	const struct ec_node *node,
	struct ec_comp *comp,
	const struct ec_strvec *strvec
)
{
	struct ec_node_option *priv = ec_node_priv(node);

	return ec_complete_child(priv->child, comp, strvec);
}

static void ec_node_option_free_priv(struct ec_node *node)
{
	struct ec_node_option *priv = ec_node_priv(node);

	ec_node_free(priv->child);
}

static size_t ec_node_option_get_children_count(const struct ec_node *node)
{
	struct ec_node_option *priv = ec_node_priv(node);

	if (priv->child)
		return 1;
	return 0;
}

static int ec_node_option_get_child(
	const struct ec_node *node,
	size_t i,
	struct ec_node **child,
	unsigned int *refs
)
{
	struct ec_node_option *priv = ec_node_priv(node);

	if (i >= 1)
		return -1;

	*child = priv->child;
	*refs = 2;
	return 0;
}

static const struct ec_config_schema ec_node_option_schema[] = {
	{
		.key = "child",
		.desc = "The child node.",
		.type = EC_CONFIG_TYPE_NODE,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static int ec_node_option_set_config(struct ec_node *node, const struct ec_config *config)
{
	struct ec_node_option *priv = ec_node_priv(node);
	const struct ec_config *child;

	child = ec_config_dict_get(config, "child");
	if (child == NULL)
		goto fail;
	if (ec_config_get_type(child) != EC_CONFIG_TYPE_NODE) {
		errno = EINVAL;
		goto fail;
	}

	if (priv->child != NULL)
		ec_node_free(priv->child);
	priv->child = ec_node_clone(child->node);

	return 0;

fail:
	return -1;
}

static struct ec_node_type ec_node_option_type = {
	.name = "option",
	.schema = ec_node_option_schema,
	.set_config = ec_node_option_set_config,
	.parse = ec_node_option_parse,
	.complete = ec_node_option_complete,
	.size = sizeof(struct ec_node_option),
	.free_priv = ec_node_option_free_priv,
	.get_children_count = ec_node_option_get_children_count,
	.get_child = ec_node_option_get_child,
};

EC_NODE_TYPE_REGISTER(ec_node_option_type);

int ec_node_option_set_child(struct ec_node *node, struct ec_node *child)
{
	const struct ec_config *cur_config = NULL;
	struct ec_config *config = NULL;
	int ret;

	if (ec_node_check_type(node, &ec_node_option_type) < 0)
		goto fail;

	cur_config = ec_node_get_config(node);
	if (cur_config == NULL)
		config = ec_config_dict();
	else
		config = ec_config_dup(cur_config);
	if (config == NULL)
		goto fail;

	if (ec_config_dict_set(config, "child", ec_config_node(child)) < 0) {
		child = NULL; /* freed */
		goto fail;
	}
	child = NULL; /* freed */

	ret = ec_node_set_config(node, config);
	config = NULL; /* freed */
	if (ret < 0)
		goto fail;

	return 0;

fail:
	ec_config_free(config);
	ec_node_free(child);
	return -1;
}

struct ec_node *ec_node_option(const char *id, struct ec_node *child)
{
	struct ec_node *node = NULL;

	if (child == NULL)
		goto fail;

	node = ec_node_from_type(&ec_node_option_type, id);
	if (node == NULL)
		goto fail;

	ec_node_option_set_child(node, child);
	child = NULL;

	return node;

fail:
	ec_node_free(child);
	return NULL;
}
