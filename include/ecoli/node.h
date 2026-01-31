/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_nodes Grammar nodes
 * @{
 *
 * @brief Libecoli grammar nodes.
 *
 * The grammar node is a main structure of the ecoli library, used to define how
 * to match and complete the input tokens. A node is a generic object that
 * implements:
 *
 * - a `parse(node, input)` method: check if an input matches
 * - a `complete(node, input)` method: return possible completions for a given
 *   input
 * - some other methods to initialize, free, ...
 *
 * One basic example is the string node (ec_node_str()). A node
 * `ec_node_str("foo")` will match any token list starting with `"foo"`, for
 * example:
 *
 * - `["foo"]`
 * - `["foo", "bar", ...]`
 * But will not match:
 * - `[]`
 * - `["bar", ...]`
 *
 * A node `ec_node_str("foo")` will complete with `"foo"` if the input contains
 * one token, with the same beginning as `"foo"`:
 *
 * - `[""]`
 * - `["f"]`
 * - `["fo"]`
 * - `["foo"]`
 *
 * But it will not complete:
 *
 * - `[]`
 * - `["bar"]`
 * - `["f", ""]`
 * - `["", "f"]`
 *
 * A node can have child nodes. For instance, a sequence node
 * `ec_node_seq(ec_node_str("foo"), ec_node_str("bar"))` will match a sequence:
 * `["foo", "bar"]`.
 *
 * Note: at some places in the documentation and the code, we may talk about the
 * grammar tree, but as loops are allowed, we should instead talk about grammar
 * graph.
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <sys/queue.h>
#include <sys/types.h>

/**
 * Node has no identifier.
 */
#define EC_NO_ID ""

/** Grammar tree node. */
struct ec_node;

/** An iterator on a grammar tree */
struct ec_node_iter;

struct ec_pnode;
struct ec_comp;
struct ec_strvec;
struct ec_dict;
struct ec_config;
struct ec_config_schema;

/**
 * Register a node type at library load.
 *
 * The node type is registered in a function that has the
 * constructor attribute: the function is called at library load.
 *
 * @param t
 *   The name of the ::ec_node_type structure variable.
 */
#define EC_NODE_TYPE_REGISTER(t)                                                                   \
	static void ec_node_init_##t(void);                                                        \
	static void __attribute__((constructor, used)) ec_node_init_##t(void)                      \
	{                                                                                          \
		if (ec_node_type_register(&t, 0) < 0)                                              \
			fprintf(stderr, "cannot register node type %s\n", t.name);                 \
	}

/**
 * Register a node type at library load, overriding previous registration.
 *
 * The node type is registered in a function that has the
 * constructor attribute: the function is called at library load.
 *
 * Be careful when using this macro, as the last type with a given name
 * is the one that is actually registered. The call order may be hard to
 * predict, especially within the same binary.
 *
 * @param t
 *   The name of the ::ec_node_type structure variable.
 */
#define EC_NODE_TYPE_REGISTER_OVERRIDE(t)                                                          \
	static void ec_node_init_##t(void);                                                        \
	static void __attribute__((constructor, used)) ec_node_init_##t(void)                      \
	{                                                                                          \
		if (ec_node_type_register(&t, 1) < 0)                                              \
			fprintf(stderr, "cannot register node type %s\n", t.name);                 \
	}

/**
 * Function type used to configure a node.
 *
 * The function pointer is not called directly, the helper ec_node_set_config()
 * should be used instead.
 *
 * The configuration passed to this function pointer is valid, i.e.
 * ec_config_validate() returned 0 on it.
 *
 * This function provided by a node type is supposed to do additional checks on
 * the configuration and store private data, if needed. If it returns 0,
 * ec_node_set_config() stores the generic configuration in the node. The
 * function can just return 0 if nothing needs to be stored in private data.
 *
 * @param node
 *   The node to configure.
 * @param config
 *   The configuration to apply to the node.
 * @return
 *   0 on success, negative on error (errno is set).
 */
typedef int (*ec_node_set_config_t)(struct ec_node *node, const struct ec_config *config);

/**
 * Parse a string vector using the given grammar graph.
 *
 * The function pointer is not called directly, the helpers ec_parse(),
 * ec_parse_strvec() or ec_parse_child() should be used instead.
 *
 * The implementation of this method for a node that manages children
 * will call ec_parse_child(child, pstate, child_strvec).
 *
 * @param node
 *   The grammar graph.
 * @param pstate
 *   A pointer to the leaf being parsed in the parsing tree. It can be
 *   used by a node to retrieve information from the current parsing
 *   tree. To get the root of the tree, ec_pnode_get_root(pstate) should
 *   be used.
 * @param strvec
 *   The string vector to be parsed.
 * @return
 *   On success, return the number of consumed items in the string vector
 *   (can be 0) or ::EC_PARSE_NOMATCH if the node cannot parse the string
 *   vector. On error, a negative value is returned and errno is set.
 */
typedef int (*ec_parse_t)(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
);

/**
 * Get completion items using the given grammar graph.
 *
 * The function pointer should not be called directly, the helpers
 * ec_complete(), ec_complete_strvec() or ec_complete_child() should be
 * used instead.
 *
 * This function completes the last element of the string vector.
 * For instance, node.type->complete(node, comp, ["ls"]) will
 * list all commands that start with "ls", while
 * node.type->complete(node, comp, ["ls", ""]) will list all
 * possible values for the next argument.
 *
 * The implementation of this function in the node is supposed
 * to either:
 * - call ec_comp_add_item() for each completion item
 *   that should be added to the list. This is typically done in
 *   terminal nodes, for example in ec_node_str() or ec_node_file().
 * - call ec_complete_child() to let
 *   the children nodes add their own completion. This is the
 *   case of ec_node_or which trivially calls ec_complete_child()
 *   on all its children, and of ec_node_seq, which has to
 *   do a more complex job (parsing strvec).
 *
 * A node that does not provide any method for the completion
 * will fall back to ec_complete_unknown(): this helper returns
 * a completion item of type ::EC_COMP_UNKNOWN, just to indicate
 * that everything before the last element of the string vector
 * has been parsed successfully, but we don't know how to
 * complete the last element.
 *
 * @param node
 *   The root node of the grammar graph.
 * @param comp
 *   The current list of completion items, to be filled by the
 *   node.type->complete() method.
 * @param strvec
 *   The string vector to be completed.
 * @return
 *   0 on success, or a negative value on error (errno is set).
 */
typedef int (*ec_complete_t)(
	const struct ec_node *node,
	struct ec_comp *comp,
	const struct ec_strvec *strvec
);

/**
 * Get the short description of a grammar node.
 *
 * This function pointer should not be called directly. The
 * ec_node_desc() helper should be used instead.
 *
 * This callback is typically used when building a help string for a
 * grammar graph. It is used in ecoli editline interface to generate
 * contextual help like this (first column):
 *
 *   `<int>`     An integer.
 *   foo       The foo string.
 *   bar       The bar string.
 *
 * If this callback is set to NULL in the node type, the
 * default behavior is to return the node type name inside `<>`, for
 * instance `<int>`. The string node type implements this method to
 * return the string value. An integer node could implement it
 * to return its range (e.g., "1..10").
 *
 * The returned value is a pointer that must be freed by
 * the caller with free().
 *
 * On error, NULL is returned and errno is set.
 */
typedef char *(*ec_node_desc_t)(const struct ec_node *);

/**
 * Initialize the node private area.
 *
 * This function pointer should not be called directly. The ec_node()
 * and ec_node_from_type() helpers, that allocate new nodes, should be
 * used instead.
 *
 * If not NULL, this function is called when a node is instantiated, to
 * initialize the private area of a node. In any case, the private area
 * is first zeroed.
 *
 * On success, 0 is returned. On error, a negative value is returned and
 * errno is set.
 */
typedef int (*ec_node_init_priv_t)(struct ec_node *);

/**
 * Free the node private area.
 *
 * This function pointer should not be called directly. The
 * ec_node_free() helper should be used instead.
 *
 * When a node is deleted, this function is called to free the resources
 * referenced in the node private area.
 */
typedef void (*ec_node_free_priv_t)(struct ec_node *);

/**
 * Count the number of node children.
 *
 * This function pointer should not be called directly. The
 * ec_node_get_children_count() helper should be used instead.
 *
 * Some grammar nodes like seq, or, many, (...), reference children
 * nodes in the grammar graph. This function returns the number of
 * children.
 */
typedef size_t (*ec_node_get_children_count_t)(const struct ec_node *);

/**
 * Get the i-th child node.
 *
 * This function pointer should not be called directly. The
 * ec_node_get_child() helper should be used instead.
 *
 * Some grammar nodes like seq, or, many, (...), reference children
 * nodes in the grammar graph. This function sets the i-th child (with i
 * lower than the return value of ec_node_get_children_count()) in the
 * child pointer. It also returns the number of references to the child
 * owned by the parent. This information is used by the algorithm that
 * frees a grammar graph taking care of loops.
 *
 * On success, 0 is returned. On error, a negative value is returned and
 * errno is set.
 */
typedef int (*ec_node_get_child_t)(
	const struct ec_node *,
	size_t i,
	struct ec_node **child,
	unsigned int *refs
);

/**
 * A structure describing a grammar node type.
 *
 * It is usually defined as a static const structure in the code
 * defining a new grammar node type. Examples can be found in
 * `ecoli_node_<type>.c` files.
 */
struct ec_node_type {
	TAILQ_ENTRY(ec_node_type) next; /**< Next in list. */
	const char *name; /**< Node type name. */
	/** Configuration schema array, must be terminated by a sentinel
	 *  `{.type = EC_CONFIG_TYPE_NONE}`. */
	const struct ec_config_schema *schema;
	size_t size; /**< Size of private area */
	ec_node_set_config_t set_config; /**< Validate and set configuration. */
	ec_parse_t parse; /**< Parse a string vector. */
	ec_complete_t complete; /**< Get completion items. */
	ec_node_desc_t desc; /**< Get short description. */
	ec_node_init_priv_t init_priv; /**< Initialize private area. */
	ec_node_free_priv_t free_priv; /**< Free node resources. */
	/** Get children count. */
	ec_node_get_children_count_t get_children_count;
	ec_node_get_child_t get_child; /**< Get the i-th child. */
};

/**
 * A list of node types.
 */
TAILQ_HEAD(ec_node_type_list, ec_node_type);

/**
 * The list of registered node types. Must not be modified by user, except at
 * initialization using the EC_NODE_TYPE_REGISTER() or
 * EC_NODE_TYPE_REGISTER_OVERRIDE() macros.
 */
extern struct ec_node_type_list node_type_list;

/**
 * Register a node type.
 *
 * The name of the type being registered is a unique identifier. However,
 * it is possible to force the registration of a type with an existing
 * name by setting "override" to true. Note that the initial type is not
 * removed from the list, instead the new one is added before in the
 * list.
 *
 * @param type
 *   The node type to be registered.
 * @param override
 *   Allow the registration of an existing type.
 * @return
 *   0 on success, negative value on error.
 */
int ec_node_type_register(struct ec_node_type *type, bool override);

/**
 * Lookup node type by name.
 *
 * @param name
 *   The name of the node type to search.
 * @return
 *   The (read-only) node type if found, or NULL on error.
 */
const struct ec_node_type *ec_node_type_lookup(const char *name);

/**
 * Dump registered node types.
 *
 * @param out
 *   The stream where the dump is sent.
 */
void ec_node_type_dump(FILE *out);

/**
 * Get the config schema of a node type.
 *
 * @param type
 *   The node type.
 * @return
 *   The (read-only) config schema of the node type, or NULL
 *   if the node type has no config schema.
 */
const struct ec_config_schema *ec_node_type_schema(const struct ec_node_type *type);

/**
 * Get the name of a node type.
 *
 * @param type
 *   The node type.
 * @return
 *   The (read-only) name of the node type.
 */
const char *ec_node_type_name(const struct ec_node_type *type);

/**
 * Create a new node when the type is known.
 *
 * This function is typically called from the node constructor.
 *
 * @param type
 *   The type of the node to create.
 * @param id
 *   The node identifier.
 * @return
 *   The new node on success, or NULL on error.
 */
struct ec_node *ec_node_from_type(const struct ec_node_type *type, const char *id);

/**
 * Create a new node from its type name.
 *
 * This function is typically called from user code, for node types that do not
 * provide a specific constructor.
 *
 * @param typename
 *   The type name of the node to create.
 * @param id
 *   The node identifier.
 * @return
 *   The new node on success, or NULL on error.
 */
struct ec_node *ec_node(const char *typename, const char *id);

/**
 * Clone a grammar node.
 *
 * This function takes a reference to the node. If ec_node_free() is later
 * called on this node, it will decrease only the reference. The free is
 * effective when the reference count reaches 0. The reference counter is
 * initialized to 1 at node creation.
 *
 * @param node
 *   The node to clone.
 * @return
 *   The pointer to the cloned node, always equal to the parameter. It returns
 *   NULL if the parameter was NULL: in this case, nothing is done.
 */
struct ec_node *ec_node_clone(struct ec_node *node);

/**
 * Decrement node reference counter and free the node if it is the last
 * reference.
 *
 * @param node
 *   The grammar node to free.
 */
void ec_node_free(struct ec_node *node);

/**
 * Set node configuration.
 *
 * For a node that supports generic configuration, set its configuration.
 *
 * After a call to this function, the config is owned by the node and must not
 * be used by the caller anymore.
 *
 * @param node
 *   The grammar node to configure.
 * @param config
 *   The configuration to apply on the node.
 * @return
 *   0 on success, or a negative value on error (in this case the config passed
 *   as parameter is freed).
 */
int ec_node_set_config(struct ec_node *node, struct ec_config *config);

/**
 * Get the current node configuration.
 *
 * For a node that supports generic configuration, get its configuration.
 *
 * @param node
 *   The grammar node.
 * @return
 *   The generic node configuration on success, or NULL on error.
 */
const struct ec_config *ec_node_get_config(const struct ec_node *node);

/**
 * Return the number of children for a node.
 *
 * @param node
 *   The grammar node.
 * @return
 *   The number of children.
 */
size_t ec_node_get_children_count(const struct ec_node *node);

/**
 * Get the n-th child of a node.
 *
 * @param node
 *   The grammar node.
 * @param i
 *   The index of the child node, that must be lower than the number of children.
 * @param child
 *   The pointer where the child node pointer will be stored on success.
 * @return
 *   0 on success, -1 on error.
 */
int ec_node_get_child(const struct ec_node *node, size_t i, struct ec_node **child);

/**
 * Get the type of a node.
 *
 * @param node
 *   The grammar node.
 * @return
 *   The type structure of this node.
 */
const struct ec_node_type *ec_node_type(const struct ec_node *node);

/**
 * Get the attributes dict of the node.
 *
 * A user can add any attribute to a node. The attributes keys starting with an
 * underscore are reserved.
 *
 * @param node
 *   The grammar node.
 * @return
 *   The attribute dictionary of this node.
 */
struct ec_dict *ec_node_attrs(const struct ec_node *node);

/**
 * Get node identifier.
 *
 * @param node
 *   The grammar node.
 * @return
 *   The node identifier string.
 */
const char *ec_node_id(const struct ec_node *node);

/**
 * Get node short description.
 *
 * @param node
 *   The grammar node.
 * @return
 *   An allocated string containing the node short description that must be
 *   freed by the caller. Return NULL on error.
 */
char *ec_node_desc(const struct ec_node *node);

/**
 * Dump a grammar tree.
 *
 * @param out
 *   The stream where the dump is sent.
 * @param node
 *   The grammar tree to dump.
 */
void ec_node_dump(FILE *out, const struct ec_node *node);

/**
 * Find a node from its identifier string.
 *
 * Browse the tree using pre-order depth traversal, and return the first node
 * that matches the given identifier.
 *
 * @param node
 *   The grammar tree.
 * @param id
 *   The identifier to match.
 * @return
 *   A node matching the identifier.
 */
struct ec_node *ec_node_find(struct ec_node *node, const char *id);

/**
 * Create an iterator on a grammar tree.
 *
 * A grammar tree is actually not really a tree, it is an oriented graph where
 * loops can exist. For this reason, it is not possible to iterate the grammar
 * graph without storing somewhere the list of browsed nodes in order to break
 * loops.
 *
 * Another property of grammar graphs is that the same node can appear several
 * times at different places in the graph, having a different parent. This kind
 * of node will be iterated only once.
 *
 * The typical use is:
 *
 * @code{.c}
 *	struct ec_node_iter *iter_root, *iter;
 *
 *	iter_root = ec_node_iter(node);
 *	if (iter_root == NULL)
 *		goto fail;
 *
 *	for (iter = iter_root; iter != NULL;
 *	     iter = ec_node_iter_next(iter_root, iter, true)) {
 *		do_something_with(ec_node_iter_get_node(iter));
 *	}
 *
 *	ec_node_iter_free(iter_root);
 * @endcode
 *
 * Technically, this function builds a tree from the grammar graph. Each node of
 * this tree references a node of the grammar graph.
 *
 * @param node
 *   The grammar graph to iterate
 * @return
 *   The grammar graph iterator on success, that must be freed using
 *   ec_node_iter_free(). Return NULL on error.
 */
struct ec_node_iter *ec_node_iter(struct ec_node *node);

/**
 * Iterate to the next node.
 *
 * See ec_node_iter().
 *
 * @param root
 *   The iterator returned by ec_node_iter().
 * @param iter
 *   The current node in the iterator.
 * @param iter_children
 *   True to iterate the children of "iter", false to skip them.
 * @return
 *   The next node of the iterator, or NULL if iteration is finished.
 */
struct ec_node_iter *
ec_node_iter_next(struct ec_node_iter *root, struct ec_node_iter *iter, bool iter_children);

/**
 * Free a grammar graph iterator.
 *
 * @param iter
 *   The iterator returned by ec_node_iter() to free.
 */
void ec_node_iter_free(struct ec_node_iter *iter);

/**
 * Get the grammar node referenced by the iterator node.
 *
 * @param iter
 *   The iterator node.
 * @return
 *   The associated grammar node.
 */
struct ec_node *ec_node_iter_get_node(struct ec_node_iter *iter);

/**
 * Get the parent of an iterator node.
 *
 * @param iter
 *   The iterator node.
 * @return
 *   The parent of the iterator node in the iterator tree. Return NULL if it's
 *   the root.
 */
struct ec_node_iter *ec_node_iter_get_parent(struct ec_node_iter *iter);

/**
 * Check the type of a node.
 *
 * @param node
 *   The grammar node.
 * @param type
 *   The pointer to the type structure.
 * @return
 *   0 if the node is of specified type, else -1.
 */
int ec_node_check_type(const struct ec_node *node, const struct ec_node_type *type);

/**
 * Get the type name of a grammar node.
 *
 * @param node
 *   The grammar node.
 * @return
 *   The type name of the node.
 */
const char *ec_node_get_type_name(const struct ec_node *node);

/**
 * Get the pointer to the node private area.
 *
 * @param node
 *   The grammar node.
 * @return
 *   The pointer to the node private area.
 */
void *ec_node_priv(const struct ec_node *node);

/**
 * Dump the node configuration schema.
 *
 * @param out
 *   The stream where the dump is sent.
 * @param node
 *   The grammar node.
 */
void ec_node_schema_dump(FILE *out, const struct ec_node *node);

/** @} */
