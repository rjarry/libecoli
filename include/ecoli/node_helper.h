/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_config
 * @{
 *
 * Helpers that are commonly used in nodes.
 */

#pragma once

#include <stdarg.h>
#include <stddef.h>

#include <ecoli/config.h>

struct ec_node;

/**
 * Build a node table from a node list in an ::ec_config.
 *
 * The function takes a node configuration as parameter, which must be a
 * node list. From it, a node table is built. A reference is taken for
 * each node.
 *
 * On error, no reference is taken.
 *
 * @param config
 *   The configuration (type must be a list of nodes). If it is
 *   NULL, an error is returned.
 * @param len
 *   The length of the allocated table on success, or 0 on error.
 * @return
 *   The allocated node table, that must be freed by the caller:
 *   each entry must be freed with ec_node_free() and the table
 *   with free(). On error, NULL is returned and errno is set.
 */
struct ec_node **ec_node_config_node_list_to_table(const struct ec_config *config, size_t *len);

/**
 * Build a list of config nodes from variable arguments.
 *
 * The va_list argument is a list of pointer to ec_node structures,
 * terminated with EC_VA_END.
 *
 * This helper is used by nodes that contain a list of nodes,
 * like "seq", "or", ...
 *
 * @param ap
 *   List of pointer to ec_node structures, terminated with
 *   EC_VA_END.
 * @return
 *   A pointer to an ::ec_config structure. In this case, the
 *   nodes will be freed when the config structure will be freed.
 *   On error, NULL is returned (and errno is set), and the
 *   nodes are freed.
 */
struct ec_config *ec_node_config_node_list_from_vargs(va_list ap);

/** @} */
