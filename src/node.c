/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <ecoli_malloc.h>
#include <ecoli_string.h>
#include <ecoli_strvec.h>
#include <ecoli_dict.h>
#include <ecoli_log.h>
#include <ecoli_config.h>
#include <ecoli_node.h>

#include <ecoli_node_str.h>
#include <ecoli_node_seq.h>
#include <ecoli_node_or.h>
#include <ecoli_node_int.h>

EC_LOG_TYPE_REGISTER(node);

/* These states are used to mark the grammar graph when freeing, to
 * detect loop. */
enum ec_node_free_state {
	EC_NODE_FREE_STATE_NONE,
	EC_NODE_FREE_STATE_TRAVERSED,
	EC_NODE_FREE_STATE_FREEABLE,
	EC_NODE_FREE_STATE_NOT_FREEABLE,
	EC_NODE_FREE_STATE_FREEING,
};

/**
 * The grammar node structure.
 */
struct ec_node {
	const struct ec_node_type *type; /**< The node type. */
	struct ec_config *config;        /**< Node configuration. */
	char *id;                  /**< Node identifier (EC_NO_ID if none). */
	struct ec_dict *attrs;           /**< Attributes of the node. */
	unsigned int refcnt;             /**< Reference counter. */
	struct {
		enum ec_node_free_state state; /**< State of loop detection. */
		unsigned int refcnt;    /**< Number of reachable references
					 *   starting from node beeing freed. */
	} free; /**< Freeing state: used for loop detection */
};

static struct ec_node_type_list node_type_list =
	TAILQ_HEAD_INITIALIZER(node_type_list);

const struct ec_node_type *
ec_node_type_lookup(const char *name)
{
	struct ec_node_type *type;

	TAILQ_FOREACH(type, &node_type_list, next) {
		if (!strcmp(name, type->name))
			return type;
	}

	errno = ENOENT;
	return NULL;
}

int ec_node_type_register(struct ec_node_type *type, bool override)
{
	if (!override && ec_node_type_lookup(type->name) != NULL) {
		errno = EEXIST;
		return -1;
	}

	TAILQ_INSERT_HEAD(&node_type_list, type, next);

	return 0;
}

void ec_node_type_dump(FILE *out)
{
	struct ec_node_type *type;

	TAILQ_FOREACH(type, &node_type_list, next)
		fprintf(out, "%s\n", type->name);
}

struct ec_node *ec_node_from_type(const struct ec_node_type *type, const char *id)
{
	struct ec_node *node = NULL;

	EC_LOG(EC_LOG_DEBUG, "create node type=%s id=%s\n",
		type->name, id);
	if (id == NULL) {
		errno = EINVAL;
		goto fail;
	}

	node = ec_calloc(1, sizeof(*node) + type->size);
	if (node == NULL)
		goto fail;

	node->type = type;
	node->refcnt = 1;

	// XXX check that id matches [_a-zA-Z][:-_0-9a-zA-Z]*
	node->id = ec_strdup(id);
	if (node->id == NULL)
		goto fail;

	node->attrs = ec_dict();
	if (node->attrs == NULL)
		goto fail;

	if (type->init_priv != NULL) {
		if (type->init_priv(node) < 0)
			goto fail;
	}

	return node;

 fail:
	if (node != NULL) {
		ec_dict_free(node->attrs);
		ec_free(node->id);
	}
	ec_free(node);

	return NULL;
}

const struct ec_config_schema *
ec_node_type_schema(const struct ec_node_type *type)
{
	return type->schema;
}

const char *
ec_node_type_name(const struct ec_node_type *type)
{
	return type->name;
}

struct ec_node *ec_node(const char *typename, const char *id)
{
	const struct ec_node_type *type;

	type = ec_node_type_lookup(typename);
	if (type == NULL) {
		EC_LOG(EC_LOG_ERR, "type=%s does not exist\n",
			typename);
		return NULL;
	}

	return ec_node_from_type(type, id);
}

static void count_references(struct ec_node *node, unsigned int refs)
{
	struct ec_node *child;
	size_t i, n;
	int ret;

	if (node->free.state == EC_NODE_FREE_STATE_TRAVERSED) {
		node->free.refcnt += refs;
		return;
	}
	node->free.refcnt = refs;
	node->free.state = EC_NODE_FREE_STATE_TRAVERSED;
	n = ec_node_get_children_count(node);
	for (i = 0; i < n; i++) {
		ret = ec_node_get_child(node, i, &child, &refs);
		assert(ret == 0);
		count_references(child, refs);
	}
}

static void mark_freeable(struct ec_node *node, enum ec_node_free_state mark)
{
	struct ec_node *child;
	unsigned int refs;
	size_t i, n;
	int ret;

	if (mark == node->free.state)
		return;

	if (node->refcnt > node->free.refcnt)
		mark = EC_NODE_FREE_STATE_NOT_FREEABLE;
	assert(node->refcnt >= node->free.refcnt);
	node->free.state = mark;

	n = ec_node_get_children_count(node);
	for (i = 0; i < n; i++) {
		ret = ec_node_get_child(node, i, &child, &refs);
		assert(ret == 0);
		mark_freeable(child, mark);
	}
}

static void reset_mark(struct ec_node *node)
{
	struct ec_node *child;
	unsigned int refs;
	size_t i, n;
	int ret;

	if (node->free.state == EC_NODE_FREE_STATE_NONE)
		return;

	node->free.state = EC_NODE_FREE_STATE_NONE;
	node->free.refcnt = 0;

	n = ec_node_get_children_count(node);
	for (i = 0; i < n; i++) {
		ret = ec_node_get_child(node, i, &child, &refs);
		assert(ret == 0);
		reset_mark(child);
	}
}

/* free a node, taking care of loops in the node graph */
void ec_node_free(struct ec_node *node)
{
	size_t n;

	if (node == NULL)
		return;

	assert(node->refcnt > 0);

	if (node->free.state == EC_NODE_FREE_STATE_NONE &&
			node->refcnt != 1) {

		/* Traverse the node tree starting from this node, and for each
		 * node, count the number of reachable references. Then, all
		 * nodes whose reachable references == total reference are
		 * marked as freeable, and other are marked as unfreeable. Any
		 * node reachable from an unfreeable node is also marked as
		 * unfreeable. */
		if (node->free.state == EC_NODE_FREE_STATE_NONE) {
			count_references(node, 1);
			mark_freeable(node, EC_NODE_FREE_STATE_FREEABLE);
		}
	}

	if (node->free.state == EC_NODE_FREE_STATE_NOT_FREEABLE) {
		node->refcnt--;
		reset_mark(node);
		return;
	}

	if (node->free.state != EC_NODE_FREE_STATE_FREEING) {
		node->free.state = EC_NODE_FREE_STATE_FREEING;

		/* children will be freed by config_free() and free_priv() */
		ec_config_free(node->config);
		node->config = NULL;
		n = ec_node_get_children_count(node);
		assert(n == 0 || node->type->free_priv != NULL);
		if (node->type->free_priv != NULL)
			node->type->free_priv(node);
		ec_free(node->id);
		ec_dict_free(node->attrs);
	}

	node->refcnt--;
	if (node->refcnt != 0)
		return;

	node->free.state = EC_NODE_FREE_STATE_NONE;
	node->free.refcnt = 0;

	ec_free(node);
}

struct ec_node *ec_node_clone(struct ec_node *node)
{
	if (node != NULL)
		node->refcnt++;
	return node;
}

size_t ec_node_get_children_count(const struct ec_node *node)
{
	if (node->type->get_children_count == NULL)
		return 0;
	return node->type->get_children_count(node);
}

int
ec_node_get_child(const struct ec_node *node, size_t i,
	struct ec_node **child, unsigned int *refs)
{
	*child = NULL;
	*refs = 0;
	if (node->type->get_child == NULL)
		return -1;
	return node->type->get_child(node, i, child, refs);
}

int
ec_node_set_config(struct ec_node *node, struct ec_config *config)
{
	if (node->type->schema == NULL) {
		errno = EINVAL;
		goto fail;
	}
	if (ec_config_validate(config, node->type->schema) < 0)
		goto fail;
	if (node->type->set_config != NULL) {
		if (node->type->set_config(node, config) < 0)
			goto fail;
	}

	ec_config_free(node->config);
	node->config = config;

	return 0;

fail:
	ec_config_free(config);
	return -1;
}

const struct ec_config *ec_node_get_config(struct ec_node *node)
{
	return node->config;
}

struct ec_node *ec_node_find(struct ec_node *node, const char *id)
{
	struct ec_node *child, *retnode;
	const char *node_id = ec_node_id(node);
	unsigned int refs;
	size_t i, n;
	int ret;

	if (id != NULL && node_id != NULL && !strcmp(node_id, id))
		return node;

	n = ec_node_get_children_count(node);
	for (i = 0; i < n; i++) {
		ret = ec_node_get_child(node, i, &child, &refs);
		assert(ret == 0);
		retnode = ec_node_find(child, id);
		if (retnode != NULL)
			return retnode;
	}

	return NULL;
}

const struct ec_node_type *ec_node_type(const struct ec_node *node)
{
	return node->type;
}

struct ec_dict *ec_node_attrs(const struct ec_node *node)
{
	return node->attrs;
}

const char *ec_node_id(const struct ec_node *node)
{
	return node->id;
}

static void __ec_node_dump(FILE *out,
	const struct ec_node *node, size_t indent, struct ec_dict *dict)
{
	const char *id, *typename;
	struct ec_node *child;
	unsigned int refs;
	char buf[32];
	size_t i, n;
	char *desc;
	int ret;

	id = ec_node_id(node);
	desc = ec_node_desc(node);
	typename = node->type->name;

	snprintf(buf, sizeof(buf), "%p", node);
	if (ec_dict_has_key(dict, buf)) {
		fprintf(out, "%*s" "%s type=%s id=%s %p... (loop)\n",
			(int)indent * 4, "", desc, typename, id, node);

		goto end;
	}

	ec_dict_set(dict, buf, NULL, NULL);
	fprintf(out, "%*s" "%s type=%s id=%s %p refs=%u free_state=%d free_refs=%d\n",
		(int)indent * 4, "", desc, typename, id, node, node->refcnt,
		node->free.state, node->free.refcnt);

	n = ec_node_get_children_count(node);
	for (i = 0; i < n; i++) {
		ret = ec_node_get_child(node, i, &child, &refs);
		assert(ret == 0);
		__ec_node_dump(out, child, indent + 1, dict);
	}
end:
	ec_free(desc);
}

/* XXX this is too much debug-oriented, we should have a parameter or 2 funcs */
void ec_node_dump(FILE *out, const struct ec_node *node)
{
	struct ec_dict *dict = NULL;

	fprintf(out, "------------------- node dump:\n");

	if (node == NULL) {
		fprintf(out, "node is NULL\n");
		return;
	}

	dict = ec_dict();
	if (dict == NULL)
		goto fail;

	__ec_node_dump(out, node, 0, dict);

	ec_dict_free(dict);
	return;

fail:
	ec_dict_free(dict);
	EC_LOG(EC_LOG_ERR, "failed to dump node\n");
}

char *ec_node_desc(const struct ec_node *node)
{
	char *desc = NULL;

	if (node->type->desc != NULL)
		return node->type->desc(node);

	if (ec_asprintf(&desc, "<%s>", node->type->name) < 0)
		return NULL;

	return desc;
}

int ec_node_check_type(const struct ec_node *node,
		const struct ec_node_type *type)
{
	if (strcmp(node->type->name, type->name)) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

const char *ec_node_get_type_name(const struct ec_node *node)
{
	return node->type->name;
}

void *ec_node_priv(const struct ec_node *node)
{
	if (node == NULL)
		return NULL;
	return (void *)(node + 1);
}
