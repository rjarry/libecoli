/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_node_empty Empty node
 * @ingroup ecoli_nodes
 * @{
 *
 * @brief A node that matches an empty input.
 */

#pragma once

/**
 * This node always matches an empty string vector
 */
struct ec_node *ec_node_empty(const char *id);

/** @} */
