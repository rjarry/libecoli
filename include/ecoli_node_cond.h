/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2019, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

struct ec_node;

/**
 * Create a condition node.
 *
 * The condition node checks that an expression is true before
 * parsing/completing the child node. If it is false, the node
 * doesn't match anything.
 *
 * @param id
 *   The node identifier.
 * @param cond_str
 *   The condition string. This is an function-based expression.
 * @param child
 *   The ecoli child node.
 * @return
 *   The new ecoli cond node.
 */
struct ec_node *ec_node_cond(const char *id, const char *cond_str,
	struct ec_node *child);

/** @} */
