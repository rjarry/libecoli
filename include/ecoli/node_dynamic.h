/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2017, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

struct ec_node;
struct ec_pnode;

/** callback invoked by parse() or complete() to build the dynamic node
 * the behavior of the node can depend on what is already parsed */
typedef struct ec_node *(*ec_node_dynamic_build_t)(
	struct ec_pnode *pstate, void *opaque);

/**
 * Dynamic node where parsing/validation is done in a user provided callback.
 */
struct ec_node *ec_node_dynamic(const char *id, ec_node_dynamic_build_t build,
				void *opaque);

/** @} */
