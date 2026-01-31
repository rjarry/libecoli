/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_parse Parse nodes
 * @{
 *
 * @brief Create parse tree from string input and grammar graph
 *
 * Node parse API.
 *
 * The parse operation is to check if an input (a string or vector of
 * strings) matches the node tree. On success, the result is stored in a
 * tree that describes which part of the input matches which node.
 *
 * The parsing tree is sometimes referenced by another node than the
 * root node. Use ec_pnode_get_root() to get the root node in that case.
 */

#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/queue.h>
#include <sys/types.h>

struct ec_node;

/** Parse tree node. */
struct ec_pnode;

/**
 * Create an empty parsing tree.
 *
 * This function is used internally when parsing an input using
 * a grammar tree.
 *
 * @param node
 *   The grammar node.
 * @return
 *   The empty parse tree.
 */
struct ec_pnode *ec_pnode(const struct ec_node *node);

/**
 * Free a parsing tree.
 *
 * @param pnode
 *   The root of the parsing tree to be freed. It must not have
 *   any parent.
 */
void ec_pnode_free(struct ec_pnode *pnode);

/**
 * Remove and free all the children of a parsing tree node.
 *
 * @param pnode
 *   Node whose children will be freed.
 */
void ec_pnode_free_children(struct ec_pnode *pnode);

/**
 * Duplicate a parsing tree.
 *
 * @param pnode
 *   A node inside a parsing tree.
 * @return
 *   A pointer to the copy of the input node, at the same place in the
 *   copy of the parsing tree. Return NULL on error (errno is set).
 */
struct ec_pnode *ec_pnode_dup(const struct ec_pnode *pnode);

/**
 * Get the string vector associated to a parsing node.
 *
 * When an input is parsed successfully (i.e. the input string vector
 * matches the grammar tree), the matching string vector is copied
 * inside the associated parsing node.
 *
 * For instance, parsing the input ["foo", "bar"] on a grammar which is
 * a sequence of strings, the attached string vector will be ["foo",
 * "bar"] on the root pnode, ["foo"] on the first leaf, and ["bar"] on
 * the second leaf.
 *
 * If the parsing tree does not match (see ec_pnode_matches()), it
 * the associated string vector is NULL.
 *
 * @param pnode
 *   The parsing node. If NULL, the function returns NULL.
 * @return
 *   The string vector associated to the parsing node, or NULL
 *   if the node is not yet parsed (this happens when building the
 *   parsing tree), or if the parsing tree does not match the
 *   input.
 */
const struct ec_strvec *ec_pnode_get_strvec(const struct ec_pnode *pnode);

/**
 * Parse a string using a grammar tree.
 *
 * This is equivalent to calling ec_parse_strvec() on the same
 * node, with a string vector containing only the argument string str.
 *
 * @param node
 *   The grammar node.
 * @param str
 *   The input string.
 * @return
 *   A parsing tree, or NULL on error (errno is set).
 */
struct ec_pnode *ec_parse(const struct ec_node *node, const char *str);

/**
 * Parse a string vector using a grammar tree.
 *
 * Generate a parsing tree by parsing the input string vector using the
 * given grammar tree.
 *
 * The parsing tree is composed of struct ec_pnode, and each of them is
 * associated to a struct ec_node (the grammar node), to the string vector
 * that matched the subtree, and to optional attributes.
 *
 * When the input matches the grammar tree, the string vector associated
 * to the root node of the returned parsing tree is the same than the
 * strvec argument. Calling ec_pnode_matches() on this tree returns true.
 *
 * If the input does not match the grammar tree, the returned parsing
 * tree only contains one root node, with no associated string vector.
 * Calling ec_pnode_matches() on this tree returns false.
 *
 * @param node
 *   The grammar node.
 * @param strvec
 *   The input string vector.
 * @return
 *   A parsing tree, or NULL on error (errno is set).
 */
struct ec_pnode *ec_parse_strvec(const struct ec_node *node, const struct ec_strvec *strvec);

/**
 * Return value of ec_parse_child() when input does not match grammar.
 */
#define EC_PARSE_NOMATCH INT_MAX

/**
 * Parse a string vector using a grammar tree, from a parent node.
 *
 * This function is usually called from an intermediate node (like
 * ec_node_seq, ec_node_or, ...) to backfill the parsing tree, which is
 * built piece by piece while browsing the grammar tree.
 *
 * ec_parse_child() creates a new child in this parsing tree, and calls
 * the parse() method for the child node, with pstate pointing to this
 * new child. If it does not match, the child is removed in the state,
 * else it is kept, with its possible descendants.
 *
 * @param node
 *   The grammar node.
 * @param pstate
 *   The node of the parsing tree.
 * @param strvec
 *   The input string vector.
 * @return
 *   On success: the number of matched elements in the input string
 *   vector (which can be 0), or EC_PARSE_NOMATCH (which is a positive
 *   value) if the input does not match the grammar. On error, -1 is
 *   returned, and errno is set.
 */
int ec_parse_child(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
);

/**
 * Link a parsing node to a parsing tree.
 *
 * Add a child to a node in a parsing tree, at the end of the children
 * list.
 *
 * @param pnode
 *   The node of the parsing tree where the child is added.
 * @param child
 *   The node (or subtree) to add in the children list.
 */
void ec_pnode_link_child(struct ec_pnode *pnode, struct ec_pnode *child);

/**
 * Remove a child node from parsing tree.
 *
 * Remove a child node (and its children) from the children list its
 * parent node in the parsing tree. The child node is not freed.
 *
 * @param child
 *   The node (or subtree) to remove.
 */
void ec_pnode_unlink_child(struct ec_pnode *child);

/**
 * Get the root of the parsing tree.
 *
 * Get the root of the parsing tree, keeping the const statement
 * if the argument has it.
 *
 * @param parse
 *   A node in the parsing tree.
 * @return
 *   The root of the parsing tree.
 */
#define EC_PNODE_GET_ROOT(parse)                                                                   \
	({                                                                                         \
		const struct ec_pnode *p_ = parse; /* check type */                                \
		struct ec_pnode *pnode_ = (struct ec_pnode *)parse;                                \
		__typeof__(parse) res_;                                                            \
		(void)p_;                                                                          \
		res_ = ec_pnode_get_root(pnode_);                                                  \
		res_;                                                                              \
	})

/**
 * Get the root of the parsing tree.
 *
 * You may also use EC_PNODE_GET_ROOT() instead, that keeps the
 * const statement.
 *
 * @param pnode
 *   A node in the parsing tree.
 * @return
 *   The root of the parsing tree.
 */
struct ec_pnode *ec_pnode_get_root(struct ec_pnode *pnode);

/**
 * Get the parent node in the parsing tree.
 *
 * @param pnode
 *   A node in the parsing tree.
 * @return
 *   The parent node, or NULL if it is the root.
 */
struct ec_pnode *ec_pnode_get_parent(const struct ec_pnode *pnode);

/**
 * Get the first child of a node in the parsing tree.
 *
 * @param pnode
 *   A node in the parsing tree.
 * @return
 *   The first child node, or NULL if it has no child.
 */
struct ec_pnode *ec_pnode_get_first_child(const struct ec_pnode *pnode);

/**
 * Get the last child of a node in the parsing tree.
 *
 * @param pnode
 *   A node in the parsing tree.
 * @return
 *   The last child node, or NULL if it has no child.
 */
struct ec_pnode *ec_pnode_get_last_child(const struct ec_pnode *pnode);

/**
 * Get the next sibling node.
 *
 * If pnode is the root of the parsing tree, return NULL. Else return
 * the next sibling node.
 *
 * @param pnode
 *   A node in the parsing tree..
 * @return
 *   The next sibling, or NULL if there is no sibling.
 */
struct ec_pnode *ec_pnode_next(const struct ec_pnode *pnode);

/**
 * Iterate the children of a node.
 *
 * @param child
 *   The item that will be set at each iteration.
 * @param pnode
 *   The parent node in the parsing tree.
 */
#define EC_PNODE_FOREACH_CHILD(child, pnode)                                                       \
	for (child = ec_pnode_get_first_child(pnode); child != NULL; child = ec_pnode_next(child))

/**
 * Get the grammar node corresponding to the parsing node.
 *
 * @param pnode
 *   A node in the parsing tree.
 * @return
 *   The corresponding grammar node, that issued the parse.
 */
const struct ec_node *ec_pnode_get_node(const struct ec_pnode *pnode);

/**
 * Unlink and free the last child.
 *
 * Shortcut to unlink and free the last child of a node. It is a quite
 * common operation in intermediate nodes (like ec_node_seq,
 * ec_node_many, ...) to remove a subtree that was temporarily added
 * when during the completion process.
 *
 * @param pnode
 *   A node in the parsing tree which has at least one child.
 */
void ec_pnode_del_last_child(struct ec_pnode *pnode);

/**
 * Get attributes associated to a node in a parsing tree.
 *
 * Attributes are key/values that are stored in a dictionary
 * and attached to a node in the parsing tree. An attribute can be
 * added to a node by the parsing or completion method of an ec_node.
 *
 * @param pnode
 *   A node in the parsing tree.
 * @return
 *   The dictionary containing the attributes.
 */
struct ec_dict *ec_pnode_get_attrs(const struct ec_pnode *pnode);

/**
 * Dump a parsing tree.
 *
 * @param out
 *   The stream where the dump is done.
 * @param pnode
 *   The parsing tree to dump.
 */
void ec_pnode_dump(FILE *out, const struct ec_pnode *pnode);

/**
 * Find a node from its identifier.
 *
 * Find the first node in the parsing tree which has the given
 * node identifier. The search is a depth-first search.
 *
 * @param root
 *   The node of the parsing tree where the search starts.
 * @param id
 *   The node identifier string to match.
 * @return
 *   The first node matching the identifier, or NULL if not found.
 */
const struct ec_pnode *ec_pnode_find(const struct ec_pnode *root, const char *id);

/**
 * Find the next node matching an identifier.
 *
 * After a successful call to ec_pnode_find() or ec_pnode_find_next(), it
 * is possible to get the next node that has the specified id. There are
 * 2 options:
 * - continue the depth-first search where it was interrupted.
 * - skip the children of the current node, and continue the depth-first
 *   search.
 *
 * @param root
 *   The root of the search, as passed to ec_pnode_find().
 * @param prev
 *   The node returned by the previous search.
 * @param id
 *   The node identifier string to match.
 * @param iter_children
 *   True to iterate the children of "prev", false to skip them.
 * @return
 *   The next node matching the identifier, or NULL if not found.
 */
const struct ec_pnode *ec_pnode_find_next(
	const struct ec_pnode *root,
	const struct ec_pnode *prev,
	const char *id,
	bool iter_children
);

/**
 * Count node occurrences in a parse subtree.
 *
 * @param root
 *   The node of the parsing tree where the search starts.
 * @param id
 *   The node identifier string to match.
 * @return
 *   The number of nodes matching the identifier.
 */
unsigned int ec_pnode_count(const struct ec_pnode *root, const char *id);

/**
 * Iterate among parse tree
 *
 * Use it with:
 * for (iter = pnode; iter != NULL; iter = EC_PNODE_ITER_NEXT(pnode, iter, 1))
 */
struct ec_pnode *
__ec_pnode_iter_next(const struct ec_pnode *root, struct ec_pnode *pnode, bool iter_children);

/**
 * Get the next node in a parse tree iteration.
 *
 * @param root
 *   The root of the parse tree being iterated.
 * @param parse
 *   The current parse node.
 * @param iter_children
 *   If true, iterate over children; if false, skip to siblings.
 * @return
 *   The next parse node, or NULL if iteration is complete.
 */
#define EC_PNODE_ITER_NEXT(root, parse, iter_children)                                             \
	({                                                                                         \
		const struct ec_pnode *p_ = parse; /* check type */                                \
		struct ec_pnode *pnode_ = (struct ec_pnode *)parse;                                \
		__typeof__(parse) res_;                                                            \
		(void)p_;                                                                          \
		res_ = __ec_pnode_iter_next(root, pnode_, iter_children);                          \
		res_;                                                                              \
	})

/**
 * Iterate depth-first over a parsing tree.
 *
 * @param iter
 *   The item that will be set at each iteration.
 * @param root
 *   The root of the parsing tree to iterate.
 */
#define EC_PNODE_FOREACH(iter, root)                                                               \
	for ((iter) = (root); (iter) != NULL; (iter) = EC_PNODE_ITER_NEXT((root), (iter), true))

/**
 * Get the number of strings in the parsed string vector.
 *
 * @param pnode
 *   A node in the parsing tree.
 * @return
 *   The number of strings in the parsed string vector, or 0 if the
 *   node is NULL or has no string vector.
 */
size_t ec_pnode_len(const struct ec_pnode *pnode);

/**
 * Check if the parsing tree matches the input.
 *
 * @param pnode
 *   A node in the parsing tree.
 * @return
 *   true if the parsing tree matches (has a string vector), false otherwise.
 */
bool ec_pnode_matches(const struct ec_pnode *pnode);

/** @} */
