/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ecoli_malloc.h>
#include <ecoli_log.h>
#include <ecoli_string.h>
#include <ecoli_test.h>
#include <ecoli_strvec.h>
#include <ecoli_htable.h>
#include <ecoli_node.h>
#include <ecoli_parse.h>
#include <ecoli_complete.h>
#include <ecoli_node_seq.h>
#include <ecoli_node_str.h>
#include <ecoli_node_option.h>
#include <ecoli_node_sh_lex.h>

EC_LOG_TYPE_REGISTER(node_sh_lex);

struct ec_node_sh_lex {
	struct ec_node *child;
	bool expand;
};

static int
ec_node_sh_lex_parse(const struct ec_node *node,
		struct ec_pnode *pstate,
		const struct ec_strvec *strvec)
{
	struct ec_node_sh_lex *priv = ec_node_priv(node);
	struct ec_strvec *new_vec = NULL;
	struct ec_pnode *child_parse;
	const char *str;
	int ret;

	if (ec_strvec_len(strvec) == 0) {
		new_vec = ec_strvec();
	} else {
		str = ec_strvec_val(strvec, 0);
		new_vec = ec_strvec_sh_lex_str(str, EC_STRVEC_STRICT, NULL);
	}
	if (new_vec == NULL && errno == EBADMSG) /* quotes not closed */
		return EC_PARSE_NOMATCH;
	if (new_vec == NULL)
		goto fail;

	if (priv->expand) {
		struct ec_strvec *exp;
		exp = ec_complete_strvec_expand(
			priv->child, EC_COMP_FULL, new_vec);
		if (exp == NULL)
			goto fail;
		ec_strvec_free(new_vec);
		new_vec = exp;
	}

	ret = ec_parse_child(priv->child, pstate, new_vec);
	if (ret < 0)
		goto fail;

	if ((unsigned)ret == ec_strvec_len(new_vec)) {
		ret = 1;
	} else if (ret != EC_PARSE_NOMATCH) {
		child_parse = ec_pnode_get_last_child(pstate);
		ec_pnode_unlink_child(child_parse);
		ec_pnode_free(child_parse);
		ret = EC_PARSE_NOMATCH;
	}

	ec_strvec_free(new_vec);
	new_vec = NULL;

	return ret;

 fail:
	ec_strvec_free(new_vec);
	return -1;
}

static int
ec_node_sh_lex_complete(const struct ec_node *node,
			struct ec_comp *comp,
			const struct ec_strvec *strvec)
{
	struct ec_node_sh_lex *priv = ec_node_priv(node);
	struct ec_strvec *new_vec = NULL;
	struct ec_comp_item *item = NULL;
	struct ec_htable *htable = NULL;
	char *new_str = NULL;
	const char *str, *last;
	char missing_quote = '\0';
	int ret;

	if (ec_strvec_len(strvec) != 1)
		return 0;

	str = ec_strvec_val(strvec, 0);
	new_vec = ec_strvec_sh_lex_str(str, EC_STRVEC_TRAILSP, &missing_quote);
	if (new_vec == NULL)
		goto fail;

	/* let's store the existing full completions in a htable */
	htable = ec_htable();
	if (htable == NULL)
		goto fail;

	EC_COMP_FOREACH(item, comp, EC_COMP_FULL) {
		if (ec_htable_set(htable, &item, sizeof(item), NULL, NULL) < 0)
			goto fail;
	}

	/* do the completion */
	if (priv->expand) {
		struct ec_strvec *exp;
		exp = ec_complete_strvec_expand(
			priv->child, EC_COMP_FULL, new_vec);
		if (exp == NULL)
			goto fail;
		ret = ec_complete_child(priv->child, comp, exp);
		ec_strvec_free(exp);
	} else {
		ret = ec_complete_child(priv->child, comp, new_vec);
	}
	if (ret < 0)
		goto fail;

	if (ec_strvec_len(new_vec) > 0)
		last = ec_strvec_val(new_vec, ec_strvec_len(new_vec) - 1);
	else
		last = NULL;

	EC_COMP_FOREACH(item, comp, EC_COMP_FULL) {
		if (ec_htable_has_key(htable, &item, sizeof(item)))
			continue;

		/* update the complete chars to compensate those in the
		   expanded string */
		if (priv->expand && last != NULL) {
			const char *s = ec_comp_item_get_str(item);
			size_t prefix = ec_strcmp_count(s, last);
			if (ec_comp_item_set_completion(item, s + prefix) < 0)
				goto fail;
		}

		/* add missing quote for any new full completions */
		if (missing_quote != '\0') {
			str = ec_comp_item_get_str(item);
			if (ec_asprintf(&new_str, "%c%s%c", missing_quote, str,
					missing_quote) < 0) {
				new_str = NULL;
				goto fail;
			}
			if (ec_comp_item_set_str(item, new_str) < 0)
				goto fail;
			ec_free(new_str);
			new_str = NULL;

			str = ec_comp_item_get_completion(item);
			if (ec_asprintf(&new_str, "%s%c", str,
					missing_quote) < 0) {
				new_str = NULL;
				goto fail;
			}
			if (ec_comp_item_set_completion(item, new_str) < 0)
				goto fail;
			ec_free(new_str);
			new_str = NULL;
		}
	}

	ec_strvec_free(new_vec);
	ec_htable_free(htable);

	return 0;

 fail:
	ec_strvec_free(new_vec);
	ec_free(new_str);
	ec_htable_free(htable);

	return -1;
}

static void ec_node_sh_lex_free_priv(struct ec_node *node)
{
	struct ec_node_sh_lex *priv = ec_node_priv(node);

	ec_node_free(priv->child);
}

static size_t
ec_node_sh_lex_get_children_count(const struct ec_node *node)
{
	struct ec_node_sh_lex *priv = ec_node_priv(node);

	if (priv->child)
		return 1;
	return 0;
}

static int
ec_node_sh_lex_get_child(const struct ec_node *node, size_t i,
	struct ec_node **child, unsigned int *refs)
{
	struct ec_node_sh_lex *priv = ec_node_priv(node);

	if (i >= 1)
		return -1;

	*refs = 1;
	*child = priv->child;
	return 0;
}

static struct ec_node_type ec_node_sh_lex_type = {
	.name = "sh_lex",
	.parse = ec_node_sh_lex_parse,
	.complete = ec_node_sh_lex_complete,
	.size = sizeof(struct ec_node_sh_lex),
	.free_priv = ec_node_sh_lex_free_priv,
	.get_children_count = ec_node_sh_lex_get_children_count,
	.get_child = ec_node_sh_lex_get_child,
};

EC_NODE_TYPE_REGISTER(ec_node_sh_lex_type);

struct ec_node *ec_node_sh_lex(const char *id, struct ec_node *child)
{
	struct ec_node *node = NULL;
	struct ec_node_sh_lex *priv;

	if (child == NULL)
		return NULL;

	node = ec_node_from_type(&ec_node_sh_lex_type, id);
	if (node == NULL) {
		ec_node_free(child);
		return NULL;
	}

	priv = ec_node_priv(node);
	priv->child = child;
	priv->expand = false;

	return node;
}

struct ec_node *ec_node_sh_lex_expand(const char *id, struct ec_node *child)
{
	struct ec_node *node = ec_node_sh_lex(id, child);
	struct ec_node_sh_lex *priv;

	if (node == NULL)
		return NULL;

	priv = ec_node_priv(node);
	priv->expand = true;

	return node;
}

/* LCOV_EXCL_START */
static int ec_node_sh_lex_testcase(void)
{
	struct ec_node *node;
	int testres = 0;

	node = ec_node_sh_lex(EC_NO_ID,
		EC_NODE_SEQ(EC_NO_ID,
			ec_node_str(EC_NO_ID, "foo"),
			ec_node_option(EC_NO_ID,
				ec_node_str(EC_NO_ID, "toto")
			),
			ec_node_str(EC_NO_ID, "bar")
		)
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "  foo   bar");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "  'foo' \"bar\"");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "  'f'oo 'toto' bar");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "  foo toto bar'");
	ec_node_free(node);

	/* test completion */
	node = ec_node_sh_lex(EC_NO_ID,
		EC_NODE_SEQ(EC_NO_ID,
			ec_node_str(EC_NO_ID, "foo"),
			ec_node_option(EC_NO_ID,
				ec_node_str(EC_NO_ID, "toto")
			),
			ec_node_str(EC_NO_ID, "bar"),
			ec_node_str(EC_NO_ID, "titi")
		)
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		" ", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"f", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo", EC_VA_END,
		"foo", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo ", EC_VA_END,
		"bar", "toto", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo t", EC_VA_END,
		"toto", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo b", EC_VA_END,
		"bar", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo bar", EC_VA_END,
		"bar", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo bar ", EC_VA_END,
		"titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo toto bar ", EC_VA_END,
		"titi", EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"x", EC_VA_END,
		EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo barx", EC_VA_END,
		EC_VA_END);
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"foo 'b", EC_VA_END,
		"'bar'", EC_VA_END);

	ec_node_free(node);
	return testres;
}

static struct ec_test ec_node_sh_lex_test = {
	.name = "node_sh_lex",
	.test = ec_node_sh_lex_testcase,
};

EC_TEST_REGISTER(ec_node_sh_lex_test);
/* LCOV_EXCL_STOP */
