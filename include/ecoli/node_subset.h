/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <ecoli/node.h>

/**
 * Create a subset node from a list of child nodes.
 *
 * A subset node matches any permutation of a subset of its children.
 * All child nodes passed as arguments are consumed and will be freed
 * when the subset node is freed, or immediately on error.
 *
 * Example:
 *
 * @code{.c}
 * EC_NODE_SUBSET("mysubset", child1, child2, child3)
 * @endcode
 */
#define EC_NODE_SUBSET(args...) __ec_node_subset(args, EC_VA_END)

/* list must be terminated with EC_VA_END */
/* all nodes given in the list will be freed when freeing this one */
/* avoid using this function directly, prefer the macro EC_NODE_SUBSET() or
 * ec_node_subset() + ec_node_subset_add() */
struct ec_node *__ec_node_subset(const char *id, ...);

/**
 * Create an empty subset node.
 *
 * Use ec_node_subset_add() to add children.
 *
 * @param id
 *   The node identifier.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_subset(const char *id);

/**
 * Add a child to a subset node.
 *
 * @param node
 *   The subset node.
 * @param child
 *   The child node to add. It is consumed and will be freed when the
 *   parent is freed, or immediately on error.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_subset_add(struct ec_node *node, struct ec_node *child);

/** @} */
