/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2019, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_node_bypass Bypass node
 * @ingroup ecoli_nodes
 * @{
 *
 * @brief A pass-through node useful for building graph loops.
 */

#pragma once

#include <ecoli/node.h>

/**
 * A node that does nothing other than calling the child node.
 * It can be helpful to build loops in a node graph.
 */
struct ec_node *ec_node_bypass(const char *id, struct ec_node *node);

/**
 * Attach a child to a bypass node.
 */
int ec_node_bypass_set_child(struct ec_node *gen_node, struct ec_node *child);

/** @} */
