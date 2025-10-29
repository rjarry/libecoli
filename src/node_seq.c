/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli/complete.h>
#include <ecoli/config.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_helper.h>
#include <ecoli/node_many.h>
#include <ecoli/node_option.h>
#include <ecoli/node_or.h>
#include <ecoli/node_seq.h>
#include <ecoli/node_str.h>
#include <ecoli/parse.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(node_seq);

struct ec_node_seq {
	struct ec_node **table;
	size_t len;
};

static int ec_node_seq_parse(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
)
{
	struct ec_node_seq *priv = ec_node_priv(node);
	struct ec_strvec *childvec = NULL;
	size_t len = 0;
	unsigned int i;
	int ret;

	for (i = 0; i < priv->len; i++) {
		childvec = ec_strvec_ndup(strvec, len, ec_strvec_len(strvec) - len);
		if (childvec == NULL)
			goto fail;

		ret = ec_parse_child(priv->table[i], pstate, childvec);
		if (ret < 0)
			goto fail;

		ec_strvec_free(childvec);
		childvec = NULL;

		if (ret == EC_PARSE_NOMATCH) {
			ec_pnode_free_children(pstate);
			return EC_PARSE_NOMATCH;
		}

		len += ret;
	}

	return len;

fail:
	ec_strvec_free(childvec);
	return -1;
}

static int __ec_node_seq_complete(
	struct ec_node **table,
	size_t table_len,
	struct ec_comp *comp,
	const struct ec_strvec *strvec
)
{
	struct ec_pnode *parse = ec_comp_get_cur_pstate(comp);
	struct ec_strvec *childvec = NULL;
	unsigned int i;
	int ret;

	if (table_len == 0)
		return 0;

	/*
	 * Example of completion for a sequence node = [n1,n2] and an
	 * input = [a,b,c,d]:
	 *
	 * result = complete(n1, [a,b,c,d]) +
	 *    complete(n2, [b,c,d]) if n1 matches [a] +
	 *    complete(n2, [c,d]) if n1 matches [a,b] +
	 *    complete(n2, [d]) if n1 matches [a,b,c] +
	 *    complete(n2, []) if n1 matches [a,b,c,d]
	 */

	/* first, try to complete with the first node of the table */
	ret = ec_complete_child(table[0], comp, strvec);
	if (ret < 0)
		goto fail;

	/* then, if the first node of the table matches the beginning of the
	 * strvec, try to complete the rest */
	for (i = 0; i < ec_strvec_len(strvec); i++) {
		childvec = ec_strvec_ndup(strvec, 0, i);
		if (childvec == NULL)
			goto fail;

		ret = ec_parse_child(table[0], parse, childvec);
		if (ret < 0)
			goto fail;

		ec_strvec_free(childvec);
		childvec = NULL;

		if ((unsigned int)ret != i) {
			if (ret != EC_PARSE_NOMATCH)
				ec_pnode_del_last_child(parse);
			continue;
		}

		childvec = ec_strvec_ndup(strvec, i, ec_strvec_len(strvec) - i);
		if (childvec == NULL) {
			ec_pnode_del_last_child(parse);
			goto fail;
		}

		ret = __ec_node_seq_complete(&table[1], table_len - 1, comp, childvec);
		ec_pnode_del_last_child(parse);
		ec_strvec_free(childvec);
		childvec = NULL;

		if (ret < 0)
			goto fail;
	}

	return 0;

fail:
	ec_strvec_free(childvec);
	return -1;
}

static int ec_node_seq_complete(
	const struct ec_node *node,
	struct ec_comp *comp,
	const struct ec_strvec *strvec
)
{
	struct ec_node_seq *priv = ec_node_priv(node);

	return __ec_node_seq_complete(priv->table, priv->len, comp, strvec);
}

static void ec_node_seq_free_priv(struct ec_node *node)
{
	struct ec_node_seq *priv = ec_node_priv(node);
	size_t i;

	for (i = 0; i < priv->len; i++)
		ec_node_free(priv->table[i]);
	free(priv->table);
	priv->table = NULL;
	priv->len = 0;
}

static const struct ec_config_schema ec_node_seq_subschema[] = {
	{
		.desc = "A child node which is part of the sequence.",
		.type = EC_CONFIG_TYPE_NODE,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static const struct ec_config_schema ec_node_seq_schema[] = {
	{
		.key = "children",
		.desc = "The list of children nodes, to be parsed in sequence.",
		.type = EC_CONFIG_TYPE_LIST,
		.subschema = ec_node_seq_subschema,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static int ec_node_seq_set_config(struct ec_node *node, const struct ec_config *config)
{
	struct ec_node_seq *priv = ec_node_priv(node);
	struct ec_node **table = NULL;
	size_t i, len = 0;

	table = ec_node_config_node_list_to_table(ec_config_dict_get(config, "children"), &len);
	if (table == NULL)
		return -1;

	for (i = 0; i < priv->len; i++)
		ec_node_free(priv->table[i]);
	free(priv->table);
	priv->table = table;
	priv->len = len;

	return 0;
}

static size_t ec_node_seq_get_children_count(const struct ec_node *node)
{
	struct ec_node_seq *priv = ec_node_priv(node);
	return priv->len;
}

static int ec_node_seq_get_child(
	const struct ec_node *node,
	size_t i,
	struct ec_node **child,
	unsigned int *refs
)
{
	struct ec_node_seq *priv = ec_node_priv(node);

	if (i >= priv->len)
		return -1;

	*child = priv->table[i];
	/* each child node is referenced twice: once in the config and
	 * once in the priv->table[] */
	*refs = 2;
	return 0;
}

static struct ec_node_type ec_node_seq_type = {
	.name = "seq",
	.schema = ec_node_seq_schema,
	.set_config = ec_node_seq_set_config,
	.parse = ec_node_seq_parse,
	.complete = ec_node_seq_complete,
	.size = sizeof(struct ec_node_seq),
	.free_priv = ec_node_seq_free_priv,
	.get_children_count = ec_node_seq_get_children_count,
	.get_child = ec_node_seq_get_child,
};

EC_NODE_TYPE_REGISTER(ec_node_seq_type);

int ec_node_seq_add(struct ec_node *node, struct ec_node *child)
{
	const struct ec_config *cur_config = NULL;
	struct ec_config *config = NULL, *children;
	int ret;

	assert(node != NULL);

	/* XXX factorize this code in a helper */

	if (ec_node_check_type(node, &ec_node_seq_type) < 0)
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

struct ec_node *__ec_node_seq(const char *id, ...)
{
	struct ec_config *config = NULL, *children = NULL;
	struct ec_node *node = NULL;
	struct ec_node *child;
	va_list ap;
	int ret;

	va_start(ap, id);
	child = va_arg(ap, struct ec_node *);

	node = ec_node_from_type(&ec_node_seq_type, id);
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
