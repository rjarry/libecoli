/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <ecoli_node.h>

struct ec_node *ec_node_option(const char *id, struct ec_node *node);
int ec_node_option_set_child(struct ec_node *gen_node, struct ec_node *child);

/** @} */
