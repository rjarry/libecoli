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
 * Create a string node that matches a specific string.
 *
 * @param id
 *   The node identifier.
 * @param str
 *   The string to match.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_str(const char *id, const char *str);

/**
 * Set the string to match on a string node.
 *
 * @param node
 *   The string node.
 * @param str
 *   The string to match. It is duplicated internally.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_str_set_str(struct ec_node *node, const char *str);

/** @} */
