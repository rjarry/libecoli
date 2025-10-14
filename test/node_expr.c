/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

EC_LOG_TYPE_REGISTER(node_expr);

struct my_eval_result {
	int val;
};

static int
ec_node_expr_test_eval_var(void **result, void *userctx,
	const struct ec_pnode *var)
{
	const struct ec_strvec *vec;
	const struct ec_node *node;
	struct my_eval_result *eval = NULL;
	int64_t val;

	(void)userctx;

	/* get parsed string vector, it should contain only one str */
	vec = ec_pnode_get_strvec(var);
	if (ec_strvec_len(vec) != 1) {
		errno = EINVAL;
		return -1;
	}

	node = ec_pnode_get_node(var);
	if (ec_node_int_getval(node, ec_strvec_val(vec, 0), &val) < 0)
		return -1;

	eval = malloc(sizeof(*eval));
	if (eval == NULL)
		return -1;

	eval->val = val;
	EC_LOG(EC_LOG_DEBUG, "eval var %d\n", eval->val);
	*result = eval;

	return 0;
}

static int
ec_node_expr_test_eval_pre_op(void **result, void *userctx, void *operand,
	const struct ec_pnode *operator)
{
	const struct ec_strvec *vec;
	struct my_eval_result *eval = operand;;

	(void)userctx;

	/* get parsed string vector, it should contain only one str */
	vec = ec_pnode_get_strvec(operator);
	if (ec_strvec_len(vec) != 1) {
		errno = EINVAL;
		return -1;
	}

	if (!strcmp(ec_strvec_val(vec, 0), "!")) {
		eval->val = !eval->val;
	} else {
		errno = EINVAL;
		return -1;
	}


	EC_LOG(EC_LOG_DEBUG, "eval pre_op %d\n", eval->val);
	*result = eval;

	return 0;
}

static int
ec_node_expr_test_eval_post_op(void **result, void *userctx, void *operand,
	const struct ec_pnode *operator)
{
	const struct ec_strvec *vec;
	struct my_eval_result *eval = operand;;

	(void)userctx;

	/* get parsed string vector, it should contain only one str */
	vec = ec_pnode_get_strvec(operator);
	if (ec_strvec_len(vec) != 1) {
		errno = EINVAL;
		return -1;
	}

	if (!strcmp(ec_strvec_val(vec, 0), "^")) {
		eval->val = eval->val * eval->val;
	} else {
		errno = EINVAL;
		return -1;
	}

	EC_LOG(EC_LOG_DEBUG, "eval post_op %d\n", eval->val);
	*result = eval;

	return 0;
}

static int
ec_node_expr_test_eval_bin_op(void **result, void *userctx, void *operand1,
	const struct ec_pnode *operator, void *operand2)

{
	const struct ec_strvec *vec;
	struct my_eval_result *eval1 = operand1;;
	struct my_eval_result *eval2 = operand2;;

	(void)userctx;

	/* get parsed string vector, it should contain only one str */
	vec = ec_pnode_get_strvec(operator);
	if (ec_strvec_len(vec) != 1) {
		errno = EINVAL;
		return -1;
	}

	if (!strcmp(ec_strvec_val(vec, 0), "+")) {
		eval1->val = eval1->val + eval2->val;
	} else if (!strcmp(ec_strvec_val(vec, 0), "*")) {
		eval1->val = eval1->val * eval2->val;
	} else {
		errno = EINVAL;
		return -1;
	}

	EC_LOG(EC_LOG_DEBUG, "eval bin_op %d\n", eval1->val);
	free(eval2);
	*result = eval1;

	return 0;
}

static int
ec_node_expr_test_eval_parenthesis(void **result, void *userctx,
	const struct ec_pnode *open_paren,
	const struct ec_pnode *close_paren,
	void *value)
{
	(void)userctx;
	(void)open_paren;
	(void)close_paren;

	EC_LOG(EC_LOG_DEBUG, "eval paren\n");
	*result = value;

	return 0;
}

static void
ec_node_expr_test_eval_free(void *result, void *userctx)
{
	(void)userctx;
	free(result);
}

static const struct ec_node_expr_eval_ops test_ops = {
	.eval_var = ec_node_expr_test_eval_var,
	.eval_pre_op = ec_node_expr_test_eval_pre_op,
	.eval_post_op = ec_node_expr_test_eval_post_op,
	.eval_bin_op = ec_node_expr_test_eval_bin_op,
	.eval_parenthesis = ec_node_expr_test_eval_parenthesis,
	.eval_free = ec_node_expr_test_eval_free,
};

static int ec_node_expr_test_eval(struct ec_node *lex_node,
	const struct ec_node *expr_node,
	const char *str, int val)
{
	struct ec_pnode *p;
	void *result;
	struct my_eval_result *eval;
	int ret;

	p = ec_parse(lex_node, str);
	if (p == NULL)
		return -1;

	ret = ec_node_expr_eval(&result, expr_node, p, &test_ops, NULL);
	ec_pnode_free(p);
	if (ret < 0)
		return -1;

	/* the parse value is an integer */
	eval = result;
	assert(eval != NULL);

	EC_LOG(EC_LOG_DEBUG, "result: %d (expected %d)\n", eval->val, val);
	if (eval->val == val)
		ret = 0;
	else
		ret = -1;

	free(eval);

	return ret;
}

EC_TEST_MAIN() {
	struct ec_node *node = NULL, *lex_node = NULL;
	int testres = 0;

	node = ec_node("expr", "my_expr");
	if (node == NULL)
		return -1;

	ec_node_expr_set_val_node(node, ec_node_int(EC_NO_ID, 0, UCHAR_MAX, 0));
	ec_node_expr_add_bin_op(node, ec_node_str(EC_NO_ID, "+"));
	ec_node_expr_add_bin_op(node, ec_node_str(EC_NO_ID, "*"));
	ec_node_expr_add_pre_op(node, ec_node_str(EC_NO_ID, "!"));  /* not */
	ec_node_expr_add_post_op(node, ec_node_str(EC_NO_ID, "^")); /* square */
	ec_node_expr_add_parenthesis(node, ec_node_str(EC_NO_ID, "("),
		ec_node_str(EC_NO_ID, ")"));
	testres |= EC_TEST_CHECK_PARSE(node, 1, "1");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "1", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "1", "*");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "1", "*", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "1", "*", "1", "*");
	testres |= EC_TEST_CHECK_PARSE(node, 4, "1", "+", "!", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 4, "1", "^", "+", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 5, "1", "*", "1", "*", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 5, "1", "*", "1", "+", "1");
	testres |= EC_TEST_CHECK_PARSE(node, 7, "1", "*", "1", "*", "1", "*",
				"1");
	testres |= EC_TEST_CHECK_PARSE(
		node, 10, "!", "(", "1", "*", "(", "1", "+", "1", ")", ")");
	testres |= EC_TEST_CHECK_PARSE(node, 5, "1", "+", "!", "1", "^");

	/* prepend a lexer to the expression node */
	lex_node = ec_node_re_lex(EC_NO_ID, ec_node_clone(node));
	if (lex_node == NULL)
		goto fail;

	testres |= ec_node_re_lex_add(lex_node, "[0-9]+", 1, NULL); /* vars */
	testres |= ec_node_re_lex_add(lex_node, "[+*!^()]", 1, NULL); /* operators */
	testres |= ec_node_re_lex_add(lex_node, "[ 	]+", 0, NULL); /* spaces */

	/* valid expressions */
	testres |= EC_TEST_CHECK_PARSE(lex_node, 1, "!1");
	testres |= EC_TEST_CHECK_PARSE(lex_node, 1, "1^");
	testres |= EC_TEST_CHECK_PARSE(lex_node, 1, "1^ + 1");
	testres |= EC_TEST_CHECK_PARSE(lex_node, 1, "1 + 4 * (2 + 3^)^");
	testres |= EC_TEST_CHECK_PARSE(lex_node, 1, "(1)");
	testres |= EC_TEST_CHECK_PARSE(lex_node, 1, "3*!3+!3*(2+ 2)");
	testres |= EC_TEST_CHECK_PARSE(lex_node, 1, "!!(!1)^ + !(4 + (2*3))");
	testres |= EC_TEST_CHECK_PARSE(lex_node, 1, "(1 + 1)^ * 1^");

	/* invalid expressions */
	testres |= EC_TEST_CHECK_PARSE(lex_node, -1, "");
	testres |= EC_TEST_CHECK_PARSE(lex_node, -1, "()");
	testres |= EC_TEST_CHECK_PARSE(lex_node, -1, "(");
	testres |= EC_TEST_CHECK_PARSE(lex_node, -1, ")");
	testres |= EC_TEST_CHECK_PARSE(lex_node, -1, "+1");
	testres |= EC_TEST_CHECK_PARSE(lex_node, -1, "1+");
	testres |= EC_TEST_CHECK_PARSE(lex_node, -1, "1+*1");
	testres |= EC_TEST_CHECK_PARSE(lex_node, -1, "1+(1*1");
	testres |= EC_TEST_CHECK_PARSE(lex_node, -1, "1+!1!1)");

	testres |= ec_node_expr_test_eval(lex_node, node, "1^", 1);
	testres |= ec_node_expr_test_eval(lex_node, node, "2^", 4);
	testres |= ec_node_expr_test_eval(lex_node, node, "!1", 0);
	testres |= ec_node_expr_test_eval(lex_node, node, "!0", 1);

	testres |= ec_node_expr_test_eval(lex_node, node, "1+1", 2);
	testres |= ec_node_expr_test_eval(lex_node, node, "1+2+3", 6);
	testres |= ec_node_expr_test_eval(lex_node, node, "1+1*2", 4);
	testres |= ec_node_expr_test_eval(lex_node, node, "2 * 2^", 8);
	testres |= ec_node_expr_test_eval(lex_node, node, "(1 + !0)^ * !0^", 4);
	testres |= ec_node_expr_test_eval(lex_node, node, "(1 + !1) * 3", 3);

	ec_node_free(node);
	ec_node_free(lex_node);

	return testres;

fail:
	ec_node_free(lex_node);
	ec_node_free(node);
	return -1;
}
