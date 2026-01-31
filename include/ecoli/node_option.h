/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_node_option Option node
 * @ingroup ecoli_nodes
 * @{
 *
 * @brief A node that makes its child optional.
 */

#pragma once

#include <ecoli/node.h>

/**
 * Create an option node that makes its child optional.
 *
 * @param id
 *   The node identifier.
 * @param node
 *   The child node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_option(const char *id, struct ec_node *node);

/**
 * Set the child of an option node.
 *
 * @param gen_node
 *   The option node.
 * @param child
 *   The child node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_option_set_child(struct ec_node *gen_node, struct ec_node *child);

/** @} */
