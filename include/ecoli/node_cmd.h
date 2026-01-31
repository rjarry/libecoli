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
 * Create a command node from a format string.
 *
 * The format string describes a command grammar using a simple syntax.
 * Child nodes are passed as variadic arguments and referenced by name
 * in the format string.
 *
 * Example:
 *
 * @code{.c}
 * EC_NODE_CMD("mycmd", "show NAME", ec_node_str("NAME", NULL));
 * @endcode
 */
#define EC_NODE_CMD(args...) __ec_node_cmd(args, EC_VA_END)

struct ec_node *__ec_node_cmd(const char *id, const char *cmd_str, ...);

/** @} */
