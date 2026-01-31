/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2017, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_node_dynamic Dynamic node
 * @ingroup ecoli_nodes
 * @{
 *
 * @brief A node built dynamically at parse time by a callback.
 */

#pragma once

struct ec_node;
struct ec_pnode;

/** Callback invoked by parse() or complete() to build the dynamic node.
 * The behavior of the node can depend on what is already parsed. */
typedef struct ec_node *(*ec_node_dynamic_build_t)(struct ec_pnode *pstate, void *opaque);

/**
 * Dynamic node where parsing/validation is done in a user-provided callback.
 */
struct ec_node *ec_node_dynamic(const char *id, ec_node_dynamic_build_t build, void *opaque);

/** @} */
