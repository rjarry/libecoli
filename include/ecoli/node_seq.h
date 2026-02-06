/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_node_seq Sequence node
 * @ingroup ecoli_nodes
 * @{
 *
 * @brief A node that matches a sequence of child nodes in order.
 */

#pragma once

#include <ecoli/node.h>

/**
 * Create a sequence node from a list of child nodes.
 *
 * All child nodes passed as arguments are consumed and will be freed
 * when the sequence node is freed, or immediately on error.
 *
 * Example:
 *
 * @code{.c}
 * EC_NODE_SEQ("myseq", child1, child2, child3)
 * @endcode
 */
#define EC_NODE_SEQ(args...) __ec_node_seq(args, EC_VA_END)

/* list must be terminated with EC_VA_END */
/* all nodes given in the list will be freed when freeing this one */
/* avoid using this function directly, prefer the macro EC_NODE_SEQ() or
 * ec_node_seq() + ec_node_seq_add() */
struct ec_node *__ec_node_seq(const char *id, ...);

/**
 * Create an empty sequence node.
 *
 * Use ec_node_seq_add() to add children.
 *
 * @param id
 *   The node identifier.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_seq(const char *id);

/**
 * Add a child to a sequence node.
 *
 * @param node
 *   The sequence node.
 * @param child
 *   The child node to add. It is consumed and will be freed when the
 *   parent is freed, or immediately on error.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_seq_add(struct ec_node *node, struct ec_node *child);

/** @} */
