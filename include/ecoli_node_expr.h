/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <ecoli_node.h>

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
typedef int (*ec_node_expr_eval_var_t)(
	void **result, void *userctx,
	const struct ec_pnode *var);

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
	void **result, void *userctx,
	void *operand,
	const struct ec_pnode *operator);

typedef int (*ec_node_expr_eval_post_op_t)(
	void **result, void *userctx,
	void *operand,
	const struct ec_pnode *operator);

typedef int (*ec_node_expr_eval_bin_op_t)(
	void **result, void *userctx,
	void *operand1,
	const struct ec_pnode *operator,
	void *operand2);

typedef int (*ec_node_expr_eval_parenthesis_t)(
	void **result, void *userctx,
	const struct ec_pnode *open_paren,
	const struct ec_pnode *close_paren,
	void * value);

typedef void (*ec_node_expr_eval_free_t)(
	void *result, void *userctx);


struct ec_node *ec_node_expr(const char *id);
int ec_node_expr_set_val_node(struct ec_node *gen_node, struct ec_node *val_node);
int ec_node_expr_add_bin_op(struct ec_node *gen_node, struct ec_node *op);
int ec_node_expr_add_pre_op(struct ec_node *gen_node, struct ec_node *op);
int ec_node_expr_add_post_op(struct ec_node *gen_node, struct ec_node *op);
int ec_node_expr_add_parenthesis(struct ec_node *gen_node,
	struct ec_node *open, struct ec_node *close);

struct ec_node_expr_eval_ops {
	ec_node_expr_eval_var_t eval_var;
	ec_node_expr_eval_pre_op_t eval_pre_op;
	ec_node_expr_eval_post_op_t eval_post_op;
	ec_node_expr_eval_bin_op_t eval_bin_op;
	ec_node_expr_eval_parenthesis_t eval_parenthesis;
	ec_node_expr_eval_free_t eval_free;
};

int ec_node_expr_eval(void **result, const struct ec_node *node,
	struct ec_pnode *parse, const struct ec_node_expr_eval_ops *ops,
	void *userctx);

/** @} */
