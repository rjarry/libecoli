/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <errno.h>

#include <ecoli_malloc.h>
#include <ecoli_log.h>
#include <ecoli_test.h>
#include <ecoli_strvec.h>
#include <ecoli_node.h>
#include <ecoli_complete.h>
#include <ecoli_parse.h>
#include <ecoli_node_many.h>
#include <ecoli_node_or.h>
#include <ecoli_node_str.h>
#include <ecoli_node_int.h>
#include <ecoli_node_re_lex.h>

EC_LOG_TYPE_REGISTER(node_re_lex);

struct regexp_pattern {
	char *pattern;
	regex_t r;
	bool keep;
};

struct ec_node_re_lex {
	struct ec_node gen;
	struct ec_node *child;
	struct regexp_pattern *table;
	size_t len;
};

static struct ec_strvec *
tokenize(struct regexp_pattern *table, size_t table_len, const char *str)
{
	struct ec_strvec *strvec = NULL;
	char *dup = NULL;
	char c;
	size_t len, off = 0;
	size_t i;
	int ret;
	regmatch_t pos;

	dup = ec_strdup(str);
	if (dup == NULL)
		goto fail;

	strvec = ec_strvec();
	if (strvec == NULL)
		goto fail;

	len = strlen(dup);
	while (off < len) {
		for (i = 0; i < table_len; i++) {
			ret = regexec(&table[i].r, &dup[off], 1, &pos, 0);
			if (ret != 0)
				continue;
			if (pos.rm_so != 0 || pos.rm_eo == 0) {
				ret = -1;
				continue;
			}

			if (table[i].keep == 0)
				break;

			c = dup[pos.rm_eo + off];
			dup[pos.rm_eo + off] = '\0';
			EC_LOG(EC_LOG_DEBUG, "re_lex match <%s>\n", &dup[off]);
			if (ec_strvec_add(strvec, &dup[off]) < 0)
				goto fail;

			dup[pos.rm_eo + off] = c;
			break;
		}

		if (ret != 0)
			goto fail;

		off += pos.rm_eo;
	}

	ec_free(dup);
	return strvec;

fail:
	ec_free(dup);
	ec_strvec_free(strvec);
	return NULL;
}

static int
ec_node_re_lex_parse(const struct ec_node *gen_node,
		struct ec_parse *state,
		const struct ec_strvec *strvec)
{
	struct ec_node_re_lex *node = (struct ec_node_re_lex *)gen_node;
	struct ec_strvec *new_vec = NULL;
	struct ec_parse *child_parse;
	const char *str;
	int ret;

	if (ec_strvec_len(strvec) == 0) {
		new_vec = ec_strvec();
	} else {
		str = ec_strvec_val(strvec, 0);
		new_vec = tokenize(node->table, node->len, str);
	}
	if (new_vec == NULL)
		goto fail;

	ret = ec_node_parse_child(node->child, state, new_vec);
	if (ret < 0)
		goto fail;

	if ((unsigned)ret == ec_strvec_len(new_vec)) {
		ret = 1;
	} else if (ret != EC_PARSE_NOMATCH) {
		child_parse = ec_parse_get_last_child(state);
		ec_parse_unlink_child(state, child_parse);
		ec_parse_free(child_parse);
		ret = EC_PARSE_NOMATCH;
	}

	ec_strvec_free(new_vec);
	new_vec = NULL;

	return ret;

 fail:
	ec_strvec_free(new_vec);
	return -1;
}

static void ec_node_re_lex_free_priv(struct ec_node *gen_node)
{
	struct ec_node_re_lex *node = (struct ec_node_re_lex *)gen_node;
	unsigned int i;

	ec_node_free(node->child);
	for (i = 0; i < node->len; i++) {
		ec_free(node->table[i].pattern);
		regfree(&node->table[i].r);
	}

	ec_free(node->table);
}

static size_t
ec_node_re_lex_get_children_count(const struct ec_node *gen_node)
{
	struct ec_node_re_lex *node = (struct ec_node_re_lex *)gen_node;

	if (node->child)
		return 1;
	return 0;
}

static int
ec_node_re_lex_get_child(const struct ec_node *gen_node, size_t i,
			struct ec_node **child, unsigned int *refs)
{
	struct ec_node_re_lex *node = (struct ec_node_re_lex *)gen_node;

	if (i >= 1)
		return -1;

	*child = node->child;
	*refs = 1;
	return 0;
}

static struct ec_node_type ec_node_re_lex_type = {
	.name = "re_lex",
	.parse = ec_node_re_lex_parse,
	.complete = ec_node_complete_unknown,
	.size = sizeof(struct ec_node_re_lex),
	.free_priv = ec_node_re_lex_free_priv,
	.get_children_count = ec_node_re_lex_get_children_count,
	.get_child = ec_node_re_lex_get_child,
};

EC_NODE_TYPE_REGISTER(ec_node_re_lex_type);

int ec_node_re_lex_add(struct ec_node *gen_node, const char *pattern, int keep)
{
	struct ec_node_re_lex *node = (struct ec_node_re_lex *)gen_node;
	struct regexp_pattern *table;
	int ret;
	char *pat_dup = NULL;

	pat_dup = ec_strdup(pattern);
	if (pat_dup == NULL)
		goto fail;

	table = ec_realloc(node->table, sizeof(*table) * (node->len + 1));
	if (table == NULL)
		goto fail;

	ret = regcomp(&table[node->len].r, pattern, REG_EXTENDED);
	if (ret != 0) {
		EC_LOG(EC_LOG_ERR,
			"Regular expression <%s> compilation failed: %d\n",
			pattern, ret);
		if (ret == REG_ESPACE)
			errno = ENOMEM;
		else
			errno = EINVAL;

		goto fail;
	}

	table[node->len].pattern = pat_dup;
	table[node->len].keep = keep;
	node->len++;
	node->table = table;

	return 0;

fail:
	ec_free(pat_dup);
	return -1;
}

struct ec_node *ec_node_re_lex(const char *id, struct ec_node *child)
{
	struct ec_node_re_lex *node = NULL;

	if (child == NULL)
		return NULL;

	node = (struct ec_node_re_lex *)ec_node_from_type(&ec_node_re_lex_type, id);
	if (node == NULL) {
		ec_node_free(child);
		return NULL;
	}

	node->child = child;

	return &node->gen;
}

/* LCOV_EXCL_START */
static int ec_node_re_lex_testcase(void)
{
	struct ec_node *node;
	int ret, testres = 0;

	node = ec_node_re_lex(EC_NO_ID,
		ec_node_many(EC_NO_ID,
			EC_NODE_OR(EC_NO_ID,
				ec_node_str(EC_NO_ID, "foo"),
				ec_node_str(EC_NO_ID, "bar"),
				ec_node_int(EC_NO_ID, 0, 1000, 0)
			), 0, 0
		)
	);
	if (node == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}

	ret = ec_node_re_lex_add(node, "[a-zA-Z]+", 1);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "[0-9]+", 1);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "=", 1);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "-", 1);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "\\+", 1);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	ret = ec_node_re_lex_add(node, "[ 	]+", 0);
	testres |= EC_TEST_CHECK(ret == 0, "cannot add regexp");
	if (ret != 0) {
		EC_LOG(EC_LOG_ERR, "cannot add regexp to node\n");
		ec_node_free(node);
		return -1;
	}

	testres |= EC_TEST_CHECK_PARSE(node, 1, "  foo bar  324 bar234");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "foo bar324");
	testres |= EC_TEST_CHECK_PARSE(node, 1, "");
	testres |= EC_TEST_CHECK_PARSE(node, -1, "foobar");

	/* no completion */
	testres |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_NODE_ENDLIST,
		EC_NODE_ENDLIST);

	ec_node_free(node);

	return testres;
}
/* LCOV_EXCL_STOP */

static struct ec_test ec_node_re_lex_test = {
	.name = "node_re_lex",
	.test = ec_node_re_lex_testcase,
};

EC_TEST_REGISTER(ec_node_re_lex_test);