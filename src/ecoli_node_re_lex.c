/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <errno.h>

#include <ecoli_malloc.h>
#include <ecoli_log.h>
#include <ecoli_strvec.h>
#include <ecoli_dict.h>
#include <ecoli_node.h>
#include <ecoli_complete.h>
#include <ecoli_parse.h>
#include <ecoli_config.h>
#include <ecoli_node_many.h>
#include <ecoli_node_or.h>
#include <ecoli_node_str.h>
#include <ecoli_node_int.h>
#include <ecoli_node_re_lex.h>

EC_LOG_TYPE_REGISTER(node_re_lex);

struct regexp_pattern {
	char *pattern;
	char *attr_name;
	regex_t r;
	bool keep;
};

struct ec_node_re_lex {
	struct ec_node *child;
	struct regexp_pattern *table;
	size_t len;
};

static struct ec_strvec *
tokenize(struct regexp_pattern *table, size_t table_len, const char *str)
{
	struct ec_strvec *strvec = NULL;
	struct ec_dict *attrs = NULL;
	char *dup = NULL;
	char c;
	size_t len, off = 0;
	size_t i;
	int ret = 0;
	regmatch_t pos = {0};

	dup = ec_strdup(str);
	if (dup == NULL)
		goto fail;

	strvec = ec_strvec();
	if (strvec == NULL)
		goto fail;

	len = strlen(dup);
	while (off < len) {
		for (i = 0; i < table_len; i++) {
			ret = regexec(&table[i].r, &dup[off], 1, &pos, 0);
			if (ret != 0)
				continue;
			if (pos.rm_so != 0 || pos.rm_eo == 0) {
				ret = -1;
				continue;
			}

			if (table[i].keep == 0)
				break;

			c = dup[pos.rm_eo + off];
			dup[pos.rm_eo + off] = '\0';
			EC_LOG(EC_LOG_DEBUG, "re_lex match <%s>\n", &dup[off]);
			if (ec_strvec_add(strvec, &dup[off]) < 0)
				goto fail;

			if (table[i].attr_name != NULL) {
				attrs = ec_dict();
				if (attrs == NULL)
					goto fail;
				if (ec_dict_set(attrs, table[i].attr_name,
						NULL, NULL) < 0)
					goto fail;
				if (ec_strvec_set_attrs(strvec,
						ec_strvec_len(strvec) - 1,
						attrs) < 0) {
					attrs = NULL;
					goto fail;
				}
				attrs = NULL;
			}

			dup[pos.rm_eo + off] = c;
			break;
		}

		if (ret != 0)
			goto fail;

		off += pos.rm_eo;
	}

	ec_free(dup);
	return strvec;

fail:
	ec_free(dup);
	ec_strvec_free(strvec);
	return NULL;
}

static int
ec_node_re_lex_parse(const struct ec_node *node,
		struct ec_pnode *pstate,
		const struct ec_strvec *strvec)
{
	struct ec_node_re_lex *priv = ec_node_priv(node);
	struct ec_strvec *new_vec = NULL;
	struct ec_pnode *child_parse;
	const char *str;
	int ret;

	if (priv->child == NULL) {
		errno = EINVAL;
		goto fail;
	}

	if (ec_strvec_len(strvec) == 0) {
		new_vec = ec_strvec();
	} else {
		str = ec_strvec_val(strvec, 0);
		new_vec = tokenize(priv->table, priv->len, str);
	}
	if (new_vec == NULL)
		goto fail;

	ret = ec_parse_child(priv->child, pstate, new_vec);
	if (ret < 0)
		goto fail;

	if ((unsigned)ret == ec_strvec_len(new_vec)) {
		ret = 1;
	} else if (ret != EC_PARSE_NOMATCH) {
		child_parse = ec_pnode_get_last_child(pstate);
		ec_pnode_unlink_child(child_parse);
		ec_pnode_free(child_parse);
		ret = EC_PARSE_NOMATCH;
	}

	ec_strvec_free(new_vec);
	new_vec = NULL;

	return ret;

 fail:
	ec_strvec_free(new_vec);
	return -1;
}

static void ec_node_re_lex_free_priv(struct ec_node *node)
{
	struct ec_node_re_lex *priv = ec_node_priv(node);
	unsigned int i;

	ec_node_free(priv->child);
	for (i = 0; i < priv->len; i++) {
		ec_free(priv->table[i].pattern);
		ec_free(priv->table[i].attr_name);
		regfree(&priv->table[i].r);
	}

	ec_free(priv->table);
}

static size_t
ec_node_re_lex_get_children_count(const struct ec_node *node)
{
	struct ec_node_re_lex *priv = ec_node_priv(node);

	if (priv->child)
		return 1;
	return 0;
}

static int
ec_node_re_lex_get_child(const struct ec_node *node, size_t i,
			struct ec_node **child, unsigned int *refs)
{
	struct ec_node_re_lex *priv = ec_node_priv(node);

	if (i >= 1)
		return -1;

	*child = priv->child;
	*refs = 2;
	return 0;
}

static const struct ec_config_schema ec_node_re_lex_dict[] = {
	{
		.key = "pattern",
		.desc = "The pattern to match.",
		.type = EC_CONFIG_TYPE_STRING,
	},
	{
		.key = "keep",
		.desc = "Whether to keep or drop the string matching "
		"the regular expression.",
		.type = EC_CONFIG_TYPE_BOOL,
	},
	{
		.key = "attr",
		.desc = "The optional attribute name to attach.",
		.type = EC_CONFIG_TYPE_STRING,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static const struct ec_config_schema ec_node_re_lex_elt[] = {
	{
		.desc = "A pattern element.",
		.type = EC_CONFIG_TYPE_DICT,
		.subschema = ec_node_re_lex_dict,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static const struct ec_config_schema ec_node_re_lex_schema[] = {
	{
		.key = "patterns",
		.desc = "The list of patterns elements.",
		.type = EC_CONFIG_TYPE_LIST,
		.subschema = ec_node_re_lex_elt,
	},
	{
		.key = "child",
		.desc = "The child node.",
		.type = EC_CONFIG_TYPE_NODE,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static int ec_node_re_lex_set_config(struct ec_node *node,
				const struct ec_config *config)
{
	struct ec_node_re_lex *priv = ec_node_priv(node);
	struct regexp_pattern *table = NULL;
	const struct ec_config *patterns, *child, *elt, *pattern, *keep, *attr;
	char *pattern_str = NULL, *attr_name = NULL;
	ssize_t i, n = 0;
	int ret;

	child = ec_config_dict_get(config, "child");
	if (child == NULL)
		goto fail;
	if (ec_config_get_type(child) != EC_CONFIG_TYPE_NODE) {
		errno = EINVAL;
		goto fail;
	}

	patterns = ec_config_dict_get(config, "patterns");
	if (patterns != NULL) {
		n = ec_config_count(patterns);
		if (n < 0)
			goto fail;

		table = ec_calloc(n, sizeof(*table));
		if (table == NULL)
			goto fail;

		n = 0;
		TAILQ_FOREACH(elt, &patterns->list, next) {
			if (ec_config_get_type(elt) != EC_CONFIG_TYPE_DICT) {
				errno = EINVAL;
				goto fail;
			}
			pattern = ec_config_dict_get(elt, "pattern");
			if (pattern == NULL) {
				errno = EINVAL;
				goto fail;
			}
			if (ec_config_get_type(pattern) != EC_CONFIG_TYPE_STRING) {
				errno = EINVAL;
				goto fail;
			}
			keep = ec_config_dict_get(elt, "keep");
			if (keep == NULL) {
				errno = EINVAL;
				goto fail;
			}
			if (ec_config_get_type(keep) != EC_CONFIG_TYPE_BOOL) {
				errno = EINVAL;
				goto fail;
			}
			attr = ec_config_dict_get(elt, "attr");
			if (attr != NULL && ec_config_get_type(attr) !=
					EC_CONFIG_TYPE_STRING) {
				errno = EINVAL;
				goto fail;
			}
			pattern_str = ec_strdup(pattern->string);
			if (pattern_str == NULL)
				goto fail;
			if (attr != NULL && attr->string != NULL) {
				attr_name = ec_strdup(attr->string);
				if (attr_name == NULL)
					goto fail;
			}

			ret = regcomp(&table[n].r, pattern_str, REG_EXTENDED);
			if (ret != 0) {
				EC_LOG(EC_LOG_ERR,
					"Regular expression <%s> compilation failed: %d\n",
					pattern_str, ret);
				if (ret == REG_ESPACE)
					errno = ENOMEM;
				else
					errno = EINVAL;
				goto fail;
			}
			table[n].pattern = pattern_str;
			table[n].keep = keep->boolean;
			table[n].attr_name = attr_name;
			pattern_str = NULL;
			attr_name = NULL;

			n++;
		}
	}

	if (priv->child != NULL)
		ec_node_free(priv->child);
	priv->child = ec_node_clone(child->node);
	for (i = 0; i < (ssize_t)priv->len; i++) {
		ec_free(priv->table[i].pattern);
		ec_free(priv->table[i].attr_name);
		regfree(&priv->table[i].r);
	}
	ec_free(priv->table);
	priv->table = table;
	priv->len = n;

	return 0;

fail:
	if (table != NULL) {
		for (i = 0; i < n; i++) {
			if (table[i].pattern != NULL) {
				ec_free(table[i].pattern);
				ec_free(table[i].attr_name);
				regfree(&table[i].r);
			}
		}
	}
	ec_free(table);
	ec_free(pattern_str);
	ec_free(attr_name);
	return -1;
}

static struct ec_node_type ec_node_re_lex_type = {
	.name = "re_lex",
	.schema = ec_node_re_lex_schema,
	.set_config = ec_node_re_lex_set_config,
	.parse = ec_node_re_lex_parse,
	.size = sizeof(struct ec_node_re_lex),
	.free_priv = ec_node_re_lex_free_priv,
	.get_children_count = ec_node_re_lex_get_children_count,
	.get_child = ec_node_re_lex_get_child,
};

EC_NODE_TYPE_REGISTER(ec_node_re_lex_type);

int ec_node_re_lex_add(struct ec_node *node, const char *pattern, int keep,
	const char *attr_name)
{
	const struct ec_config *cur_config = NULL;
	struct ec_config *config = NULL, *patterns = NULL, *elt = NULL;
	int ret;

	if (ec_node_check_type(node, &ec_node_re_lex_type) < 0)
		goto fail;

	elt = ec_config_dict();
	if (elt == NULL)
		goto fail;
	if (ec_config_dict_set(elt, "pattern", ec_config_string(pattern)) < 0)
		goto fail;
	if (ec_config_dict_set(elt, "keep", ec_config_bool(keep)) < 0)
		goto fail;
	if (attr_name != NULL) {
		if (ec_config_dict_set(elt, "attr",
					ec_config_string(attr_name)) < 0)
			goto fail;
	}

	cur_config = ec_node_get_config(node);
	if (cur_config == NULL)
		config = ec_config_dict();
	else
		config = ec_config_dup(cur_config);
	if (config == NULL)
		goto fail;

	patterns = ec_config_dict_get(config, "patterns");
	if (patterns == NULL) {
		patterns = ec_config_list();
		if (patterns == NULL)
			goto fail;

		if (ec_config_dict_set(config, "patterns", patterns) < 0)
			goto fail; /* patterns list is freed on error */
	}

	if (ec_config_list_add(patterns, elt) < 0) {
		elt = NULL;
		goto fail;
	}
	elt = NULL;

	ret = ec_node_set_config(node, config);
	config = NULL; /* freed */
	if (ret < 0)
		goto fail;

	return 0;

fail:
	ec_config_free(config);
	ec_config_free(elt);
	return -1;
}

static int
ec_node_re_lex_set_child(struct ec_node *node, struct ec_node *child)
{
	const struct ec_config *cur_config = NULL;
	struct ec_config *config = NULL;
	int ret;

	if (ec_node_check_type(node, &ec_node_re_lex_type) < 0)
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

struct ec_node *ec_node_re_lex(const char *id, struct ec_node *child)
{
	struct ec_node *node = NULL;

	if (child == NULL)
		return NULL;

	node = ec_node_from_type(&ec_node_re_lex_type, id);
	if (node == NULL)
		goto fail;

	if (ec_node_re_lex_set_child(node, child) < 0) {
		child = NULL; /* freed */
		goto fail;
	}

	return node;

fail:
	ec_node_free(node);
	ec_node_free(child);
	return NULL;
}
