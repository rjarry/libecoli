/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli/complete.h>
#include <ecoli/config.h>
#include <ecoli/dict.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_any.h>
#include <ecoli/parse.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(node_any);

struct ec_node_any {
	char *attr_name;
};

static int ec_node_any_parse(const struct ec_node *node,
			struct ec_pnode *pstate,
			const struct ec_strvec *strvec)
{
	struct ec_node_any *priv = ec_node_priv(node);
	const struct ec_dict *attrs;

	(void)pstate;

	if (ec_strvec_len(strvec) == 0)
		return EC_PARSE_NOMATCH;
	if (priv->attr_name != NULL) {
		attrs = ec_strvec_get_attrs(strvec, 0);
		if (attrs == NULL || !ec_dict_has_key(attrs, priv->attr_name))
			return EC_PARSE_NOMATCH;
	}

	return 1;
}

static void ec_node_any_free_priv(struct ec_node *node)
{
	struct ec_node_any *priv = ec_node_priv(node);

	free(priv->attr_name);
}

static const struct ec_config_schema ec_node_any_schema[] = {
	{
		.key = "attr",
		.desc = "The optional attribute name to attach.",
		.type = EC_CONFIG_TYPE_STRING,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static int ec_node_any_set_config(struct ec_node *node,
				const struct ec_config *config)
{
	struct ec_node_any *priv = ec_node_priv(node);
	const struct ec_config *value = NULL;
	char *s = NULL;

	value = ec_config_dict_get(config, "attr");
	if (value != NULL) {
		s = strdup(value->string);
		if (s == NULL)
			goto fail;
	}

	free(priv->attr_name);
	priv->attr_name = s;

	return 0;

fail:
	free(s);
	return -1;
}

static struct ec_node_type ec_node_any_type = {
	.name = "any",
	.schema = ec_node_any_schema,
	.set_config = ec_node_any_set_config,
	.parse = ec_node_any_parse,
	.size = sizeof(struct ec_node_any),
	.free_priv = ec_node_any_free_priv,
};

EC_NODE_TYPE_REGISTER(ec_node_any_type);

struct ec_node *
ec_node_any(const char *id, const char *attr)
{
	struct ec_config *config = NULL;
	struct ec_node *node = NULL;
	int ret;

	node = ec_node_from_type(&ec_node_any_type, id);
	if (node == NULL)
		return NULL;

	config = ec_config_dict();
	if (config == NULL)
		goto fail;

	ret = ec_config_dict_set(config, "attr", ec_config_string(attr));
	if (ret < 0)
		goto fail;

	ret = ec_node_set_config(node, config);
	config = NULL;
	if (ret < 0)
		goto fail;

	return node;

fail:
	ec_config_free(config);
	ec_node_free(node);
	return NULL;
}
