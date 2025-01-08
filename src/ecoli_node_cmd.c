/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <ecoli_init.h>
#include <ecoli_malloc.h>
#include <ecoli_log.h>
#include <ecoli_test.h>
#include <ecoli_strvec.h>
#include <ecoli_node.h>
#include <ecoli_config.h>
#include <ecoli_parse.h>
#include <ecoli_complete.h>
#include <ecoli_node_helper.h>
#include <ecoli_node_expr.h>
#include <ecoli_node_str.h>
#include <ecoli_node_or.h>
#include <ecoli_node_subset.h>
#include <ecoli_node_int.h>
#include <ecoli_node_many.h>
#include <ecoli_node_seq.h>
#include <ecoli_node_option.h>
#include <ecoli_node_re.h>
#include <ecoli_node_re_lex.h>
#include <ecoli_node_cmd.h>

EC_LOG_TYPE_REGISTER(node_cmd);

static struct ec_node *ec_node_cmd_parser; /* the expression parser. */
static struct ec_node *ec_node_cmd_expr;   /* the expr parser without lexer. */

struct ec_node_cmd {
	char *cmd_str;           /* the command string. */
	struct ec_node *cmd;     /* the command node. */
	struct ec_node **table;  /* table of node referenced in command. */
	unsigned int len;        /* len of the table. */
};

/* passed as user context to expression parser */
struct ec_node_cmd_ctx {
	struct ec_node **table;
	unsigned int len;
};

static int
ec_node_cmd_eval_var(void **result, void *userctx,
	const struct ec_pnode *var)
{
	const struct ec_strvec *vec;
	struct ec_node_cmd_ctx *ctx = userctx;
	struct ec_node *eval = NULL;
	const char *str, *id;
	unsigned int i;

	/* get parsed string vector, it should contain only one str */
	vec = ec_pnode_get_strvec(var);
	if (ec_strvec_len(vec) != 1) {
		errno = EINVAL;
		return -1;
	}
	str = ec_strvec_val(vec, 0);

	for (i = 0; i < ctx->len; i++) {
		id = ec_node_id(ctx->table[i]);
		if (id == NULL)
			continue;
		if (strcmp(str, id))
			continue;
		/* if id matches, use a node provided by the user... */
		eval = ec_node_clone(ctx->table[i]);
		if (eval == NULL)
			return -1;
		break;
	}

	/* ...or create a string node */
	if (eval == NULL) {
		eval = ec_node_str(EC_NO_ID, str);
		if (eval == NULL)
			return -1;
	}

	*result = eval;

	return 0;
}

static int
ec_node_cmd_eval_pre_op(void **result, void *userctx, void *operand,
	const struct ec_pnode *operator)
{
	(void)result;
	(void)userctx;
	(void)operand;
	(void)operator;

	errno = EINVAL;
	return -1;
}

static int
ec_node_cmd_eval_post_op(void **result, void *userctx, void *operand,
	const struct ec_pnode *operator)
{
	const struct ec_strvec *vec;
	struct ec_node *in = operand;;
	struct ec_node *out = NULL;;

	(void)userctx;

	/* get parsed string vector, it should contain only one str */
	vec = ec_pnode_get_strvec(operator);
	if (ec_strvec_len(vec) != 1) {
		errno = EINVAL;
		return -1;
	}

	if (!strcmp(ec_strvec_val(vec, 0), "*")) {
		out = ec_node_many(EC_NO_ID,
				ec_node_clone(in), 0, 0);
		if (out == NULL)
			return -1;
		ec_node_free(in);
		*result = out;
	} else if (!strcmp(ec_strvec_val(vec, 0), "+")) {
		out = ec_node_many(EC_NO_ID,
				ec_node_clone(in), 1, 0);
		if (out == NULL)
			return -1;
		ec_node_free(in);
		*result = out;
	} else {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int
ec_node_cmd_eval_bin_op(void **result, void *userctx, void *operand1,
	const struct ec_pnode *operator, void *operand2)

{
	const struct ec_strvec *vec;
	struct ec_node *out = NULL;
	struct ec_node *in1 = operand1;
	struct ec_node *in2 = operand2;

	(void)userctx;

	/* get parsed string vector, it should contain only one str */
	vec = ec_pnode_get_strvec(operator);
	if (ec_strvec_len(vec) > 1) {
		errno = EINVAL;
		return -1;
	}

	if (ec_strvec_len(vec) == 0) {
		if (!strcmp(ec_node_get_type_name(in1), "seq")) {
			if (ec_node_seq_add(in1, ec_node_clone(in2)) < 0)
				return -1;
			ec_node_free(in2);
			*result = in1;
		} else {
			out = EC_NODE_SEQ(EC_NO_ID, ec_node_clone(in1),
					ec_node_clone(in2));
			if (out == NULL)
				return -1;
			ec_node_free(in1);
			ec_node_free(in2);
			*result = out;
		}
	} else if (!strcmp(ec_strvec_val(vec, 0), "|")) {
		if (!strcmp(ec_node_get_type_name(in2), "or")) {
			if (ec_node_or_add(in2, ec_node_clone(in1)) < 0)
				return -1;
			ec_node_free(in1);
			*result = in2;
		} else if (!strcmp(ec_node_get_type_name(in1), "or")) {
			if (ec_node_or_add(in1, ec_node_clone(in2)) < 0)
				return -1;
			ec_node_free(in2);
			*result = in1;
		} else {
			out = EC_NODE_OR(EC_NO_ID, ec_node_clone(in1),
					ec_node_clone(in2));
			if (out == NULL)
				return -1;
			ec_node_free(in1);
			ec_node_free(in2);
			*result = out;
		}
	} else if (!strcmp(ec_strvec_val(vec, 0), ",")) {
		if (!strcmp(ec_node_get_type_name(in2), "subset")) {
			if (ec_node_subset_add(in2, ec_node_clone(in1)) < 0)
				return -1;
			ec_node_free(in1);
			*result = in2;
		} else if (!strcmp(ec_node_get_type_name(in1), "subset")) {
			if (ec_node_subset_add(in1, ec_node_clone(in2)) < 0)
				return -1;
			ec_node_free(in2);
			*result = in1;
		} else {
			out = EC_NODE_SUBSET(EC_NO_ID, ec_node_clone(in1),
					ec_node_clone(in2));
			if (out == NULL)
				return -1;
			ec_node_free(in1);
			ec_node_free(in2);
			*result = out;
		}
	} else {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int
ec_node_cmd_eval_parenthesis(void **result, void *userctx,
	const struct ec_pnode *open_paren,
	const struct ec_pnode *close_paren,
	void *value)
{
	const struct ec_strvec *vec;
	struct ec_node *in = value;;
	struct ec_node *out = NULL;;

	(void)userctx;
	(void)close_paren;

	/* get parsed string vector, it should contain only one str */
	vec = ec_pnode_get_strvec(open_paren);
	if (ec_strvec_len(vec) != 1) {
		errno = EINVAL;
		return -1;
	}

	if (!strcmp(ec_strvec_val(vec, 0), "[")) {
		out = ec_node_option(EC_NO_ID, ec_node_clone(in));
		if (out == NULL)
			return -1;
		ec_node_free(in);
	} else if (!strcmp(ec_strvec_val(vec, 0), "(")) {
		out = in;
	} else {
		errno = EINVAL;
		return -1;
	}

	*result = out;

	return 0;
}

static void
ec_node_cmd_eval_free(void *result, void *userctx)
{
	(void)userctx;
	ec_free(result);
}

static const struct ec_node_expr_eval_ops expr_ops = {
	.eval_var = ec_node_cmd_eval_var,
	.eval_pre_op = ec_node_cmd_eval_pre_op,
	.eval_post_op = ec_node_cmd_eval_post_op,
	.eval_bin_op = ec_node_cmd_eval_bin_op,
	.eval_parenthesis = ec_node_cmd_eval_parenthesis,
	.eval_free = ec_node_cmd_eval_free,
};

static struct ec_node *
ec_node_cmd_build_expr(void)
{
	struct ec_node *expr = NULL;
	int ret;

	/* build the expression parser */
	expr = ec_node("expr", "expr");
	if (expr == NULL)
		goto fail;
	ret = ec_node_expr_set_val_node(expr, ec_node_re(EC_NO_ID,
					"[a-zA-Z0-9_-]+"));
	if (ret < 0)
		goto fail;
	ret = ec_node_expr_add_bin_op(expr, ec_node_str(EC_NO_ID, ","));
	if (ret < 0)
		goto fail;
	ret = ec_node_expr_add_bin_op(expr, ec_node_str(EC_NO_ID, "|"));
	if (ret < 0)
		goto fail;
	ret = ec_node_expr_add_bin_op(expr, ec_node("empty", EC_NO_ID));
	if (ret < 0)
		goto fail;
	ret = ec_node_expr_add_post_op(expr, ec_node_str(EC_NO_ID, "+"));
	if (ret < 0)
		goto fail;
	ret = ec_node_expr_add_post_op(expr, ec_node_str(EC_NO_ID, "*"));
	if (ret < 0)
		goto fail;
	ret = ec_node_expr_add_parenthesis(expr, ec_node_str(EC_NO_ID, "["),
		ec_node_str(EC_NO_ID, "]"));
	if (ret < 0)
		goto fail;
	ec_node_expr_add_parenthesis(expr, ec_node_str(EC_NO_ID, "("),
		ec_node_str(EC_NO_ID, ")"));
	if (ret < 0)
		goto fail;

	return expr;

fail:
	ec_node_free(expr);
	return NULL;
}

static struct ec_node *
ec_node_cmd_build_parser(struct ec_node *expr)
{
	struct ec_node *lex = NULL;
	int ret;

	/* prepend a lexer to the expression node */
	lex = ec_node_re_lex(EC_NO_ID, ec_node_clone(expr));
	if (lex == NULL)
		goto fail;

	ret = ec_node_re_lex_add(lex, "[a-zA-Z0-9_-]+", 1, NULL);
	if (ret < 0)
		goto fail;
	ret = ec_node_re_lex_add(lex, "[*+|,()]", 1, NULL);
	if (ret < 0)
		goto fail;
	ret = ec_node_re_lex_add(lex, "\\[", 1, NULL);
	if (ret < 0)
		goto fail;
	ret = ec_node_re_lex_add(lex, "\\]", 1, NULL);
	if (ret < 0)
		goto fail;
	ret = ec_node_re_lex_add(lex, "[	 ]+", 0, NULL);
	if (ret < 0)
		goto fail;

	return lex;

fail:
	ec_node_free(lex);

	return NULL;
}

static struct ec_node *
ec_node_cmd_build(const char *cmd_str, struct ec_node **table, size_t len)
{
	struct ec_node_cmd_ctx ctx = { table, len };
	struct ec_pnode *p = NULL;
	void *result;
	int ret;

	/* parse the command expression */
	p = ec_parse(ec_node_cmd_parser, cmd_str);
	if (p == NULL)
		goto fail;

	if (!ec_pnode_matches(p)) {
		errno = EINVAL;
		goto fail;
	}

	ret = ec_node_expr_eval(&result, ec_node_cmd_expr,
				ec_pnode_get_first_child(p),
				&expr_ops, &ctx);
	if (ret < 0)
		goto fail;

	ec_pnode_free(p);
	return result;

fail:
	ec_pnode_free(p);
	return NULL;
}

static int
ec_node_cmd_parse(const struct ec_node *node, struct ec_pnode *pstate,
		const struct ec_strvec *strvec)
{
	struct ec_node_cmd *priv = ec_node_priv(node);

	return ec_parse_child(priv->cmd, pstate, strvec);
}

static int
ec_node_cmd_complete(const struct ec_node *node,
		struct ec_comp *comp,
		const struct ec_strvec *strvec)
{
	struct ec_node_cmd *priv = ec_node_priv(node);

	return ec_complete_child(priv->cmd, comp, strvec);
}

static void ec_node_cmd_free_priv(struct ec_node *node)
{
	struct ec_node_cmd *priv = ec_node_priv(node);
	size_t i;

	ec_free(priv->cmd_str);
	priv->cmd_str = NULL;
	ec_node_free(priv->cmd);
	priv->cmd = NULL;
	for (i = 0; i < priv->len; i++)
		ec_node_free(priv->table[i]);
	ec_free(priv->table);
	priv->table = NULL;
	priv->len = 0;
}

static const struct ec_config_schema ec_node_cmd_subschema[] = {
	{
		.desc = "A child node whose id is referenced in the expression.",
		.type = EC_CONFIG_TYPE_NODE,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static const struct ec_config_schema ec_node_cmd_schema[] = {
	{
		.key = "expr",
		.desc = "The expression to match. Supported operators "
		"are or '|', list ',', many '+', many-or-zero '*', "
		"option '[]', group '()'. An identifier (alphanumeric) can "
		"reference a node whose node_id matches. Else it is "
		"interpreted as ec_node_str() matching this string. "
		"Example: command [option] (subset1, subset2) x|y",
		.type = EC_CONFIG_TYPE_STRING,
	},
	{
		.key = "children",
		.desc = "The list of children nodes.",
		.type = EC_CONFIG_TYPE_LIST,
		.subschema = ec_node_cmd_subschema,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static int ec_node_cmd_set_config(struct ec_node *node,
				const struct ec_config *config)
{
	struct ec_node_cmd *priv = ec_node_priv(node);
	const struct ec_config *expr = NULL;
	struct ec_node *cmd = NULL;
	struct ec_node **table = NULL;
	char *cmd_str = NULL;
	size_t len = 0, i;

	/* retrieve config locally */
	expr = ec_config_dict_get(config, "expr");
	if (expr == NULL) {
		errno = EINVAL;
		goto fail;
	}

	table = ec_node_config_node_list_to_table(
		ec_config_dict_get(config, "children"), &len);
	if (table == NULL)
		goto fail;

	cmd_str = ec_strdup(expr->string);
	if (cmd_str == NULL)
		goto fail;

	/* parse expression to build the cmd child node */
	cmd = ec_node_cmd_build(cmd_str, table, len);
	if (cmd == NULL)
		goto fail;

	/* ok, store the config */
	ec_node_free(priv->cmd);
	priv->cmd = cmd;
	ec_free(priv->cmd_str);
	priv->cmd_str = cmd_str;
	for (i = 0; i < priv->len; i++)
		ec_node_free(priv->table[i]);
	ec_free(priv->table);
	priv->table = table;
	priv->len = len;

	return 0;

fail:
	if (table != NULL) {
		for (i = 0; i < len; i++)
			ec_node_free(table[i]);
	}
	ec_free(table);
	ec_free(cmd_str);
	ec_node_free(cmd);
	return -1;
}

static size_t
ec_node_cmd_get_children_count(const struct ec_node *node)
{
	struct ec_node_cmd *priv = ec_node_priv(node);

	if (priv->cmd == NULL)
		return 0;
	return 1;
}

static int
ec_node_cmd_get_child(const struct ec_node *node, size_t i,
		struct ec_node **child, unsigned int *refs)
{
	struct ec_node_cmd *priv = ec_node_priv(node);

	if (i > 0)
		return -1;

	*child = priv->cmd;
	*refs = 1;
	return 0;
}

static struct ec_node_type ec_node_cmd_type = {
	.name = "cmd",
	.schema = ec_node_cmd_schema,
	.set_config = ec_node_cmd_set_config,
	.parse = ec_node_cmd_parse,
	.complete = ec_node_cmd_complete,
	.size = sizeof(struct ec_node_cmd),
	.free_priv = ec_node_cmd_free_priv,
	.get_children_count = ec_node_cmd_get_children_count,
	.get_child = ec_node_cmd_get_child,
};

EC_NODE_TYPE_REGISTER(ec_node_cmd_type);

struct ec_node *__ec_node_cmd(const char *id, const char *cmd, ...)
{
	struct ec_config *config = NULL, *children = NULL;
	struct ec_node *node = NULL;
	va_list ap;
	int ret;

	/* this block must stay first, it frees the nodes on error */
	va_start(ap, cmd);
	children = ec_node_config_node_list_from_vargs(ap);
	va_end(ap);
	if (children == NULL)
		goto fail;

	node = ec_node_from_type(&ec_node_cmd_type, id);
	if (node == NULL)
		goto fail;

	config = ec_config_dict();
	if (config == NULL)
		goto fail;

	if (ec_config_dict_set(config, "expr", ec_config_string(cmd)) < 0)
		goto fail;

	if (ec_config_dict_set(config, "children", children) < 0) {
		children = NULL; /* freed */
		goto fail;
	}
	children = NULL;

	ret = ec_node_set_config(node, config);
	config = NULL; /* freed */
	if (ret < 0)
		goto fail;

	return node;

fail:
	ec_node_free(node); /* will also free added children */
	ec_config_free(children);
	ec_config_free(config);

	return NULL;
}

static int ec_node_cmd_init_func(void)
{
	ec_node_cmd_expr = ec_node_cmd_build_expr();
	if (ec_node_cmd_expr == NULL)
		goto fail;

	ec_node_cmd_parser = ec_node_cmd_build_parser(ec_node_cmd_expr);
	if (ec_node_cmd_parser == NULL)
		goto fail;

	return 0;

fail:
	EC_LOG(EC_LOG_ERR, "Failed to initialize command parser\n");
	ec_node_free(ec_node_cmd_expr);
	ec_node_cmd_expr = NULL;
	ec_node_free(ec_node_cmd_parser);
	ec_node_cmd_parser = NULL;
	return -1;
}

static void ec_node_cmd_exit_func(void)
{
	ec_node_free(ec_node_cmd_expr);
	ec_node_cmd_expr = NULL;
	ec_node_free(ec_node_cmd_parser);
	ec_node_cmd_parser = NULL;
}

static struct ec_init ec_node_cmd_init = {
	.init = ec_node_cmd_init_func,
	.exit = ec_node_cmd_exit_func,
	.priority = 75,
};

EC_INIT_REGISTER(ec_node_cmd_init);

/* LCOV_EXCL_START */
static int ec_node_cmd_testcase(void)
{
	struct ec_node *node;
	int testres = 0;

	node = EC_NODE_CMD(EC_NO_ID,
		"command [option] (subset1, subset2, subset3, subset4) x|y z*",
		ec_node_int("x", 0, 10, 10),
		ec_node_int("y", 20, 30, 10)
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 2, "command", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "command", "subset1", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 4, "command", "subset3", "subset2",
				"1");
	testres |= EC_TEST_CHECK_PARSE(node, 5, "command", "subset2", "subset3",
				"subset1", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 6, "command", "subset3", "subset1",
				"subset4", "subset2", "4");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "command", "23");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "command", "option", "23");
	testres |= EC_TEST_CHECK_PARSE(node, 5, "command", "option", "23",
				"z", "z");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "command", "15");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foo");
	ec_node_free(node);

	node = EC_NODE_CMD(EC_NO_ID, "good morning [count] bob|bobby|michael",
			ec_node_int("count", 0, 10, 10));
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 4, "good", "morning", "1", "bob");

	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_VA_END,
		"good", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"g", EC_VA_END,
		"good", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"good", "morning", "", EC_VA_END,
		"bob", "bobby", "michael", EC_VA_END);

	ec_node_free(node);

	node = EC_NODE_CMD(EC_NO_ID, "[foo [bar]]");
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 0);
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 0, "x");
	ec_node_free(node);

	return testres;
}

static struct ec_test ec_node_cmd_test = {
	.name = "node_cmd",
	.test = ec_node_cmd_testcase,
};

EC_TEST_REGISTER(ec_node_cmd_test);
/* LCOV_EXCL_STOP */
