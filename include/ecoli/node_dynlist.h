/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_node_dynlist Dynamic list node
 * @ingroup ecoli_nodes
 * @{
 *
 * @brief A node that matches names from a dynamic list.
 *
 * This node is able to parse a list of object names, returned by a user-defined
 * function as a string vector.
 *
 * Some flags can alter the behavior of parsing and completion:
 * - Match names returned by the user callback.
 * - Match names from a regular expression pattern.
 * - Don't match names returned by the user callback, even if it matches the regexp.
 */

#pragma once

struct ec_node;
struct ec_pnode;

/**
 * Callback invoked by parse() or complete() to build the strvec containing the
 * list of object names.
 *
 * @param pstate
 *   The current parsing state.
 * @param opaque
 *   The user pointer passed at node creation.
 * @return
 *   A string vector containing the list of object names.
 */
typedef struct ec_strvec *(*ec_node_dynlist_get_t)(struct ec_pnode *pstate, void *opaque);

/**
 * Flags passed at ec_node_dynlist creation.
 */
enum ec_node_dynlist_flags {
	/**
	 * Match names returned by the user callback.
	 */
	DYNLIST_MATCH_LIST = 1 << 0,

	/**
	 * Match names from regexp pattern.
	 */
	DYNLIST_MATCH_REGEXP = 1 << 1,

	/**
	 * Don't match names returned by the user callback, even if it matches
	 * the regexp.
	 */
	DYNLIST_EXCLUDE_LIST = 1 << 2,
};

/**
 * Create a dynlist node.
 *
 * The parsing and completion depend on a list returned by a user-provided
 * callback, a regular expression, and flags.
 *
 * @param id
 *   The node identifier.
 * @param get
 *   The function that returns the list of object names as a string vector.
 * @param opaque
 *   A user pointer passed to the get function.
 * @param re_str
 *   The regular expression defining the valid pattern for object names.
 * @param flags
 *   Customize parsing and completion behavior.
 * @return
 *   The dynlist grammar node, or NULL on error.
 */
struct ec_node *ec_node_dynlist(
	const char *id,
	ec_node_dynlist_get_t get,
	void *opaque,
	const char *re_str,
	enum ec_node_dynlist_flags flags
);

/** @} */
