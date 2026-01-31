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
 * Create a regex-based lexer node.
 *
 * This node tokenizes the input using regular expressions added with
 * ec_node_re_lex_add() and passes the resulting tokens to the child node.
 *
 * @param id
 *   The node identifier.
 * @param child
 *   The child node. It is consumed and will be freed when the parent
 *   is freed, or immediately on error.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_re_lex(const char *id, struct ec_node *child);

/**
 * Add a token pattern to a regex lexer node.
 *
 * @param gen_node
 *   The regex lexer node.
 * @param pattern
 *   The regular expression pattern for matching tokens.
 * @param keep
 *   If non-zero, include matched tokens in the output; if zero, discard them.
 * @param attr_name
 *   Optional attribute name to attach to matched tokens, or NULL.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_re_lex_add(
	struct ec_node *gen_node,
	const char *pattern,
	int keep,
	const char *attr_name
);

/** @} */
