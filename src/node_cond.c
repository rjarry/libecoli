/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2019, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <ecoli/complete.h>
#include <ecoli/config.h>
#include <ecoli/dict.h>
#include <ecoli/htable.h>
#include <ecoli/init.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_any.h>
#include <ecoli/node_bypass.h>
#include <ecoli/node_cond.h>
#include <ecoli/node_many.h>
#include <ecoli/node_option.h>
#include <ecoli/node_or.h>
#include <ecoli/node_re.h>
#include <ecoli/node_re_lex.h>
#include <ecoli/node_seq.h>
#include <ecoli/node_str.h>
#include <ecoli/node_subset.h>
#include <ecoli/parse.h>
#include <ecoli/string.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(node_cond);

static struct ec_node *ec_node_cond_parser; /* the expression parser. */
static struct ec_dict *ec_node_cond_functions; /* functions dictionary */

struct ec_node_cond {
	char *cond_str; /* the condition string. */
	struct ec_pnode *parsed_cond; /* the parsed condition. */
	struct ec_node *child; /* the child node. */
};

enum cond_result_type {
	NODESET,
	BOOLEAN,
	INT,
	STR,
};

/*
 * XXX missing:
 * - find by attrs? get_attr ?
 * - get/set variable
 */

struct cond_result {
	enum cond_result_type type;
	union {
		struct ec_htable *htable;
		char *str;
		int64_t int64;
		bool boolean;
	};
};

typedef struct cond_result
	*(cond_func_t)(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len);

static void cond_result_free(struct cond_result *res)
{
	if (res == NULL)
		return;

	switch (res->type) {
	case NODESET:
		ec_htable_free(res->htable);
		break;
	case STR:
		free(res->str);
		break;
	case BOOLEAN:
	case INT:
		break;
	}

	free(res);
}

static void cond_result_table_free(struct cond_result **table, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		cond_result_free(table[i]);
		table[i] = NULL;
	}
	free(table);
}

static struct ec_node *ec_node_cond_build_parser(void)
{
	struct ec_node *lex = NULL;
	struct ec_node *expr = NULL;
	int ret;

	expr = ec_node("or", "id_arg");
	if (expr == NULL)
		goto fail;

	if (ec_node_or_add(
		    expr,
		    EC_NODE_SEQ(
			    "id_function",
			    ec_node_any("id_function_name", "a_identifier"),
			    ec_node_any(EC_NO_ID, "a_open"),
			    ec_node_option(
				    "id_arg_list",
				    EC_NODE_SEQ(
					    EC_NO_ID,
					    ec_node_clone(expr),
					    ec_node_many(
						    EC_NO_ID,
						    EC_NODE_SEQ(
							    EC_NO_ID,
							    ec_node_str(EC_NO_ID, ","),
							    ec_node_clone(expr)
						    ),
						    0,
						    0
					    )
				    )
			    ),
			    ec_node_any(EC_NO_ID, "a_close")
		    )
	    )
	    < 0)
		goto fail;

	if (ec_node_or_add(expr, ec_node_any("id_value_str", "a_identifier")) < 0)
		goto fail;

	if (ec_node_or_add(expr, ec_node_any("id_value_int", "a_int")) < 0)
		goto fail;

	/* prepend a lexer to the expression node */
	lex = ec_node_re_lex(EC_NO_ID, ec_node_clone(expr));
	if (lex == NULL)
		goto fail;

	ec_node_free(expr);
	expr = NULL;

	ret = ec_node_re_lex_add(lex, "[_a-zA-Z][._a-zA-Z0-9]*", 1, "a_identifier");
	if (ret < 0)
		goto fail;
	ret = ec_node_re_lex_add(lex, "[0-9]+", 1, "a_int");
	if (ret < 0)
		goto fail;
	ret = ec_node_re_lex_add(lex, "\\(", 1, "a_open");
	if (ret < 0)
		goto fail;
	ret = ec_node_re_lex_add(lex, "\\)", 1, "a_close");
	if (ret < 0)
		goto fail;
	ret = ec_node_re_lex_add(lex, ",", 1, NULL);
	if (ret < 0)
		goto fail;
	ret = ec_node_re_lex_add(lex, "[ 	]", 0, NULL);
	if (ret < 0)
		goto fail;

	return lex;

fail:
	ec_node_free(lex);
	ec_node_free(expr);

	return NULL;
}

static struct ec_pnode *ec_node_cond_build(const char *cond_str)
{
	struct ec_pnode *p = NULL;

	/* parse the condition expression */
	p = ec_parse(ec_node_cond_parser, cond_str);
	if (p == NULL)
		goto fail;

	if (!ec_pnode_matches(p)) {
		errno = EINVAL;
		goto fail;
	}

	return p;

fail:
	ec_pnode_free(p);
	return NULL;
}

static struct cond_result *
eval_root(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	struct cond_result *out = NULL;
	const struct ec_pnode *root = NULL;

	(void)in;

	if (in_len != 0) {
		EC_LOG(LOG_ERR, "root() does not take any argument\n");
		errno = EINVAL;
		goto fail;
	}

	out = malloc(sizeof(*out));
	if (out == NULL)
		goto fail;

	out->type = NODESET;
	out->htable = ec_htable();
	if (out->htable == NULL)
		goto fail;

	root = EC_PNODE_GET_ROOT(pstate);
	if (ec_htable_set(out->htable, &root, sizeof(root), NULL, NULL) < 0)
		goto fail;

	cond_result_table_free(in, in_len);
	return out;

fail:
	cond_result_free(out);
	cond_result_table_free(in, in_len);
	return NULL;
}

static struct cond_result *
eval_current(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	struct cond_result *out = NULL;

	(void)in;

	if (in_len != 0) {
		EC_LOG(LOG_ERR, "current() does not take any argument\n");
		errno = EINVAL;
		goto fail;
	}

	out = malloc(sizeof(*out));
	if (out == NULL)
		goto fail;

	out->type = NODESET;
	out->htable = ec_htable();
	if (out->htable == NULL)
		goto fail;

	if (ec_htable_set(out->htable, &pstate, sizeof(pstate), NULL, NULL) < 0)
		goto fail;

	cond_result_table_free(in, in_len);
	return out;

fail:
	cond_result_free(out);
	cond_result_table_free(in, in_len);
	return NULL;
}

static bool boolean_value(const struct cond_result *res)
{
	switch (res->type) {
	case NODESET:
		return ec_htable_len(res->htable) > 0;
	case BOOLEAN:
		return res->boolean;
	case INT:
		return res->int64 != 0;
	case STR:
		return res->str[0] != 0;
	}

	return false;
}

static struct cond_result *
eval_bool(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	struct cond_result *out = NULL;

	(void)pstate;

	if (in_len != 1) {
		EC_LOG(LOG_ERR, "bool() takes one argument.\n");
		errno = EINVAL;
		goto fail;
	}

	out = malloc(sizeof(*out));
	if (out == NULL)
		goto fail;

	out->type = BOOLEAN;
	out->boolean = boolean_value(in[0]);

	cond_result_table_free(in, in_len);
	return out;

fail:
	cond_result_free(out);
	cond_result_table_free(in, in_len);
	return NULL;
}

static struct cond_result *
eval_or(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	struct cond_result *out = NULL;
	size_t i;

	(void)pstate;

	if (in_len < 2) {
		EC_LOG(LOG_ERR, "or() takes at least two arguments\n");
		errno = EINVAL;
		goto fail;
	}

	/* return the first true element, or the last one */
	for (i = 0; i < in_len; i++) {
		if (boolean_value(in[i]))
			break;
	}
	if (i == in_len)
		i--;

	out = in[i];
	in[i] = NULL;

	cond_result_table_free(in, in_len);
	return out;

fail:
	cond_result_free(out);
	cond_result_table_free(in, in_len);
	return NULL;
}

static struct cond_result *
eval_and(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	struct cond_result *out = NULL;
	size_t i;

	(void)pstate;

	if (in_len < 2) {
		EC_LOG(LOG_ERR, "or() takes at least two arguments\n");
		errno = EINVAL;
		goto fail;
	}

	/* return the first false element, or the last one */
	for (i = 0; i < in_len; i++) {
		if (!boolean_value(in[i]))
			break;
	}
	if (i == in_len)
		i--;

	out = in[i];
	in[i] = NULL;

	cond_result_table_free(in, in_len);
	return out;

fail:
	cond_result_free(out);
	cond_result_table_free(in, in_len);
	return NULL;
}

static struct cond_result *
eval_first_child(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	struct cond_result *out = NULL;
	struct ec_htable_elt_ref *iter;
	const struct ec_pnode *const *pparse;
	struct ec_pnode *parse;

	(void)pstate;

	if (in_len != 1 || in[0]->type != NODESET) {
		EC_LOG(LOG_ERR, "first_child() takes one argument of type nodeset.\n");
		errno = EINVAL;
		goto fail;
	}

	out = malloc(sizeof(*out));
	if (out == NULL)
		goto fail;

	out->type = NODESET;
	out->htable = ec_htable();
	if (out->htable == NULL)
		goto fail;

	for (iter = ec_htable_iter(in[0]->htable); iter != NULL; iter = ec_htable_iter_next(iter)) {
		pparse = ec_htable_iter_get_key(iter);
		parse = ec_pnode_get_first_child(*pparse);
		if (parse == NULL)
			continue;
		if (ec_htable_set(out->htable, &parse, sizeof(parse), NULL, NULL) < 0)
			goto fail;
	}

	cond_result_table_free(in, in_len);
	return out;

fail:
	cond_result_free(out);
	cond_result_table_free(in, in_len);
	return NULL;
}

static struct cond_result *
eval_find(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	struct cond_result *out = NULL;
	struct ec_htable_elt_ref *iter;
	struct ec_pnode *const *pparse;
	const struct ec_pnode *parse;
	const char *id;

	(void)pstate;

	if (in_len != 2 || in[0]->type != NODESET || in[1]->type != STR) {
		EC_LOG(LOG_ERR, "find() takes two arguments (nodeset, str).\n");
		errno = EINVAL;
		goto fail;
	}

	out = malloc(sizeof(*out));
	if (out == NULL)
		goto fail;

	out->type = NODESET;
	out->htable = ec_htable();
	if (out->htable == NULL)
		goto fail;

	id = in[1]->str;
	for (iter = ec_htable_iter(in[0]->htable); iter != NULL; iter = ec_htable_iter_next(iter)) {
		pparse = ec_htable_iter_get_key(iter);
		parse = ec_pnode_find(*pparse, id);
		while (parse != NULL) {
			if (ec_htable_set(out->htable, &parse, sizeof(parse), NULL, NULL) < 0)
				goto fail;
			parse = ec_pnode_find_next(*pparse, parse, id, 1);
		}
	}

	cond_result_table_free(in, in_len);
	return out;

fail:
	cond_result_free(out);
	cond_result_table_free(in, in_len);
	return NULL;
}

static struct cond_result *
eval_cmp(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	struct cond_result *out = NULL;
	struct ec_htable_elt_ref *iter;
	bool eq = false, gt = false;

	(void)pstate;

	if (in_len != 3 || in[0]->type != STR || in[1]->type != in[2]->type) {
		EC_LOG(LOG_ERR, "cmp() takes 3 arguments (str, <type>, <type>).\n");
		errno = EINVAL;
		goto fail;
	}

	if (strcmp(in[0]->str, "eq") && strcmp(in[0]->str, "ne") && strcmp(in[0]->str, "gt")
	    && strcmp(in[0]->str, "lt") && strcmp(in[0]->str, "ge") && strcmp(in[0]->str, "le")) {
		EC_LOG(LOG_ERR, "invalid comparison operator in cmp().\n");
		errno = EINVAL;
		goto fail;
	}

	if (strcmp(in[0]->str, "eq") && strcmp(in[0]->str, "ne") && in[1]->type != INT) {
		EC_LOG(LOG_ERR, "cmp(gt|lt|ge|le, ...) is only allowed with integers.\n");
		errno = EINVAL;
		goto fail;
	}

	if (in[1]->type == INT) {
		eq = in[1]->int64 == in[2]->int64;
		gt = in[1]->int64 > in[2]->int64;
	} else if (in[1]->type == NODESET
		   && ec_htable_len(in[1]->htable) != ec_htable_len(in[2]->htable)) {
		eq = false;
	} else if (in[1]->type == NODESET) {
		eq = true;
		for (iter = ec_htable_iter(in[1]->htable); iter != NULL;
		     iter = ec_htable_iter_next(iter)) {
			if (ec_htable_get(
				    in[2]->htable,
				    ec_htable_iter_get_key(iter),
				    sizeof(struct ec_pnode *)
			    )
			    == NULL) {
				eq = false;
				break;
			}
		}
	} else if (in[1]->type == STR) {
		eq = !strcmp(in[1]->str, in[2]->str);
	} else if (in[1]->type == BOOLEAN) {
		eq = in[1]->boolean == in[2]->boolean;
	}

	out = malloc(sizeof(*out));
	if (out == NULL)
		goto fail;

	out->type = BOOLEAN;
	if (!strcmp(in[0]->str, "eq"))
		out->boolean = eq;
	else if (!strcmp(in[0]->str, "ne"))
		out->boolean = !eq;
	else if (!strcmp(in[0]->str, "lt"))
		out->boolean = !gt && !eq;
	else if (!strcmp(in[0]->str, "gt"))
		out->boolean = gt && !eq;
	else if (!strcmp(in[0]->str, "le"))
		out->boolean = !gt || eq;
	else if (!strcmp(in[0]->str, "ge"))
		out->boolean = gt || eq;

	cond_result_table_free(in, in_len);
	return out;

fail:
	cond_result_free(out);
	cond_result_table_free(in, in_len);
	return NULL;
}

static struct cond_result *
eval_count(const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	struct cond_result *out = NULL;

	(void)pstate;

	if (in_len != 1 || in[0]->type != NODESET) {
		EC_LOG(LOG_ERR, "count() takes one argument of type nodeset.\n");
		errno = EINVAL;
		goto fail;
	}

	out = malloc(sizeof(*out));
	if (out == NULL)
		goto fail;

	out->type = INT;
	out->int64 = ec_htable_len(in[0]->htable);

	cond_result_table_free(in, in_len);
	return out;

fail:
	cond_result_free(out);
	cond_result_table_free(in, in_len);
	return NULL;
}

static struct cond_result *
eval_func(const char *name, const struct ec_pnode *pstate, struct cond_result **in, size_t in_len)
{
	cond_func_t *f;

	f = ec_dict_get(ec_node_cond_functions, name);
	if (f == NULL) {
		EC_LOG(LOG_ERR, "No such function <%s>\n", name);
		errno = ENOENT;
		cond_result_table_free(in, in_len);
		return NULL;
	}

	return f(pstate, in, in_len);
}

static struct cond_result *
eval_condition(const struct ec_pnode *cond, const struct ec_pnode *pstate)
{
	const struct ec_pnode *iter;
	struct cond_result *res = NULL;
	struct cond_result **args = NULL;
	const struct ec_pnode *func = NULL, *func_name = NULL, *arg_list = NULL;
	const struct ec_pnode *value = NULL;
	const char *id;
	size_t n_arg = 0;

	// XXX fix cast (x3)
	func = ec_pnode_find((void *)cond, "id_function");
	if (func != NULL) {
		EC_PNODE_FOREACH_CHILD (iter, func) {
			id = ec_node_id(ec_pnode_get_node(iter));
			if (!strcmp(id, "id_function_name"))
				func_name = iter;
			if (!strcmp(id, "id_arg_list"))
				arg_list = iter;
		}

		iter = ec_pnode_find((void *)arg_list, "id_arg");
		while (iter != NULL) {
			args = realloc(args, (n_arg + 1) * sizeof(*args));
			args[n_arg] = eval_condition(iter, pstate);
			if (args[n_arg] == NULL)
				goto fail;
			n_arg++;
			iter = ec_pnode_find_next((void *)arg_list, (void *)iter, "id_arg", 0);
		}

		res = eval_func(
			ec_strvec_val(ec_pnode_get_strvec(func_name), 0), pstate, args, n_arg
		);
		args = NULL;
		return res;
	}

	value = ec_pnode_find((void *)cond, "id_value_str");
	if (value != NULL) {
		res = malloc(sizeof(*res));
		if (res == NULL)
			goto fail;
		res->type = STR;
		res->str = strdup(ec_strvec_val(ec_pnode_get_strvec(value), 0));
		if (res->str == NULL)
			goto fail;
		return res;
	}

	value = ec_pnode_find((void *)cond, "id_value_int");
	if (value != NULL) {
		res = malloc(sizeof(*res));
		if (res == NULL)
			goto fail;
		res->type = INT;
		if (ec_str_parse_llint(
			    ec_strvec_val(ec_pnode_get_strvec(value), 0),
			    0,
			    LLONG_MIN,
			    LLONG_MAX,
			    &res->int64
		    )
		    < 0)
			goto fail;
		return res;
	}

fail:
	cond_result_free(res);
	cond_result_table_free(args, n_arg);
	return NULL;
}

static int validate_condition(const struct ec_pnode *cond, const struct ec_pnode *pstate)
{
	struct cond_result *res;
	int ret;

	res = eval_condition(cond, pstate);
	if (res == NULL)
		return -1;

	ret = boolean_value(res);
	cond_result_free(res);

	return ret;
}

static int ec_node_cond_parse(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
)
{
	struct ec_node_cond *priv = ec_node_priv(node);
	struct ec_pnode *child;
	int ret, valid;

	ret = ec_parse_child(priv->child, pstate, strvec);
	if (ret <= 0)
		return ret;

	valid = validate_condition(priv->parsed_cond, pstate);
	if (valid < 0)
		return valid;

	if (valid == 0) {
		child = ec_pnode_get_last_child(pstate);
		ec_pnode_unlink_child(child);
		ec_pnode_free(child);
		return EC_PARSE_NOMATCH;
	}

	return ret;
}

static int ec_node_cond_complete(
	const struct ec_node *node,
	struct ec_comp *comp,
	const struct ec_strvec *strvec
)
{
	struct ec_node_cond *priv = ec_node_priv(node);

	// XXX eval condition
	// XXX before or after completing ? configurable ?

	return ec_complete_child(priv->child, comp, strvec);
}

static void ec_node_cond_free_priv(struct ec_node *node)
{
	struct ec_node_cond *priv = ec_node_priv(node);

	free(priv->cond_str);
	priv->cond_str = NULL;
	ec_pnode_free(priv->parsed_cond);
	priv->parsed_cond = NULL;
	ec_node_free(priv->child);
}

static const struct ec_config_schema ec_node_cond_schema[] = {
	{
		.key = "expr",
		.desc = "XXX",
		.type = EC_CONFIG_TYPE_STRING,
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

static int ec_node_cond_set_config(struct ec_node *node, const struct ec_config *config)
{
	struct ec_node_cond *priv = ec_node_priv(node);
	const struct ec_config *cond = NULL;
	struct ec_pnode *parsed_cond = NULL;
	const struct ec_config *child;
	char *cond_str = NULL;

	cond = ec_config_dict_get(config, "expr");
	if (cond == NULL) {
		errno = EINVAL;
		goto fail;
	}

	cond_str = strdup(cond->string);
	if (cond_str == NULL)
		goto fail;

	child = ec_config_dict_get(config, "child");
	if (child == NULL)
		goto fail;

	/* parse expression to build the cmd child node */
	parsed_cond = ec_node_cond_build(cond_str);
	if (parsed_cond == NULL)
		goto fail;

	/* ok, store the config */
	ec_pnode_free(priv->parsed_cond);
	priv->parsed_cond = parsed_cond;
	free(priv->cond_str);
	priv->cond_str = cond_str;
	ec_node_free(priv->child);
	priv->child = ec_node_clone(child->node);

	return 0;

fail:
	ec_pnode_free(parsed_cond);
	free(cond_str);
	return -1;
}

static size_t ec_node_cond_get_children_count(const struct ec_node *node)
{
	struct ec_node_cond *priv = ec_node_priv(node);

	if (priv->child == NULL)
		return 0;
	return 1;
}

static int ec_node_cond_get_child(
	const struct ec_node *node,
	size_t i,
	struct ec_node **child,
	unsigned int *refs
)
{
	struct ec_node_cond *priv = ec_node_priv(node);

	if (i > 0)
		return -1;

	*child = priv->child;
	*refs = 1;
	return 0;
}

static struct ec_node_type ec_node_cond_type = {
	.name = "cond",
	.schema = ec_node_cond_schema,
	.set_config = ec_node_cond_set_config,
	.parse = ec_node_cond_parse,
	.complete = ec_node_cond_complete,
	.size = sizeof(struct ec_node_cond),
	.free_priv = ec_node_cond_free_priv,
	.get_children_count = ec_node_cond_get_children_count,
	.get_child = ec_node_cond_get_child,
};

EC_NODE_TYPE_REGISTER(ec_node_cond_type);

struct ec_node *ec_node_cond(const char *id, const char *cmd, struct ec_node *child)
{
	struct ec_config *config = NULL;
	struct ec_node *node = NULL;
	int ret;

	if (child == NULL)
		return NULL;

	node = ec_node_from_type(&ec_node_cond_type, id);
	if (node == NULL)
		goto fail;

	config = ec_config_dict();
	if (config == NULL)
		goto fail;

	if (ec_config_dict_set(config, "expr", ec_config_string(cmd)) < 0)
		goto fail;

	if (ec_config_dict_set(config, "child", ec_config_node(child)) < 0) {
		child = NULL; /* freed */
		goto fail;
	}
	child = NULL;

	ret = ec_node_set_config(node, config);
	config = NULL; /* freed */
	if (ret < 0)
		goto fail;

	return node;

fail:
	ec_node_free(node);
	ec_node_free(child);
	ec_config_free(config);

	return NULL;
}

static void ec_node_cond_exit_func(void)
{
	ec_node_free(ec_node_cond_parser);
	ec_node_cond_parser = NULL;
	ec_dict_free(ec_node_cond_functions);
	ec_node_cond_functions = NULL;
}

static int add_func(const char *name, cond_func_t *f)
{
	return ec_dict_set(ec_node_cond_functions, name, f, NULL);
}

static int ec_node_cond_init_func(void)
{
	ec_node_cond_parser = ec_node_cond_build_parser();
	if (ec_node_cond_parser == NULL)
		goto fail;

	ec_node_cond_functions = ec_dict();
	if (ec_node_cond_functions == NULL)
		goto fail;

	if (add_func("root", eval_root) < 0)
		goto fail;
	if (add_func("current", eval_current) < 0)
		goto fail;
	if (add_func("bool", eval_bool) < 0)
		goto fail;
	if (add_func("or", eval_or) < 0)
		goto fail;
	if (add_func("and", eval_and) < 0)
		goto fail;
	if (add_func("first_child", eval_first_child) < 0)
		goto fail;
	if (add_func("find", eval_find) < 0)
		goto fail;
	if (add_func("cmp", eval_cmp) < 0)
		goto fail;
	if (add_func("count", eval_count) < 0)
		goto fail;

	return 0;

fail:
	EC_LOG(EC_LOG_ERR, "Failed to initialize condition parser\n");
	ec_node_cond_exit_func();
	return -1;
}

static struct ec_init ec_node_cond_init = {
	.init = ec_node_cond_init_func,
	.exit = ec_node_cond_exit_func,
	.priority = 75,
};

EC_INIT_REGISTER(ec_node_cond_init);
