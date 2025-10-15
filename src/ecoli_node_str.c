/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ecoli_log.h>
#include <ecoli_malloc.h>
#include <ecoli_strvec.h>
#include <ecoli_node.h>
#include <ecoli_config.h>
#include <ecoli_parse.h>
#include <ecoli_complete.h>
#include <ecoli_node_str.h>

EC_LOG_TYPE_REGISTER(node_str);

struct ec_node_str {
	char *string;
	unsigned len;
};

static int
ec_node_str_parse(const struct ec_node *node,
		struct ec_pnode *pstate,
		const struct ec_strvec *strvec)
{
	struct ec_node_str *priv = ec_node_priv(node);
	const char *str;

	(void)pstate;

	if (priv->string == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (ec_strvec_len(strvec) == 0)
		return EC_PARSE_NOMATCH;

	str = ec_strvec_val(strvec, 0);
	if (strcmp(str, priv->string) != 0)
		return EC_PARSE_NOMATCH;

	return 1;
}

static int
ec_node_str_complete(const struct ec_node *node,
		struct ec_comp *comp,
		const struct ec_strvec *strvec)
{
	struct ec_node_str *priv = ec_node_priv(node);
	const struct ec_comp_item *item;
	const char *str;
	size_t n = 0;

	if (ec_strvec_len(strvec) != 1)
		return 0;

	/* XXX startswith ? */
	str = ec_strvec_val(strvec, 0);
	for (n = 0; n < priv->len; n++) {
		if (str[n] != priv->string[n])
			break;
	}

	/* no completion */
	if (str[n] != '\0')
		return EC_PARSE_NOMATCH;

	item = ec_comp_add_item(comp, node, EC_COMP_FULL, str, priv->string);
	if (item == NULL)
		return -1;

	return 0;
}

static char *ec_node_str_desc(const struct ec_node *node)
{
	struct ec_node_str *priv = ec_node_priv(node);

	return ec_strdup(priv->string);
}

static void ec_node_str_free_priv(struct ec_node *node)
{
	struct ec_node_str *priv = ec_node_priv(node);

	ec_free(priv->string);
}

static const struct ec_config_schema ec_node_str_schema[] = {
	{
		.key = "string",
		.desc = "The string to match.",
		.type = EC_CONFIG_TYPE_STRING,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static int ec_node_str_set_config(struct ec_node *node,
				const struct ec_config *config)
{
	struct ec_node_str *priv = ec_node_priv(node);
	const struct ec_config *value = NULL;
	char *s = NULL;

	value = ec_config_dict_get(config, "string");
	if (value == NULL) {
		errno = EINVAL;
		goto fail;
	}

	s = ec_strdup(value->string);
	if (s == NULL)
		goto fail;

	ec_free(priv->string);
	priv->string = s;
	priv->len = strlen(priv->string);

	return 0;

fail:
	ec_free(s);
	return -1;
}

static struct ec_node_type ec_node_str_type = {
	.name = "str",
	.schema = ec_node_str_schema,
	.set_config = ec_node_str_set_config,
	.parse = ec_node_str_parse,
	.complete = ec_node_str_complete,
	.desc = ec_node_str_desc,
	.size = sizeof(struct ec_node_str),
	.free_priv = ec_node_str_free_priv,
};

EC_NODE_TYPE_REGISTER(ec_node_str_type);

int ec_node_str_set_str(struct ec_node *node, const char *str)
{
	struct ec_config *config = NULL;
	int ret;

	if (ec_node_check_type(node, &ec_node_str_type) < 0)
		goto fail;

	if (str == NULL) {
		errno = EINVAL;
		goto fail;
	}

	config = ec_config_dict();
	if (config == NULL)
		goto fail;

	ret = ec_config_dict_set(config, "string", ec_config_string(str));
	if (ret < 0)
		goto fail;

	ret = ec_node_set_config(node, config);
	config = NULL;
	if (ret < 0)
		goto fail;

	return 0;

fail:
	ec_config_free(config);
	return -1;
}

struct ec_node *ec_node_str(const char *id, const char *str)
{
	struct ec_node *node = NULL;

	node = ec_node_from_type(&ec_node_str_type, id);
	if (node == NULL)
		goto fail;

	if (ec_node_str_set_str(node, str) < 0)
		goto fail;

	return node;

fail:
	ec_node_free(node);
	return NULL;
}
