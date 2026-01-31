/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

/**
 * Create a many node that matches its child multiple times.
 *
 * @param id
 *   The node identifier.
 * @param child
 *   The child node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @param min
 *   Minimum number of repetitions. Use 0 for no minimum.
 * @param max
 *   Maximum number of repetitions. Use 0 for no maximum.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *
ec_node_many(const char *id, struct ec_node *child, unsigned int min, unsigned int max);

/**
 * Set the parameters of a many node.
 *
 * @param gen_node
 *   The many node.
 * @param child
 *   The child node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @param min
 *   Minimum number of repetitions. Use 0 for no minimum.
 * @param max
 *   Maximum number of repetitions. Use 0 for no maximum.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_many_set_params(
	struct ec_node *gen_node,
	struct ec_node *child,
	unsigned int min,
	unsigned int max
);

/** @} */
