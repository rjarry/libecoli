/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_interact Interactive command line
 * @{
 *
 * @brief Helpers for interactive command line (editline, readline, ...)
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>

#include <histedit.h>

struct ec_node;
struct ec_pnode;
struct ec_comp;

/**
 * A structure describing a contextual help.
 */
struct ec_interact_help {
	char *desc; /**< The short description of the item. */
	char *help; /**< The associated help. */
};

/**
 * The key of the node attribute storing the contextual help.
 */
#define EC_INTERACT_HELP_ATTR "_help"

/**
 * The key of the node attribute storing the command callback.
 */
#define EC_INTERACT_CB_ATTR "_cb"

/**
 * The key of the node attribute storing the short description.
 */
#define EC_INTERACT_DESC_ATTR "_desc"

/**
 * Type of callback attached with EC_INTERACT_CB_ATTR attribute.
 */
typedef int (*ec_interact_command_cb_t)(const struct ec_pnode *);

/**
 * Get completion matches as an array of strings.
 *
 * @param cmpl
 *   The completions, as returned by ec_complete().
 * @param matches_out
 *   The pointer where the matches array will be returned.
 * @return
 *   The size of the array on success (>= 0), or -1 on error.
 */
ssize_t ec_interact_get_completions(const struct ec_comp *cmpl, char ***matches_out);

/**
 * Free the array of completion matches.
 *
 * @param matches
 *   The array of matches.
 * @param n
 *   The size of the array.
 */
void ec_interact_free_completions(char **matches, size_t n);

/**
 * Print completion matches as columns.
 *
 * @param out
 *   The pointer to the output stream.
 * @params width
 *   The number of columns on terminal.
 * @param matches
 *   The string array of matches to display.
 * @param n
 *   The size of the array.
 * @return
 *   0 on success, or -1 on error.
 */
int ec_interact_print_cols(FILE *out, unsigned int width, char const *const *matches, size_t n);

/**
 * Get characters to append to the line for a completion.
 *
 * @param cmpl
 *   The completion object containing all the completion items.
 * @return
 *   An allocated string to be appended to the current line (must be freed by
 *   the caller using free()). Return NULL on error.
 */
char *ec_interact_append_chars(const struct ec_comp *cmpl);

/**
 * Get contextual helps from the current line.
 *
 * @param node
 *   The pointer to the sh_lex grammar node.
 * @param line
 *   The line from which to get help.
 * @param helps_out
 *   The pointer where the helps array will be returned.
 * @return
 *   The size of the array on success (>= 0), or -1 on error.
 */
ssize_t ec_interact_get_helps(
	const struct ec_node *node,
	const char *line,
	struct ec_interact_help **helps_out
);

/**
 * Print helps generated with ec_interact_get_helps().
 *
 * @param out
 *   The pointer to the output stream.
 * @params width
 *   The number of columns on terminal.
 * @param helps
 *   The helps array returned by ec_interact_get_helps().
 * @param n
 *   The array size returned by ec_interact_get_helps().
 * @return
 *   0 on success, or -1 on error.
 */
int ec_interact_print_helps(
	FILE *out,
	unsigned int width,
	const struct ec_interact_help *helps,
	size_t n
);

/**
 * Free contextual helps.
 *
 * Free helps generated with ec_interact_get_helps() or
 * ec_interact_get_error_helps().
 *
 * @param helps
 *   The helps array.
 * @param n
 *   The array size.
 */
void ec_interact_free_helps(struct ec_interact_help *helps, size_t n);

/**
 * Get suggestions after a parsing error for the current line.
 *
 * @param node
 *   The pointer to the sh_lex grammar node.
 * @param line
 *   The line from which to get error help.
 * @param helps_out
 *   The pointer where the helps array will be returned.
 * @param char_idx
 *   A pointer to an integer where the index of the error in the line string
 *   is returned.
 * @return
 *   The size of the array on success (>= 0), or -1 on error.
 */
ssize_t ec_interact_get_error_helps(
	const struct ec_node *node,
	const char *line,
	struct ec_interact_help **helps_out,
	size_t *char_idx
);

/**
 * Print suggestions generated with ec_interact_get_error_helps().
 *
 * @param out
 *   The pointer to the output stream.
 * @params width
 *   The number of columns on terminal.
 * @param line
 *   The line used to generate the error helps.
 * @param helps
 *   The helps array returned by ec_interact_get_helps().
 * @param n
 *   The array size returned by ec_interact_get_helps().
 * @param char_idx
 *   The index of the error in the line string.
 * @return
 *   0 on success, or -1 on error.
 */
int ec_interact_print_error_helps(
	FILE *out,
	unsigned int width,
	const char *line,
	const struct ec_interact_help *helps,
	size_t n,
	size_t char_idx
);

/**
 * Set help on a grammar node.
 *
 * Set the node attribute EC_INTERACT_HELP_ATTR on the node, containing the
 * given string. It is used by the ec_interact functions to display contextual
 * helps on completion or parsing error.
 *
 * @param node
 *   The ec_node on which to add the help attribute.
 * @param help
 *   The help string.
 * @return
 *   0 on success, or -1 on error.
 */
int ec_interact_set_help(struct ec_node *node, const char *help);

/**
 * Set callback function on a grammar node.
 *
 * Set the node attribute EC_INTERACT_CB_ATTR on the node, containing the
 * pointer to a function invoked on successful parsing.
 *
 * @param node
 *   The ec_node on which to add the callback attribute.
 * @param cb
 *   The callback function pointer.
 * @return
 *   0 on success, or -1 on error.
 */
int ec_interact_set_callback(struct ec_node *node, ec_interact_command_cb_t cb);

/**
 * Set short description of a grammar node.
 *
 * Set the node attribute EC_INTERACT_DESC_ATTR on the node, containing the
 * given string. It is used by the ec_interact functions to display the short
 * description of the contextual help, overriding the value provided by
 * ec_node_desc().
 *
 * @param node
 *   The ec_node on which to add the desc attribute.
 * @param desc
 *   The short description string.
 * @return
 *   0 on success, or -1 on error.
 */
int ec_interact_set_desc(struct ec_node *node, const char *desc);

/**
 * Get callback attached to a parse tree.
 *
 * This function browses the parse tree and try to find an attribute EC_INTERACT_CB_ATTR attached to
 * a grammar node referenced in the tree. Return the value of this attribute, which is a function
 * pointer.
 *
 * @param parse
 *   The parsed tree.
 * @return
 *   The function pointer used as command callback.
 */
ec_interact_command_cb_t ec_interact_get_callback(struct ec_pnode *parse);

/** @} */
