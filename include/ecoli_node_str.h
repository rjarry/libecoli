/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <ecoli_node.h>

struct ec_node *ec_node_str(const char *id, const char *str);

/* str is duplicated */
int ec_node_str_set_str(struct ec_node *node, const char *str);

/** @} */
