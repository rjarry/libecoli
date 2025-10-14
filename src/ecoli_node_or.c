/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <ecoli_log.h>
#include <ecoli_strvec.h>
#include <ecoli_node.h>
#include <ecoli_parse.h>
#include <ecoli_complete.h>
#include <ecoli_node.h>
#include <ecoli_config.h>
#include <ecoli_node_helper.h>
#include <ecoli_node_or.h>
#include <ecoli_node_str.h>

EC_LOG_TYPE_REGISTER(node_or);

struct ec_node_or {
	struct ec_node **table;
	size_t len;
};

static int
ec_node_or_parse(const struct ec_node *node,
		struct ec_pnode *pstate,
		const struct ec_strvec *strvec)
{
	struct ec_node_or *priv = ec_node_priv(node);
	unsigned int i;
	int ret;

	for (i = 0; i < priv->len; i++) {
		ret = ec_parse_child(priv->table[i], pstate, strvec);
		if (ret == EC_PARSE_NOMATCH)
			continue;
		return ret;
	}

	return EC_PARSE_NOMATCH;
}

static int
ec_node_or_complete(const struct ec_node *node,
		struct ec_comp *comp,
		const struct ec_strvec *strvec)
{
	struct ec_node_or *priv = ec_node_priv(node);
	int ret;
	size_t n;

	for (n = 0; n < priv->len; n++) {
		ret = ec_complete_child(priv->table[n],
					comp, strvec);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static void ec_node_or_free_priv(struct ec_node *node)
{
	struct ec_node_or *priv = ec_node_priv(node);
	size_t i;

	for (i = 0; i < priv->len; i++)
		ec_node_free(priv->table[i]);
	free(priv->table);
	priv->table = NULL;
	priv->len = 0;
}

static const struct ec_config_schema ec_node_or_subschema[] = {
	{
		.desc = "A child node which is part of the choice.",
		.type = EC_CONFIG_TYPE_NODE,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static const struct ec_config_schema ec_node_or_schema[] = {
	{
		.key = "children",
		.desc = "The list of children nodes defining the choice "
		"elements.",
		.type = EC_CONFIG_TYPE_LIST,
		.subschema = ec_node_or_subschema,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static int ec_node_or_set_config(struct ec_node *node,
				const struct ec_config *config)
{
	struct ec_node_or *priv = ec_node_priv(node);
	struct ec_node **table = NULL;
	size_t i, len = 0;

	table = ec_node_config_node_list_to_table(
		ec_config_dict_get(config, "children"), &len);
	if (table == NULL)
		return -1;

	for (i = 0; i < priv->len; i++)
		ec_node_free(priv->table[i]);
	free(priv->table);
	priv->table = table;
	priv->len = len;

	return 0;
}

static size_t
ec_node_or_get_children_count(const struct ec_node *node)
{
	struct ec_node_or *priv = ec_node_priv(node);
	return priv->len;
}

static int
ec_node_or_get_child(const struct ec_node *node, size_t i,
		struct ec_node **child, unsigned int *refs)
{
	struct ec_node_or *priv = ec_node_priv(node);

	if (i >= priv->len)
		return -1;

	*child = priv->table[i];
	/* each child node is referenced twice: once in the config and
	 * once in the priv->table[] */
	*refs = 2;
	return 0;
}

static struct ec_node_type ec_node_or_type = {
	.name = "or",
	.schema = ec_node_or_schema,
	.set_config = ec_node_or_set_config,
	.parse = ec_node_or_parse,
	.complete = ec_node_or_complete,
	.size = sizeof(struct ec_node_or),
	.free_priv = ec_node_or_free_priv,
	.get_children_count = ec_node_or_get_children_count,
	.get_child = ec_node_or_get_child,
};

EC_NODE_TYPE_REGISTER(ec_node_or_type);

int ec_node_or_add(struct ec_node *node, struct ec_node *child)
{
	const struct ec_config *cur_config = NULL;
	struct ec_config *config = NULL, *children;
	int ret;

	assert(node != NULL);

	/* XXX factorize this code in a helper */

	if (ec_node_check_type(node, &ec_node_or_type) < 0)
		goto fail;

	cur_config = ec_node_get_config(node);
	if (cur_config == NULL)
		config = ec_config_dict();
	else
		config = ec_config_dup(cur_config);
	if (config == NULL)
		goto fail;

	children = ec_config_dict_get(config, "children");
	if (children == NULL) {
		children = ec_config_list();
		if (children == NULL)
			goto fail;

		if (ec_config_dict_set(config, "children", children) < 0)
			goto fail; /* children list is freed on error */
	}

	if (ec_config_list_add(children, ec_config_node(child)) < 0) {
		child = NULL;
		goto fail;
	}

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

struct ec_node *__ec_node_or(const char *id, ...)
{
	struct ec_config *config = NULL, *children = NULL;
	struct ec_node *node = NULL;
	struct ec_node *child;
	va_list ap;
	int ret;

	va_start(ap, id);
	child = va_arg(ap, struct ec_node *);

	node = ec_node_from_type(&ec_node_or_type, id);
	if (node == NULL)
		goto fail_free_children;

	config = ec_config_dict();
	if (config == NULL)
		goto fail_free_children;

	children = ec_config_list();
	if (children == NULL)
		goto fail_free_children;

	for (; child != EC_VA_END; child = va_arg(ap, struct ec_node *)) {
		if (child == NULL)
			goto fail_free_children;

		if (ec_config_list_add(children, ec_config_node(child)) < 0) {
			child = NULL;
			goto fail_free_children;
		}
	}

	if (ec_config_dict_set(config, "children", children) < 0) {
		children = NULL; /* freed */
		goto fail;
	}
	children = NULL;

	ret = ec_node_set_config(node, config);
	config = NULL; /* freed */
	if (ret < 0)
		goto fail;

	va_end(ap);

	return node;

fail_free_children:
	for (; child != EC_VA_END; child = va_arg(ap, struct ec_node *))
		ec_node_free(child);
fail:
	ec_node_free(node); /* will also free added children */
	ec_config_free(children);
	ec_config_free(config);
	va_end(ap);

	return NULL;
}
