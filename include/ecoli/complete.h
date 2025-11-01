/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_complete Complete
 * @{
 *
 * @brief Complete string input using a grammar graph.
 *
 * This file provides helpers to list and manipulate the possible
 * completions for a given input.
 *
 * Use ec_complete_strvec() to complete a vector of strings when
 * the input is already split into several tokens. You can use
 * ec_complete() if you know that the size of the vector is
 * 1. This is common if your grammar graph includes a lexer that
 * will tokenize the input.
 *
 * These two functions return a pointer to an ::ec_comp structure, that
 * lists the possible completions. The completions are grouped into
 * ec_comp_group. All completions items of a group shares the same parsing
 * state and are issued by the same node.
 */

#pragma once

#include <stdio.h>
#include <sys/queue.h>
#include <sys/types.h>

struct ec_node;
struct ec_comp_item;
struct ec_comp_group;
struct ec_comp;
struct ec_strvec;

/**
 * Completion item type.
 */
enum ec_comp_type {
	EC_COMP_UNKNOWN = 0x1,
	EC_COMP_FULL = 0x2, /**< The item is fully completed. */
	EC_COMP_PARTIAL = 0x4, /**< The item is partially completed. */
	EC_COMP_ALL = 0x7,
};

/**
 * Get the list of completions from a string input.
 *
 * It is equivalent that calling ec_complete_strvec() with a
 * vector that only contains 1 element, the input string. Using this
 * function is often more convenient if you get your input from a
 * buffer, because you won't have to create a vector. Usually, it means
 * you have a lexer in your grammar graph that will tokenize the input.
 *
 * See ec_complete_strvec() for more details.
 *
 * @param node
 *   The grammar graph.
 * @param str
 *   The input string.
 * @return
 *   A pointer to the completion list on success, or NULL
 *   on error (errno is set).
 */
struct ec_comp *ec_complete(const struct ec_node *node, const char *str);

/**
 * Get the list of completions from a string vector input.
 *
 * This function tries to complete the last element of the given string
 * vector. For instance, to complete with file names in an equivalent of
 * the `cat` shell command, the passed vector should be ["cat", ""] (and
 * not ["cat"]). To complete with files starting with "x", the passed
 * vector should be ["cat", "x"].
 *
 * To get the completion list, the engine parses the beginning of the
 * input using the grammar graph. The resulting parsing tree is saved and
 * attached to each completion group.
 *
 * The result is a ec_comp structure pointer, which contains several
 * groups of completion items.
 *
 * @param node
 *   The grammar graph.
 * @param strvec
 *   The input string vector.
 * @return
 *   A pointer to the completion list on success, or NULL
 *   on error (errno is set).
 */
struct ec_comp *ec_complete_strvec(const struct ec_node *node, const struct ec_strvec *strvec);

/**
 * Return a new string vector based on the provided one using completion to
 * expand non-ambiguous tokens to their full value.
 *
 * @param node
 *   The grammar graph.
 * @param type
 *   Completion item type to consider.
 * @param strvec
 *   The input string vector.
 * @return
 *   A new string vector with non-ambiguous tokens expanded, or NULL
 *   on error (errno is set).
 */
struct ec_strvec *ec_complete_strvec_expand(
	const struct ec_node *node,
	enum ec_comp_type type,
	const struct ec_strvec *strvec
);

/**
 * Get the list of completions of a child node.
 *
 * This function is to be used by intermediate ecoli nodes, i.e. nodes
 * which have children (ex: ec_node_seq(), ec_node_or(), ...). It fills an
 * existing ::ec_comp structure, passed by the parent node.
 *
 * @param node
 *   The grammar graph.
 * @param comp
 *   The current completion list to be filled.
 * @param strvec
 *   The input string vector.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_complete_child(
	const struct ec_node *node,
	struct ec_comp *comp,
	const struct ec_strvec *strvec
);

/**
 * Create an empty completion object (list of completion items).
 *
 * @return
 *   A pointer to the completion structure on success, or NULL on error
 *   (errno is set).
 */
struct ec_comp *ec_comp(void);

/**
 * Free a completion object and all its items.
 *
 * @param comp
 *   The pointer to the completion structure to free.
 */
void ec_comp_free(struct ec_comp *comp);

/**
 * Dump the content of a completions list.
 *
 * @param out
 *   The stream where the dump is sent.
 * @param comp
 *   The pointer to the completion list structure.
 */
void ec_comp_dump(FILE *out, const struct ec_comp *comp);

/**
 * Merge items contained in @p from into @p to.
 *
 * The @p from comp struct is freed.
 *
 * @param to
 *   The destination completion list.
 * @param from
 *   The source completion list.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_comp_merge(struct ec_comp *to, struct ec_comp *from);

/**
 * Get current parsing state of completion.
 *
 * This function can be called by a node during the completion process.
 *
 * When processing the list of completions for a given input,
 * an incomplete parsing tree is generated before the completion
 * callback is invoked. A node may use it if the completions list
 * depend on what was previously parsed. For instance, the ec_node_once()
 * node checks in the parsing tree if the node is already parsed.
 * In this case, no completion is issued.
 *
 * @param comp
 *   The current completion list.
 * @return
 *   The current parsing state (cannot be NULL).
 */
struct ec_pnode *ec_comp_get_cur_pstate(const struct ec_comp *comp);

/**
 * Get current completion group.
 *
 * This function can be called by a node during the completion process.
 *
 * A completion group is a list of completion items that share the same
 * parsing state and are issued by the same grammar node. The completion
 * group is created when the first item is added, thus this function
 * returns NULL if no item has been added in the group.
 *
 * @param comp
 *   The current completion list.
 * @return
 *   The current completion group (can be NULL).
 */
struct ec_comp_group *ec_comp_get_cur_group(const struct ec_comp *comp);

/**
 * Get completion attributes.
 *
 * Arbitrary attributes (stored in a dictionary) can be attached to a
 * completion state.
 *
 * @param comp
 *   The completion list.
 * @return
 *   The associated attributes.
 */
struct ec_dict *ec_comp_get_attrs(const struct ec_comp *comp);

/**
 * Add an item in completion list.
 *
 * This function can be called by a node during the completion process,
 * for each completion item that should be added to the list. This is
 * typically done in terminal nodes, like ec_node_str() or ec_node_file().
 *
 * Create a new completion item, and add it into the completion
 * list. A completion item has a type, which can be:
 *
 * - EC_COMP_FULL: the item is fully completed (common case, used
 *   for instance in the str node)
 * - EC_COMP_PARTIAL: the item is only partially completed (this
 *   happens rarely, for instance in the file node, when a completion
 *   goes up to the next slash).
 * - EC_COMP_UNKNOWN: the node detects a valid token, but does not
 *   how to complete it (ex: the int node).
 *
 * @param comp
 *   The current completion list.
 * @param node
 *   The node issuing the completion item.
 * @param type
 *   The type of the item.
 * @param start
 *   The incomplete string being completed.
 * @param full
 *   The string fully completed.
 * @return
 *   The item that was added in the list on success, or NULL
 *   on error. Note: do not free the returned value, as it is referenced
 *   by the completion list. It is returned in case it needs to be
 *   modified, for instance with ec_comp_item_set_display().
 */
struct ec_comp_item *ec_comp_add_item(
	struct ec_comp *comp,
	const struct ec_node *node,
	enum ec_comp_type type,
	const char *start,
	const char *full
);

/**
 * Get the string value of a completion item.
 *
 * @param item
 *   The completion item.
 * @return
 *   The completion string of this item.
 */
const char *ec_comp_item_get_str(const struct ec_comp_item *item);

/**
 * Get the display string value of a completion item.
 *
 * The display string corresponds to what is displayed when
 * listing the possible completions.
 *
 * @param item
 *   The completion item.
 * @return
 *   The display string of this item.
 */
const char *ec_comp_item_get_display(const struct ec_comp_item *item);

/**
 * Get the completion string value of a completion item.
 *
 * The completion string corresponds to what should be added to
 * the incomplete token to complete it.
 *
 * @param item
 *   The completion item.
 * @return
 *   The completion string of this item.
 */
const char *ec_comp_item_get_completion(const struct ec_comp_item *item);

/**
 * Get the group of a completion item.
 *
 * The completion group corresponds to the list of items that share
 * the same parsing state and are issued by the same node.
 *
 * @param item
 *   The completion item.
 * @return
 *   The completion group of this item.
 */
const struct ec_comp_group *ec_comp_item_get_grp(const struct ec_comp_item *item);

/**
 * Get the type of a completion item.
 *
 * @param item
 *   The completion item.
 * @return
 *   The type of this item (EC_COMP_UNKNOWN, EC_COMP_PARTIAL or
 *   EC_COMP_FULL).
 */
enum ec_comp_type ec_comp_item_get_type(const struct ec_comp_item *item);

/**
 * Get the node associated to a completion item.
 *
 * @param item
 *   The completion item.
 * @return
 *   The node that issued the completion item.
 */
const struct ec_node *ec_comp_item_get_node(const struct ec_comp_item *item);

/**
 * Set the completion item string.
 *
 * Some intermediate nodes like sh_lex modify the completion string of
 * items generated by their children, for instance to add quotes.
 *
 * @param item
 *   The completion item to update.
 * @param str
 *   The new item string.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_comp_item_set_str(struct ec_comp_item *item, const char *str);

/**
 * Set the display value of an item.
 *
 * The display string corresponds to what is displayed when listing the
 * possible completions. Some nodes like ec_node_file() change the default
 * value display the base name instead of the full path.
 *
 * @param item
 *   The completion item to update.
 * @param str
 *   The new display string.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_comp_item_set_display(struct ec_comp_item *item, const char *display);

/**
 * Set the completion value of an item.
 *
 * The completion string corresponds to what should be added to
 * the incomplete token to complete it. Some nodes like sh_lex
 * modify it in the items generated by their children, for instance
 * to add a terminating quote.
 *
 * @param item
 *   The completion item to update.
 * @param str
 *   The new completion string.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_comp_item_set_completion(struct ec_comp_item *item, const char *completion);

/**
 * Get the completion group node.
 *
 * @param grp
 *   The completion group.
 */
const struct ec_node *ec_comp_group_get_node(const struct ec_comp_group *grp);

/**
 * Get the completion group parsing state.
 *
 * All items of a completion group are issued by the same node.
 * This function returns a pointer to this node.
 *
 * @param grp
 *   The completion group.
 * @return
 *   The node that issued the completion group.
 */
const struct ec_pnode *ec_comp_group_get_pstate(const struct ec_comp_group *grp);

/**
 * Get the completion group attributes.
 *
 * The parsing state contains the parsing result of the input data
 * preceding the completion. All items of a completion group share the
 * same parsing state.
 *
  * @param grp
 *   The completion group.
 * @return
 *   The parsing state of the completion group.
 */
const struct ec_dict *ec_comp_group_get_attrs(const struct ec_comp_group *grp);

/**
 * Default node completion callback
 *
 * This function is the default completion method for nodes that do
 * not define one.
 *
 * This helper adds a completion item of type EC_COMP_UNKNOWN if the
 * input string vector contains one element, to indicate that everything
 * before the last element of the string vector has been parsed
 * successfully, but the node doesn't know how to complete the last
 * element.
 *
 * @param node
 *   The completion node.
 * @param comp
 *   The completion list to update.
 * @param strvec
 *   The input string vector.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_complete_unknown(
	const struct ec_node *node,
	struct ec_comp *comp,
	const struct ec_strvec *strvec
);

/**
 * Get the number of completion items.
 *
 * Return the number of completion items that match a given type in a
 * completion list.
 *
 * @param comp
 *   The completion list.
 * @param type
 *   A logical OR of flags among EC_COMP_UNKNOWN, EC_COMP_PARTIAL and
 *   EC_COMP_FULL, to select the type to match.
 * @return
 *   The number of matching items.
 */
size_t ec_comp_count(const struct ec_comp *comp, enum ec_comp_type type);

/**
 * Get the first completion item matching the type.
 *
 * Also see EC_COMP_FOREACH().
 *
 * @param comp
 *   The completion list.
 * @param type
 *   A logical OR of flags among EC_COMP_UNKNOWN, EC_COMP_PARTIAL and
 *   EC_COMP_FULL, to select the type to iterate.
 * @return
 *   A completion item.
 */
struct ec_comp_item *ec_comp_iter_first(const struct ec_comp *comp, enum ec_comp_type type);

/**
 * Get the first completion item matching the type.
 *
 * Also see EC_COMP_FOREACH().
 *
 * @param comp
 *   The completion list.
 * @param type
 *   A logical OR of flags among EC_COMP_UNKNOWN, EC_COMP_PARTIAL and
 *   EC_COMP_FULL, to select the type to iterate.
 * @return
 *   A completion item.
 */
struct ec_comp_item *ec_comp_iter_next(struct ec_comp_item *item, enum ec_comp_type type);

/**
 * Iterate items matching a given type.
 *
 * @param item
 *   The item that will be set at each iteration.
 * @param comp
 *   The completion list.
 * @param type
 *   A logical OR of flags among EC_COMP_UNKNOWN, EC_COMP_PARTIAL and
 *   EC_COMP_FULL, to select the type to iterate.
 */
#define EC_COMP_FOREACH(item, comp, type)                                                          \
	for (item = ec_comp_iter_first(comp, type); item != NULL;                                  \
	     item = ec_comp_iter_next(item, type))

/** @} */
