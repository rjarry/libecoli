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
 * Create a regular expression node.
 *
 * @param id
 *   The node identifier.
 * @param str
 *   The regular expression pattern (POSIX extended regex).
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_re(const char *id, const char *str);

/**
 * Set the regular expression on a regex node.
 *
 * @param node
 *   The regex node.
 * @param re
 *   The regular expression pattern. It is duplicated internally.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_re_set_regexp(struct ec_node *node, const char *re);

/** @} */
