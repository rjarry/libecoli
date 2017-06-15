/*
 * Copyright (c) 2016, Olivier MATZ <zer0@droids-corp.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ecoli_log.h>
#include <ecoli_malloc.h>
#include <ecoli_test.h>
#include <ecoli_strvec.h>
#include <ecoli_node.h>
#include <ecoli_node_str.h>

struct ec_node_str {
	struct ec_node gen;
	char *string;
	unsigned len;
};

static struct ec_parsed *ec_node_str_parse(const struct ec_node *gen_node,
	const struct ec_strvec *strvec)
{
	struct ec_node_str *node = (struct ec_node_str *)gen_node;
	struct ec_strvec *match_strvec;
	struct ec_parsed *parsed = NULL;
	const char *str;

	parsed = ec_parsed();
	if (parsed == NULL)
		goto fail;

	if (ec_strvec_len(strvec) == 0)
		return parsed;

	str = ec_strvec_val(strvec, 0);
	if (strcmp(str, node->string) != 0)
		return parsed;

	match_strvec = ec_strvec_ndup(strvec, 0, 1);
	if (match_strvec == NULL)
		goto fail;

	ec_parsed_set_match(parsed, gen_node, match_strvec);

	return parsed;

 fail:
	ec_parsed_free(parsed);
	return NULL;
}

static struct ec_completed *ec_node_str_complete(const struct ec_node *gen_node,
	const struct ec_strvec *strvec)
{
	struct ec_node_str *node = (struct ec_node_str *)gen_node;
	struct ec_completed *completed;
	struct ec_completed_elt *completed_elt;
	const char *str, *add;
	size_t n = 0;

	completed = ec_completed();
	if (completed == NULL)
		return NULL;

	if (ec_strvec_len(strvec) != 1)
		return completed;

	str = ec_strvec_val(strvec, 0);
	for (n = 0; n < node->len; n++) {
		if (str[n] != node->string[n])
			break;
	}

	if (str[n] != '\0')
		add = NULL;
	else
		add = node->string + n;

	completed_elt = ec_completed_elt(gen_node, add);
	if (completed_elt == NULL) {
		ec_completed_free(completed);
		return NULL;
	}

	ec_completed_add_elt(completed, completed_elt);

	return completed;
}

static const char *ec_node_str_desc(const struct ec_node *gen_node)
{
	struct ec_node_str *node = (struct ec_node_str *)gen_node;

	return node->string;
}

static void ec_node_str_free_priv(struct ec_node *gen_node)
{
	struct ec_node_str *node = (struct ec_node_str *)gen_node;

	ec_free(node->string);
}

static struct ec_node_type ec_node_str_type = {
	.name = "str",
	.parse = ec_node_str_parse,
	.complete = ec_node_str_complete,
	.desc = ec_node_str_desc,
	.size = sizeof(struct ec_node_str),
	.free_priv = ec_node_str_free_priv,
};

EC_NODE_TYPE_REGISTER(ec_node_str_type);

int ec_node_str_set_str(struct ec_node *gen_node, const char *str)
{
	struct ec_node_str *node = (struct ec_node_str *)gen_node;

	if (str == NULL)
		return -EINVAL;
	if (node->string != NULL)
		return -EEXIST; // XXX allow to replace

	node->string = ec_strdup(str);
	if (node->string == NULL)
		return -ENOMEM;

	node->len = strlen(node->string);

	return 0;
}

struct ec_node *ec_node_str(const char *id, const char *str)
{
	struct ec_node *gen_node = NULL;

	gen_node = __ec_node(&ec_node_str_type, id);
	if (gen_node == NULL)
		goto fail;

	if (ec_node_str_set_str(gen_node, str) < 0)
		goto fail;

	return gen_node;

fail:
	ec_node_free(gen_node);
	return NULL;
}

static int ec_node_str_testcase(void)
{
	struct ec_node *node;
	int ret = 0;

	/* XXX use EC_NO_ID instead of NULL */
	node = ec_node_str(NULL, "foo");
	if (node == NULL) {
		ec_log(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	ret |= EC_TEST_CHECK_PARSE(node, 1, "foo");
	ret |= EC_TEST_CHECK_PARSE(node, 1, "foo", "bar");
	ret |= EC_TEST_CHECK_PARSE(node, -1, "foobar");
	ret |= EC_TEST_CHECK_PARSE(node, -1, " foo");
	ret |= EC_TEST_CHECK_PARSE(node, -1, "");
	ec_node_free(node);

	node = ec_node_str(NULL, "Здравствуйте");
	if (node == NULL) {
		ec_log(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	ret |= EC_TEST_CHECK_PARSE(node, 1, "Здравствуйте");
	ret |= EC_TEST_CHECK_PARSE(node, 1, "Здравствуйте",
		"John!");
	ret |= EC_TEST_CHECK_PARSE(node, -1, "foo");
	ret |= EC_TEST_CHECK_PARSE(node, -1, "");
	ec_node_free(node);

	/* an empty string node always matches */
	node = ec_node_str(NULL, "");
	if (node == NULL) {
		ec_log(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	ret |= EC_TEST_CHECK_PARSE(node, 1, "");
	ret |= EC_TEST_CHECK_PARSE(node, 1, "", "foo");
	ret |= EC_TEST_CHECK_PARSE(node, -1, "foo");
	ec_node_free(node);

	/* test completion */
	node = ec_node_str(NULL, "foo");
	if (node == NULL) {
		ec_log(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}
	ret |= EC_TEST_CHECK_COMPLETE(node,
		EC_NODE_ENDLIST,
		EC_NODE_ENDLIST,
		"");
	ret |= EC_TEST_CHECK_COMPLETE(node,
		"", EC_NODE_ENDLIST,
		"foo", EC_NODE_ENDLIST,
		"foo");
	ret |= EC_TEST_CHECK_COMPLETE(node,
		"f", EC_NODE_ENDLIST,
		"oo", EC_NODE_ENDLIST,
		"oo");
	ret |= EC_TEST_CHECK_COMPLETE(node,
		"foo", EC_NODE_ENDLIST,
		"", EC_NODE_ENDLIST,
		"");
	ret |= EC_TEST_CHECK_COMPLETE(node,
		"x", EC_NODE_ENDLIST,
		EC_NODE_ENDLIST,
		"");
	ec_node_free(node);

	return ret;
}

static struct ec_test ec_node_str_test = {
	.name = "node_str",
	.test = ec_node_str_testcase,
};

EC_TEST_REGISTER(ec_node_str_test);