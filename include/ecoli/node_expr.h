/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_node_expr Expression node
 * @ingroup ecoli_nodes
 * @{
 *
 * @brief A node for parsing and evaluating expressions with operators.
 */

#pragma once

#include <ecoli/node.h>

/**
 * Callback function type for evaluating a variable
 *
 * @param result
 *   On success, this pointer must be set by the user to point
 *   to a user structure describing the evaluated result.
 * @param userctx
 *   A user-defined context passed to all callback functions, which
 *   can be used to maintain a state or store global information.
 * @param var
 *   The parse result referencing the variable.
 * @return
 *   0 on success (*result must be set), or -errno on error (*result
 *   is undefined).
 */
typedef int (*ec_node_expr_eval_var_t)(void **result, void *userctx, const struct ec_pnode *var);

/**
 * Callback function type for evaluating a prefix-operator
 *
 * @param result
 *   On success, this pointer must be set by the user to point
 *   to a user structure describing the evaluated result.
 * @param userctx
 *   A user-defined context passed to all callback functions, which
 *   can be used to maintain a state or store global information.
 * @param operand
 *   The evaluated expression on which the operation should be applied.
 * @param operator
 *   The parse result referencing the operator.
 * @return
 *   0 on success (*result must be set, operand is freed),
 *   or -errno on error (*result is undefined, operand is not freed).
 */
typedef int (*ec_node_expr_eval_pre_op_t)(
	void **result,
	void *userctx,
	void *operand,
	const struct ec_pnode *operator
);

/**
 * Callback function type for evaluating a postfix-operator.
 *
 * Same as ec_node_expr_eval_pre_op_t but for postfix operators.
 */
typedef int (*ec_node_expr_eval_post_op_t)(
	void **result,
	void *userctx,
	void *operand,
	const struct ec_pnode *operator
);

/**
 * Callback function type for evaluating a binary operator.
 *
 * @param result
 *   On success, this pointer must be set by the user to point
 *   to a user structure describing the evaluated result.
 * @param userctx
 *   A user-defined context passed to all callback functions.
 * @param operand1
 *   The evaluated left operand.
 * @param operator
 *   The parse result referencing the operator.
 * @param operand2
 *   The evaluated right operand.
 * @return
 *   0 on success (*result must be set, operands are freed),
 *   or -errno on error (*result is undefined, operands are not freed).
 */
typedef int (*ec_node_expr_eval_bin_op_t)(
	void **result,
	void *userctx,
	void *operand1,
	const struct ec_pnode *operator,
	void * operand2
);

/**
 * Callback function type for evaluating a parenthesized expression.
 *
 * @param result
 *   On success, this pointer must be set by the user to point
 *   to a user structure describing the evaluated result.
 * @param userctx
 *   A user-defined context passed to all callback functions.
 * @param open_paren
 *   The parse result referencing the opening parenthesis.
 * @param close_paren
 *   The parse result referencing the closing parenthesis.
 * @param value
 *   The evaluated expression inside the parentheses.
 * @return
 *   0 on success (*result must be set, value is freed),
 *   or -errno on error (*result is undefined, value is not freed).
 */
typedef int (*ec_node_expr_eval_parenthesis_t)(
	void **result,
	void *userctx,
	const struct ec_pnode *open_paren,
	const struct ec_pnode *close_paren,
	void *value
);

/**
 * Callback function type for freeing an evaluated result.
 *
 * @param result
 *   The result to free.
 * @param userctx
 *   A user-defined context passed to all callback functions.
 */
typedef void (*ec_node_expr_eval_free_t)(void *result, void *userctx);

/**
 * Create an expression node.
 *
 * @param id
 *   The node identifier.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_expr(const char *id);

/**
 * Set the value (terminal) node for an expression.
 *
 * @param gen_node
 *   The expression node.
 * @param val_node
 *   The node matching values. It is consumed and will be freed when the
 *   parent is freed, or immediately on error.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_expr_set_val_node(struct ec_node *gen_node, struct ec_node *val_node);

/**
 * Add a binary operator to an expression node.
 *
 * @param gen_node
 *   The expression node.
 * @param op
 *   The operator node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_expr_add_bin_op(struct ec_node *gen_node, struct ec_node *op);

/**
 * Add a prefix operator to an expression node.
 *
 * @param gen_node
 *   The expression node.
 * @param op
 *   The operator node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_expr_add_pre_op(struct ec_node *gen_node, struct ec_node *op);

/**
 * Add a postfix operator to an expression node.
 *
 * @param gen_node
 *   The expression node.
 * @param op
 *   The operator node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_expr_add_post_op(struct ec_node *gen_node, struct ec_node *op);

/**
 * Add parentheses to an expression node.
 *
 * @param gen_node
 *   The expression node.
 * @param open
 *   The opening parenthesis node. It is consumed and will be freed when
 *   the parent is freed, or immediately on error.
 * @param close
 *   The closing parenthesis node. It is consumed and will be freed when
 *   the parent is freed, or immediately on error.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_expr_add_parenthesis(
	struct ec_node *gen_node,
	struct ec_node *open,
	struct ec_node *close
);

/**
 * Expression evaluation operations.
 */
struct ec_node_expr_eval_ops {
	ec_node_expr_eval_var_t eval_var; /**< Evaluate a variable. */
	ec_node_expr_eval_pre_op_t eval_pre_op; /**< Evaluate a prefix operator. */
	ec_node_expr_eval_post_op_t eval_post_op; /**< Evaluate a postfix operator. */
	ec_node_expr_eval_bin_op_t eval_bin_op; /**< Evaluate a binary operator. */
	ec_node_expr_eval_parenthesis_t eval_parenthesis; /**< Evaluate parentheses. */
	ec_node_expr_eval_free_t eval_free; /**< Free an evaluated result. */
};

/**
 * Evaluate a parsed expression.
 *
 * @param result
 *   On success, this pointer will be set to the evaluated result.
 * @param node
 *   The expression node.
 * @param parse
 *   The parse tree to evaluate.
 * @param ops
 *   The evaluation callbacks.
 * @param userctx
 *   A user-defined context passed to all callbacks.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_expr_eval(
	void **result,
	const struct ec_node *node,
	struct ec_pnode *parse,
	const struct ec_node_expr_eval_ops *ops,
	void *userctx
);

/** @} */
