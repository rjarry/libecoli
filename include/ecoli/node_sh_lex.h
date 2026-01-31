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
 * Create a shell lexer node.
 *
 * This node tokenizes the input using shell-like lexing rules (handling
 * quotes, escapes, etc.) and passes the resulting tokens to the child node.
 *
 * @param id
 *   The node identifier.
 * @param child
 *   The child node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_sh_lex(const char *id, struct ec_node *child);

/**
 * Create a shell lexer node with variable expansion.
 *
 * Same as ec_node_sh_lex() but with shell variable expansion enabled.
 *
 * @param id
 *   The node identifier.
 * @param child
 *   The child node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_sh_lex_expand(const char *id, struct ec_node *child);

/** @} */
