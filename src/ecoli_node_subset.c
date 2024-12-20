/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <stdbool.h>

#include <ecoli_malloc.h>
#include <ecoli_log.h>
#include <ecoli_strvec.h>
#include <ecoli_node.h>
#include <ecoli_parse.h>
#include <ecoli_complete.h>
#include <ecoli_node_subset.h>
#include <ecoli_node_str.h>
#include <ecoli_node_or.h>
#include <ecoli_test.h>

EC_LOG_TYPE_REGISTER(node_subset);

struct ec_node_subset {
	struct ec_node **table;
	unsigned int len;
};

struct parse_result {
	size_t parse_len;           /* number of parsed nodes */
	size_t len;                 /* consumed strings */
};

/* recursively find the longest list of nodes that matches: the state is
 * updated accordingly. */
static int
__ec_node_subset_parse(struct parse_result *out, struct ec_node **table,
		size_t table_len, struct ec_pnode *pstate,
		const struct ec_strvec *strvec)
{
	struct ec_node **child_table;
	struct ec_strvec *childvec = NULL;
	size_t i, j, len = 0;
	struct parse_result best_result, result;
	struct ec_pnode *best_parse = NULL;
	int ret;

	if (table_len == 0)
		return 0;

	memset(&best_result, 0, sizeof(best_result));

	child_table = ec_calloc(table_len - 1, sizeof(*child_table));
	if (child_table == NULL)
		goto fail;

	for (i = 0; i < table_len; i++) {
		/* try to parse elt i */
		ret = ec_parse_child(table[i], pstate, strvec);
		if (ret < 0)
			goto fail;

		if (ret == EC_PARSE_NOMATCH)
			continue;

		/* build a new table without elt i */
		for (j = 0; j < table_len; j++) {
			if (j < i)
				child_table[j] = table[j];
			else if (j > i)
				child_table[j - 1] = table[j];
		}

		/* build a new strvec (ret is the len of matched strvec) */
		len = ret;
		childvec = ec_strvec_ndup(strvec, len,
					ec_strvec_len(strvec) - len);
		if (childvec == NULL)
			goto fail;

		memset(&result, 0, sizeof(result));
		ret = __ec_node_subset_parse(&result, child_table,
					table_len - 1, pstate, childvec);
		ec_strvec_free(childvec);
		childvec = NULL;
		if (ret < 0)
			goto fail;

		/* if result is not the best, ignore */
		if (result.parse_len < best_result.parse_len) {
			memset(&result, 0, sizeof(result));
			ec_pnode_del_last_child(pstate);
			continue;
		}

		/* replace the previous best result */
		ec_pnode_free(best_parse);
		best_parse = ec_pnode_get_last_child(pstate);
		ec_pnode_unlink_child(best_parse);

		best_result.parse_len = result.parse_len + 1;
		best_result.len = len + result.len;

		memset(&result, 0, sizeof(result));
	}

	*out = best_result;
	ec_free(child_table);
	if (best_parse != NULL)
		ec_pnode_link_child(pstate, best_parse);

	return 0;

 fail:
	ec_pnode_free(best_parse);
	ec_strvec_free(childvec);
	ec_free(child_table);
	return -1;
}

static int
ec_node_subset_parse(const struct ec_node *node,
		struct ec_pnode *pstate,
		const struct ec_strvec *strvec)
{
	struct ec_node_subset *priv = ec_node_priv(node);
	struct ec_pnode *parse = NULL;
	struct parse_result result;
	int ret;

	if (ec_strvec_len(strvec) == 0)
		return EC_PARSE_NOMATCH;

	memset(&result, 0, sizeof(result));

	ret = __ec_node_subset_parse(&result, priv->table,
				priv->len, pstate, strvec);
	if (ret < 0)
		goto fail;

	/* if no child node matches, return a matching empty strvec */
	if (result.parse_len == 0)
		return 0;

	return result.len;

 fail:
	ec_pnode_free(parse);
	return ret;
}

static int
__ec_node_subset_complete(struct ec_node **table, size_t table_len,
			struct ec_comp *comp,
			const struct ec_strvec *strvec)
{
	struct ec_pnode *parse = ec_comp_get_cur_pstate(comp);
	struct ec_strvec *childvec = NULL;
	struct ec_node *save;
	size_t i, len;
	int ret;

	/*
	 * example with table = [a, b, c]
	 * subset_complete([a,b,c], strvec) returns:
	 *   complete(a, strvec) + complete(b, strvec) + complete(c, strvec) +
	 *   + __subset_complete([b, c], childvec) if a matches
	 *   + __subset_complete([a, c], childvec) if b matches
	 *   + __subset_complete([a, b], childvec) if c matches
	 */

	/* first, try to complete with each node of the table */
	for (i = 0; i < table_len; i++) {
		if (table[i] == NULL)
			continue;

		ret = ec_complete_child(table[i],
					comp, strvec);
		if (ret < 0)
			goto fail;
	}

	/* then, if a node matches, advance in strvec and try to complete with
	 * all the other nodes */
	for (i = 0; i < table_len; i++) {
		if (table[i] == NULL)
			continue;

		ret = ec_parse_child(table[i], parse, strvec);
		if (ret < 0)
			goto fail;

		if (ret == EC_PARSE_NOMATCH)
			continue;

		len = ret;
		childvec = ec_strvec_ndup(strvec, len,
					ec_strvec_len(strvec) - len);
		if (childvec == NULL) {
			ec_pnode_del_last_child(parse);
			goto fail;
		}

		save = table[i];
		table[i] = NULL;
		ret = __ec_node_subset_complete(table, table_len,
						comp, childvec);
		table[i] = save;
		ec_strvec_free(childvec);
		childvec = NULL;
		ec_pnode_del_last_child(parse);

		if (ret < 0)
			goto fail;
	}

	return 0;

fail:
	return -1;
}

static int
ec_node_subset_complete(const struct ec_node *node,
			struct ec_comp *comp,
			const struct ec_strvec *strvec)
{
	struct ec_node_subset *priv = ec_node_priv(node);

	return __ec_node_subset_complete(priv->table, priv->len, comp,
					strvec);
}

static void ec_node_subset_free_priv(struct ec_node *node)
{
	struct ec_node_subset *priv = ec_node_priv(node);
	size_t i;

	for (i = 0; i < priv->len; i++)
		ec_node_free(priv->table[i]);
	ec_free(priv->table);
}

static size_t
ec_node_subset_get_children_count(const struct ec_node *node)
{
	struct ec_node_subset *priv = ec_node_priv(node);
	return priv->len;
}

static int
ec_node_subset_get_child(const struct ec_node *node, size_t i,
			struct ec_node **child, unsigned int *refs)
{
	struct ec_node_subset *priv = ec_node_priv(node);

	if (i >= priv->len)
		return -1;

	*child = priv->table[i];
	*refs = 1;
	return 0;
}

static struct ec_node_type ec_node_subset_type = {
	.name = "subset",
	.parse = ec_node_subset_parse,
	.complete = ec_node_subset_complete,
	.size = sizeof(struct ec_node_subset),
	.free_priv = ec_node_subset_free_priv,
	.get_children_count = ec_node_subset_get_children_count,
	.get_child = ec_node_subset_get_child,
};

EC_NODE_TYPE_REGISTER(ec_node_subset_type);

int ec_node_subset_add(struct ec_node *node, struct ec_node *child)
{
	struct ec_node_subset *priv = ec_node_priv(node);
	struct ec_node **table;

	assert(node != NULL); // XXX specific assert for it, like in libyang

	if (child == NULL) {
		errno = EINVAL;
		goto fail;
	}

	if (ec_node_check_type(node, &ec_node_subset_type) < 0)
		goto fail;

	table = ec_realloc(priv->table, (priv->len + 1) * sizeof(*priv->table));
	if (table == NULL) {
		ec_node_free(child);
		return -1;
	}

	priv->table = table;
	table[priv->len] = child;
	priv->len++;

	return 0;

fail:
	ec_node_free(child);
	return -1;
}

struct ec_node *__ec_node_subset(const char *id, ...)
{
	struct ec_node *node = NULL;
	struct ec_node *child;
	va_list ap;
	int fail = 0;

	va_start(ap, id);

	node = ec_node_from_type(&ec_node_subset_type, id);
	if (node == NULL)
		fail = 1;;

	for (child = va_arg(ap, struct ec_node *);
	     child != EC_VA_END;
	     child = va_arg(ap, struct ec_node *)) {

		/* on error, don't quit the loop to avoid leaks */
		if (fail == 1 || child == NULL ||
				ec_node_subset_add(node, child) < 0) {
			fail = 1;
			ec_node_free(child);
		}
	}

	if (fail == 1)
		goto fail;

	va_end(ap);
	return node;

fail:
	ec_node_free(node); /* will also free children */
	va_end(ap);
	return NULL;
}

/* LCOV_EXCL_START */
static int ec_node_subset_testcase(void)
{
	struct ec_node *node;
	int testres = 0;

	node = EC_NODE_SUBSET(EC_NO_ID,
		EC_NODE_OR(EC_NO_ID,
			ec_node_str(EC_NO_ID, "foo"),
			ec_node_str(EC_NO_ID, "bar")),
		ec_node_str(EC_NO_ID, "bar"),
		ec_node_str(EC_NO_ID, "toto")
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, -1);
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "foo", "bar", "titi");
	testres |= EC_TEST_CHECK_PARSE(node, 3, "bar", "foo", "toto");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "bar", "bar");
	testres |= EC_TEST_CHECK_PARSE(node, 2, "bar", "foo");
	testres |= EC_TEST_CHECK_PARSE(node, 0, " ");
	testres |= EC_TEST_CHECK_PARSE(node, 0, "foox");
	ec_node_free(node);

	/* test completion */
	node = EC_NODE_SUBSET(EC_NO_ID,
		ec_node_str(EC_NO_ID, "foo"),
		ec_node_str(EC_NO_ID, "bar"),
		ec_node_str(EC_NO_ID, "bar2"),
		ec_node_str(EC_NO_ID, "toto"),
		ec_node_str(EC_NO_ID, "titi")
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_VA_END,
		"foo", "bar", "bar2", "toto", "titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_VA_END,
		"bar2", "bar", "foo", "toto", "titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"bar", "bar2", "", EC_VA_END,
		"foo", "toto", "titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"f", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"b", EC_VA_END,
		"bar", "bar2", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"bar", EC_VA_END,
		"bar", "bar2", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"bar", "b", EC_VA_END,
		"bar2", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"t", EC_VA_END,
		"toto", "titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"to", EC_VA_END,
		"toto", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"x", EC_VA_END,
		EC_VA_END);
	ec_node_free(node);

	return testres;
}

static struct ec_test ec_node_subset_test = {
	.name = "node_subset",
	.test = ec_node_subset_testcase,
};

EC_TEST_REGISTER(ec_node_subset_test);
/* LCOV_EXCL_STOP */
