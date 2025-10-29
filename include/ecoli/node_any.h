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
 * Create a "any" node.
 *
 * This node always matches 1 string in the vector.
 * An optional strvec attribute can be checked too. These
 * attributes are usually set by a lexer node.
 *
 * @param id
 *   The node identifier.
 * @param attr
 *   The strvec attribute to match, or NULL.
 * @return
 *   The ecoli node.
 */
struct ec_node *
ec_node_any(const char *id, const char *attr);

/** @} */
