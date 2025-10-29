/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli/assert.h>
#include <ecoli/dict.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_seq.h>
#include <ecoli/node_sh_lex.h>
#include <ecoli/node_str.h>
#include <ecoli/parse.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(parse);

TAILQ_HEAD(ec_pnode_list, ec_pnode);

struct ec_pnode {
	TAILQ_ENTRY(ec_pnode) next;
	struct ec_pnode_list children;
	struct ec_pnode *parent;
	const struct ec_node *node;
	struct ec_strvec *strvec;
	struct ec_dict *attrs;
};

static int __ec_parse_child(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	bool is_root,
	const struct ec_strvec *strvec
)
{
	struct ec_strvec *match_strvec;
	struct ec_pnode *child = NULL;
	int ret;

	// XXX limit max number of recursions to avoid segfault

	if (ec_node_type(node)->parse == NULL) {
		errno = ENOTSUP;
		return -1;
	}

	if (!is_root) {
		child = ec_pnode(node);
		if (child == NULL)
			return -1;

		ec_pnode_link_child(pstate, child);
	} else {
		child = pstate;
	}
	ret = ec_node_type(node)->parse(node, child, strvec);
	if (ret < 0)
		goto fail;

	if (ret == EC_PARSE_NOMATCH) {
		if (!is_root) {
			ec_pnode_unlink_child(child);
			ec_pnode_free(child);
		}
		return ret;
	}

	match_strvec = ec_strvec_ndup(strvec, 0, ret);
	if (match_strvec == NULL)
		goto fail;

	child->strvec = match_strvec;

	return ret;

fail:
	if (!is_root) {
		ec_pnode_unlink_child(child);
		ec_pnode_free(child);
	}
	return -1;
}

int ec_parse_child(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
)
{
	assert(pstate != NULL);
	return __ec_parse_child(node, pstate, false, strvec);
}

struct ec_pnode *ec_parse_strvec(const struct ec_node *node, const struct ec_strvec *strvec)
{
	struct ec_pnode *pnode = ec_pnode(node);
	int ret;

	if (pnode == NULL)
		return NULL;

	ret = __ec_parse_child(node, pnode, true, strvec);
	if (ret < 0) {
		ec_pnode_free(pnode);
		return NULL;
	}

	return pnode;
}

struct ec_pnode *ec_parse(const struct ec_node *node, const char *str)
{
	struct ec_strvec *strvec = NULL;
	struct ec_pnode *pnode = NULL;

	errno = ENOMEM;
	strvec = ec_strvec();
	if (strvec == NULL)
		goto fail;

	if (ec_strvec_add(strvec, str) < 0)
		goto fail;

	pnode = ec_parse_strvec(node, strvec);
	if (pnode == NULL)
		goto fail;

	ec_strvec_free(strvec);
	return pnode;

fail:
	ec_strvec_free(strvec);
	ec_pnode_free(pnode);
	return NULL;
}

struct ec_pnode *ec_pnode(const struct ec_node *node)
{
	struct ec_pnode *pnode = NULL;

	pnode = calloc(1, sizeof(*pnode));
	if (pnode == NULL)
		goto fail;

	TAILQ_INIT(&pnode->children);

	pnode->node = node;
	pnode->attrs = ec_dict();
	if (pnode->attrs == NULL)
		goto fail;

	return pnode;

fail:
	if (pnode != NULL)
		ec_dict_free(pnode->attrs);
	free(pnode);

	return NULL;
}

static struct ec_pnode *
__ec_pnode_dup(const struct ec_pnode *root, const struct ec_pnode *ref, struct ec_pnode **new_ref)
{
	struct ec_pnode *dup = NULL;
	struct ec_pnode *child, *dup_child;
	struct ec_dict *attrs = NULL;

	if (root == NULL)
		return NULL;

	dup = ec_pnode(root->node);
	if (dup == NULL)
		return NULL;

	if (root == ref)
		*new_ref = dup;

	attrs = ec_dict_dup(root->attrs);
	if (attrs == NULL)
		goto fail;
	ec_dict_free(dup->attrs);
	dup->attrs = attrs;

	if (root->strvec != NULL) {
		dup->strvec = ec_strvec_dup(root->strvec);
		if (dup->strvec == NULL)
			goto fail;
	}

	TAILQ_FOREACH (child, &root->children, next) {
		dup_child = __ec_pnode_dup(child, ref, new_ref);
		if (dup_child == NULL)
			goto fail;
		ec_pnode_link_child(dup, dup_child);
	}

	return dup;

fail:
	ec_pnode_free(dup);
	return NULL;
}

struct ec_pnode *ec_pnode_dup(const struct ec_pnode *pnode)
{
	const struct ec_pnode *root;
	struct ec_pnode *dup_root, *dup = NULL;

	root = EC_PNODE_GET_ROOT(pnode);
	dup_root = __ec_pnode_dup(root, pnode, &dup);
	if (dup_root == NULL)
		return NULL;
	assert(dup != NULL);

	return dup;
}

void ec_pnode_free_children(struct ec_pnode *pnode)
{
	struct ec_pnode *child;

	if (pnode == NULL)
		return;

	while (!TAILQ_EMPTY(&pnode->children)) {
		child = TAILQ_FIRST(&pnode->children);
		TAILQ_REMOVE(&pnode->children, child, next);
		child->parent = NULL;
		ec_pnode_free(child);
	}
}

void ec_pnode_free(struct ec_pnode *pnode)
{
	if (pnode == NULL)
		return;

	ec_assert_print(pnode->parent == NULL, "parent not NULL in ec_pnode_free()");

	ec_pnode_free_children(pnode);
	ec_strvec_free(pnode->strvec);
	ec_dict_free(pnode->attrs);
	free(pnode);
}

static void __ec_pnode_dump(FILE *out, const struct ec_pnode *pnode, size_t indent)
{
	struct ec_pnode *child;
	const struct ec_strvec *vec;
	const char *id = "none", *typename = "none";
	char *desc = NULL;

	/* node can be null when parsing is incomplete */
	if (pnode->node != NULL) {
		id = ec_node_id(pnode->node);
		typename = ec_node_type(pnode->node)->name;
		desc = ec_node_desc(pnode->node);
	}

	fprintf(out,
		"%*s"
		"%s type=%s id=%s vec=",
		(int)indent * 4,
		"",
		desc ?: "none",
		typename,
		id);
	vec = ec_pnode_get_strvec(pnode);
	ec_strvec_dump(out, vec);

	TAILQ_FOREACH (child, &pnode->children, next)
		__ec_pnode_dump(out, child, indent + 1);

	free(desc);
}

// XXX dump in other formats? yaml? json?
void ec_pnode_dump(FILE *out, const struct ec_pnode *pnode)
{
	fprintf(out, "------------------- parse dump:\n");

	if (pnode == NULL) {
		fprintf(out, "pnode is NULL\n");
		return;
	}

	/* Do not dump if it does not match (strvec == NULL) and if it
	 * does not have children. Note that an incomplete parsing tree,
	 * like those generated by complete(), don't match but have
	 * children that may match, and we want to dump them. */
	if (!ec_pnode_matches(pnode) && TAILQ_EMPTY(&pnode->children)) {
		fprintf(out, "no match\n");
		return;
	}

	__ec_pnode_dump(out, pnode, 0);
}

void ec_pnode_link_child(struct ec_pnode *pnode, struct ec_pnode *child)
{
	TAILQ_INSERT_TAIL(&pnode->children, child, next);
	child->parent = pnode;
}

void ec_pnode_unlink_child(struct ec_pnode *child)
{
	struct ec_pnode *parent = child->parent;

	if (parent != NULL) {
		TAILQ_REMOVE(&parent->children, child, next);
		child->parent = NULL;
	}
}

struct ec_pnode *ec_pnode_get_first_child(const struct ec_pnode *pnode)
{
	return TAILQ_FIRST(&pnode->children);
}

struct ec_pnode *ec_pnode_get_last_child(const struct ec_pnode *pnode)
{
	return TAILQ_LAST(&pnode->children, ec_pnode_list);
}

struct ec_pnode *ec_pnode_next(const struct ec_pnode *pnode)
{
	return TAILQ_NEXT(pnode, next);
}

const struct ec_node *ec_pnode_get_node(const struct ec_pnode *pnode)
{
	if (pnode == NULL)
		return NULL;

	return pnode->node;
}

void ec_pnode_del_last_child(struct ec_pnode *pnode)
{
	struct ec_pnode *child;

	child = ec_pnode_get_last_child(pnode);
	if (child != NULL) {
		ec_pnode_unlink_child(child);
		ec_pnode_free(child);
	}
}

struct ec_pnode *ec_pnode_get_root(struct ec_pnode *pnode)
{
	if (pnode == NULL)
		return NULL;

	while (pnode->parent != NULL)
		pnode = pnode->parent;

	return pnode;
}

struct ec_pnode *ec_pnode_get_parent(const struct ec_pnode *pnode)
{
	if (pnode == NULL)
		return NULL;

	return pnode->parent;
}

struct ec_pnode *
__ec_pnode_iter_next(const struct ec_pnode *root, struct ec_pnode *pnode, bool iter_children)
{
	struct ec_pnode *child, *parent, *next;

	if (iter_children) {
		child = TAILQ_FIRST(&pnode->children);
		if (child != NULL)
			return child;
	}
	parent = pnode->parent;
	while (parent != NULL && pnode != root) {
		next = TAILQ_NEXT(pnode, next);
		if (next != NULL)
			return next;
		pnode = parent;
		parent = pnode->parent;
	}
	return NULL;
}

const struct ec_pnode *ec_pnode_find_next(
	const struct ec_pnode *root,
	const struct ec_pnode *prev,
	const char *id,
	bool iter_children
)
{
	const struct ec_pnode *iter;

	if (root == NULL)
		return NULL;
	if (prev == NULL)
		prev = root;
	else
		prev = EC_PNODE_ITER_NEXT(root, prev, iter_children);

	for (iter = prev; iter != NULL; iter = EC_PNODE_ITER_NEXT(root, iter, 1)) {
		if (iter->node != NULL && !strcmp(ec_node_id(iter->node), id))
			return iter;
	}

	return NULL;
}

const struct ec_pnode *ec_pnode_find(const struct ec_pnode *root, const char *id)
{
	return ec_pnode_find_next(root, NULL, id, 1);
}

struct ec_dict *ec_pnode_get_attrs(struct ec_pnode *pnode)
{
	if (pnode == NULL)
		return NULL;

	return pnode->attrs;
}

const struct ec_strvec *ec_pnode_get_strvec(const struct ec_pnode *pnode)
{
	if (pnode == NULL)
		return NULL;

	return pnode->strvec;
}

/* number of strings in the parsed string vector */
size_t ec_pnode_len(const struct ec_pnode *pnode)
{
	if (pnode == NULL || pnode->strvec == NULL)
		return 0;

	return ec_strvec_len(pnode->strvec);
}

size_t ec_pnode_matches(const struct ec_pnode *pnode)
{
	if (pnode == NULL)
		return 0;

	if (pnode->strvec == NULL)
		return 0;

	return 1;
}
