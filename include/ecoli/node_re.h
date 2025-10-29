/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <ecoli/node.h>

struct ec_node *ec_node_re(const char *id, const char *str);

/* re is duplicated */
int ec_node_re_set_regexp(struct ec_node *node, const char *re);

/** @} */
