/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <ecoli_node.h>

#define EC_NODE_CMD(args...) __ec_node_cmd(args, EC_VA_END)

struct ec_node *__ec_node_cmd(const char *id, const char *cmd_str, ...);

/** @} */
