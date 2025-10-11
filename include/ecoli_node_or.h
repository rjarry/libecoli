/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <ecoli_node.h>
#include <ecoli_utils.h>

/**
 * Create a new "or" node from an arbitrary list of child nodes.
 * All nodes given in the list will be freed when freeing this one.
 */
#define EC_NODE_OR(args...) __ec_node_or(args, EC_VA_END)

/**
 * Avoid using this function directly, prefer the macro EC_NODE_OR() or
 * ec_node_or() + ec_node_or_add()
 */
struct ec_node *__ec_node_or(const char *id, ...);

/**
 * Create an empty "or" node.
 */
struct ec_node *ec_node_or(const char *id);

/**
 * Add a child to an "or" node. Child is consumed.
 */
int ec_node_or_add(struct ec_node *node, struct ec_node *child);

/** @} */
