/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <ecoli_node.h>

struct ec_node *ec_node_sh_lex(const char *id, struct ec_node *child);

struct ec_node *ec_node_sh_lex_expand(const char *id, struct ec_node *child);

/** @} */
