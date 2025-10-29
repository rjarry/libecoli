/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli/complete.h>
#include <ecoli/config.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_re.h>
#include <ecoli/parse.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(node_re);

struct ec_node_re {
	char *re_str;
	regex_t re;
};

static int ec_node_re_parse(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
)
{
	struct ec_node_re *priv = ec_node_priv(node);
	const char *str;
	regmatch_t pos;

	(void)pstate;

	if (ec_strvec_len(strvec) == 0)
		return EC_PARSE_NOMATCH;

	str = ec_strvec_val(strvec, 0);
	if (regexec(&priv->re, str, 1, &pos, 0) != 0)
		return EC_PARSE_NOMATCH;
	if (pos.rm_so != 0 || pos.rm_eo != (int)strlen(str))
		return EC_PARSE_NOMATCH;

	return 1;
}

static void ec_node_re_free_priv(struct ec_node *node)
{
	struct ec_node_re *priv = ec_node_priv(node);

	if (priv->re_str != NULL) {
		free(priv->re_str);
		regfree(&priv->re);
	}
}

static const struct ec_config_schema ec_node_re_schema[] = {
	{
		.key = "pattern",
		.desc = "The pattern to match.",
		.type = EC_CONFIG_TYPE_STRING,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static int ec_node_re_set_config(struct ec_node *node, const struct ec_config *config)
{
	struct ec_node_re *priv = ec_node_priv(node);
	const struct ec_config *value = NULL;
	char *s = NULL;
	regex_t re;
	int ret;

	value = ec_config_dict_get(config, "pattern");
	if (value == NULL) {
		errno = EINVAL;
		goto fail;
	}

	s = strdup(value->string);
	if (s == NULL)
		goto fail;

	ret = regcomp(&re, s, REG_EXTENDED);
	if (ret != 0) {
		if (ret == REG_ESPACE)
			errno = ENOMEM;
		else
			errno = EINVAL;
		goto fail;
	}

	if (priv->re_str != NULL) {
		free(priv->re_str);
		regfree(&priv->re);
	}
	priv->re_str = s;
	priv->re = re;

	return 0;

fail:
	free(s);
	return -1;
}

static struct ec_node_type ec_node_re_type = {
	.name = "re",
	.schema = ec_node_re_schema,
	.set_config = ec_node_re_set_config,
	.parse = ec_node_re_parse,
	.size = sizeof(struct ec_node_re),
	.free_priv = ec_node_re_free_priv,
};

EC_NODE_TYPE_REGISTER(ec_node_re_type);

int ec_node_re_set_regexp(struct ec_node *node, const char *str)
{
	struct ec_config *config = NULL;
	int ret;

	EC_CHECK_ARG(str != NULL, -1, EINVAL);

	if (ec_node_check_type(node, &ec_node_re_type) < 0)
		goto fail;

	config = ec_config_dict();
	if (config == NULL)
		goto fail;

	ret = ec_config_dict_set(config, "pattern", ec_config_string(str));
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

struct ec_node *ec_node_re(const char *id, const char *re_str)
{
	struct ec_node *node = NULL;

	node = ec_node_from_type(&ec_node_re_type, id);
	if (node == NULL)
		goto fail;

	if (ec_node_re_set_regexp(node, re_str) < 0)
		goto fail;

	return node;

fail:
	ec_node_free(node);
	return NULL;
}
