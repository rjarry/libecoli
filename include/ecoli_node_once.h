/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <ecoli_node.h>

/**
 * This node behaves like its child, but prevent from parsing it several times.
 *
 * Example:
 *
 * ```
 *   many(
 *     or(
 *       once(str("foo")),
 *       str("bar")))
 * ```
 *
 * Matches: [], ["foo", "bar"], ["bar", "bar"], ["foo", "bar", "bar"], ...
 * But not: ["foo", "foo"], ["foo", "bar", "foo"], ...
 *
 * on error, child is *not* freed
 */
struct ec_node *ec_node_once(const char *id, struct ec_node *child);

/** on error, child is freed */
int ec_node_once_set_child(struct ec_node *node, struct ec_node *child);

/** @} */
