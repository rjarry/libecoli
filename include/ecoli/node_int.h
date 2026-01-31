/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_node_int Integer node
 * @ingroup ecoli_nodes
 * @{
 *
 * @brief Nodes that match signed or unsigned integers.
 */

#pragma once

#include <stdint.h>

#include <ecoli/node.h>

/**
 * Create a signed integer node.
 *
 * @param id
 *   The node identifier.
 * @param min
 *   The minimum valid value (included).
 * @param max
 *   The maximum valid value (included).
 * @param base
 *   The base to use for parsing. If 0, try to guess from prefix.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_int(const char *id, int64_t min, int64_t max, unsigned int base);

/**
 * Get the value of a parsed signed integer.
 *
 * @param node
 *   The integer node.
 * @param str
 *   The string to parse.
 * @param result
 *   Pointer where the result will be stored on success.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_int_getval(const struct ec_node *node, const char *str, int64_t *result);

/**
 * Create an unsigned integer node.
 *
 * @param id
 *   The node identifier.
 * @param min
 *   The minimum valid value (included).
 * @param max
 *   The maximum valid value (included).
 * @param base
 *   The base to use for parsing. If 0, try to guess from prefix.
 * @return
 *   The node, or NULL on error (errno is set).
 */
struct ec_node *ec_node_uint(const char *id, uint64_t min, uint64_t max, unsigned int base);

/**
 * Get the value of a parsed unsigned integer.
 *
 * @param node
 *   The unsigned integer node.
 * @param str
 *   The string to parse.
 * @param result
 *   Pointer where the result will be stored on success.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_node_uint_getval(const struct ec_node *node, const char *str, uint64_t *result);

/** @} */
