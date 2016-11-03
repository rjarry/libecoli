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
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include <ecoli_malloc.h>
#include <ecoli_log.h>
#include <ecoli_test.h>
#include <ecoli_tk.h>
#include <ecoli_tk_str.h>
#include <ecoli_tk_option.h>
#include <ecoli_tk_shlex.h>

static int isend(char c)
{
	if (c == '\0' || c == '#' || c == '\n' || c == '\r')
		return 1;
	return 0;
}

/* Remove quotes and stop when we reach the end of token. Return the
 * number of "eaten" bytes from the source buffer, or a negative value
 * on error */
/* XXX support simple quotes, try to be posix-compatible */
int get_token(const char *src, char **p_dst)
{
	unsigned s = 0, d = 0, dstlen;
	int quoted = 0;
	char *dst;

	dstlen = strlen(src) + 1;
	dst = ec_malloc(dstlen);
	if (dst == NULL)
		return -ENOMEM;

	/* skip spaces */
	while (isblank(src[s]))
		s++;

	/* empty token */
	if (isend(src[s])) {
		ec_free(dst);
		return -ENOENT;
	}

	/* copy token and remove quotes */
	while (src[s] != '\0') {
		if (d >= dstlen) {
			ec_free(dst);
			return -EMSGSIZE;
		}

		if ((isblank(src[s]) || isend(src[s])) && quoted == 0)
			break;

		if (src[s] == '\\' && src[s+1] == '"') {
			dst[d++] = '"';
			s += 2;
			continue;
		}
		if (src[s] == '\\' && src[s+1] == '\\') {
			dst[d++] = '\\';
			s += 2;
			continue;
		}
		if (src[s] == '"') {
			s++;
			quoted = !quoted;
			continue;
		}
		dst[d++] = src[s++];
	}

	/* not enough room in dst buffer */
	if (d >= dstlen) {
		ec_free(dst);
		return -EMSGSIZE;
	}

	/* end of string during quote */
	if (quoted) {
		ec_free(dst);
		return -EINVAL;
	}

	dst[d++] = '\0';
	*p_dst = dst;
	return s;
}

static int safe_realloc(void *arg, size_t size)
{
	void **pptr = arg;
	void *new_ptr = ec_realloc(*pptr, size);

	if (new_ptr == NULL)
		return -1;
	*pptr = new_ptr;
	return 0;
}

static char **tokenize(const char *str, int add_empty)
{
	char **table = NULL, *token;
	unsigned i, count = 1, off = 0;
	int ret;

	if (safe_realloc(&table, sizeof(char *)) < 0)
		return NULL;

	table[0] = NULL;

	while (1) {
		ret = get_token(str + off, &token);
		if (ret == -ENOENT)
			break;
		else if (ret < 0)
			goto fail;

		off += ret;
		count++;
		if (safe_realloc(&table, sizeof(char *) * count) < 0)
			goto fail;
		table[count - 2] = token;
		table[count - 1] = NULL;
	}

	if (add_empty && (off != strlen(str) || strlen(str) == 0)) {
		token = ec_strdup("");
		if (token == NULL)
			goto fail;

		count++;
		if (safe_realloc(&table, sizeof(char *) * count) < 0)
			goto fail;
		table[count - 2] = token;
		table[count - 1] = NULL;
	}

	return table;

 fail:
	for (i = 0; i < count; i++)
		ec_free(table[i]);
	ec_free(table);
	return NULL;
}

static struct ec_parsed_tk *ec_tk_shlex_parse(const struct ec_tk *gen_tk,
	const char *str)
{
	struct ec_tk_shlex *tk = (struct ec_tk_shlex *)gen_tk;
	struct ec_parsed_tk *parsed_tk, *child_parsed_tk;
	unsigned int i;
	char **tokens, **t;

	parsed_tk = ec_parsed_tk_new(gen_tk);
	if (parsed_tk == NULL)
		return NULL;

	tokens = tokenize(str, 0);
	if (tokens == NULL)
		goto fail;

	t = &tokens[0];
	for (i = 0, t = &tokens[0]; i < tk->len; i++, t++) {
		if (*t == NULL)
			goto fail;

		child_parsed_tk = ec_tk_parse(tk->table[i], *t);
		if (child_parsed_tk == NULL)
			goto fail;

		ec_parsed_tk_add_child(parsed_tk, child_parsed_tk);
		if (strlen(child_parsed_tk->str) == 0)
			t--;
		else if (strlen(child_parsed_tk->str) != strlen(*t))
			goto fail;
	}

	/* check it was the last token */
	if (*t != NULL)
		goto fail;

	if (tokens != NULL) {
		for (t = &tokens[0]; *t != NULL; t++)
			ec_free(*t);
		ec_free(tokens);
		tokens = NULL;
	}

	parsed_tk->str = ec_strdup(str);

	return parsed_tk;

 fail:
	if (tokens != NULL) {
		for (t = &tokens[0]; *t != NULL; t++)
			ec_free(*t);
		ec_free(tokens);
	}
	ec_parsed_tk_free(parsed_tk);

	return NULL;
}

static struct ec_completed_tk *ec_tk_shlex_complete(const struct ec_tk *gen_tk,
	const char *str)
{
	struct ec_tk_shlex *tk = (struct ec_tk_shlex *)gen_tk;
	struct ec_completed_tk *completed_tk, *child_completed_tk = NULL;
	struct ec_parsed_tk *child_parsed_tk;
	unsigned int i;
	char **tokens, **t;

	tokens = tokenize(str, 1);
	if (tokens == NULL)
		goto fail;

	printf("complete <%s>\n", str);
	for (t = &tokens[0]; *t != NULL; t++)
		printf("  token <%s> %p\n", *t, *t);

	t = &tokens[0];

	completed_tk = ec_completed_tk_new();
	if (completed_tk == NULL)
		return NULL;

	for (i = 0, t = &tokens[0]; i < tk->len; i++, t++) {
		if (*(t + 1) != NULL) {
			child_parsed_tk = ec_tk_parse(tk->table[i], *t);
			if (child_parsed_tk == NULL)
				goto fail;

			if (strlen(child_parsed_tk->str) == 0)
				t--;
			else if (strlen(child_parsed_tk->str) != strlen(*t)) {
				ec_parsed_tk_free(child_parsed_tk);
				goto fail;
			}

			ec_parsed_tk_free(child_parsed_tk);
		} else {
			child_completed_tk = ec_tk_complete(tk->table[i], *t);
			if (child_completed_tk == NULL) {
				ec_completed_tk_free(completed_tk);
				return NULL;
			}
			ec_completed_tk_merge(completed_tk, child_completed_tk);

			child_parsed_tk = ec_tk_parse(tk->table[i], "");
			if (child_parsed_tk == NULL)
				break;
			ec_parsed_tk_free(child_parsed_tk);
			t--;
		}
	}

	if (tokens != NULL) {
		for (t = &tokens[0]; *t != NULL; t++)
			ec_free(*t);
		ec_free(tokens);
		tokens = NULL;
	}

	ec_completed_tk_dump(stdout, completed_tk);

	return completed_tk;

 fail:
	if (tokens != NULL) {
		for (t = &tokens[0]; *t != NULL; t++)
			ec_free(*t);
		ec_free(tokens);
	}
	ec_completed_tk_free(completed_tk);

	return NULL;
}

static void ec_tk_shlex_free_priv(struct ec_tk *gen_tk)
{
	struct ec_tk_shlex *tk = (struct ec_tk_shlex *)gen_tk;
	unsigned int i;

	for (i = 0; i < tk->len; i++)
		ec_tk_free(tk->table[i]);
	ec_free(tk->table);
}

static struct ec_tk_ops ec_tk_shlex_ops = {
	.parse = ec_tk_shlex_parse,
	.complete = ec_tk_shlex_complete,
	.free_priv = ec_tk_shlex_free_priv,
};

struct ec_tk *ec_tk_shlex_new(const char *id)
{
	struct ec_tk_shlex *tk = NULL;

	tk = (struct ec_tk_shlex *)ec_tk_new(id, &ec_tk_shlex_ops, sizeof(*tk));
	if (tk == NULL)
		return NULL;

	tk->table = NULL;
	tk->len = 0;

	return &tk->gen;
}

struct ec_tk *ec_tk_shlex_new_list(const char *id, ...)
{
	struct ec_tk_shlex *tk = NULL;
	struct ec_tk *child;
	va_list ap;

	va_start(ap, id);

	tk = (struct ec_tk_shlex *)ec_tk_shlex_new(id);
	if (tk == NULL)
		goto fail;

	for (child = va_arg(ap, struct ec_tk *);
	     child != EC_TK_ENDLIST;
	     child = va_arg(ap, struct ec_tk *)) {
		if (child == NULL)
			goto fail;

		ec_tk_shlex_add(&tk->gen, child);
	}

	va_end(ap);
	return &tk->gen;

fail:
	ec_tk_free(&tk->gen); /* will also free children */
	va_end(ap);
	return NULL;
}

int ec_tk_shlex_add(struct ec_tk *gen_tk, struct ec_tk *child)
{
	struct ec_tk_shlex *tk = (struct ec_tk_shlex *)gen_tk;
	struct ec_tk **table;

	// XXX check tk type

	assert(tk != NULL);
	assert(child != NULL);

	table = ec_realloc(tk->table, (tk->len + 1) * sizeof(*tk->table));
	if (table == NULL)
		return -1;

	tk->table = table;
	table[tk->len] = child;
	tk->len ++;

	return 0;
}

static int ec_tk_shlex_testcase(void)
{
	struct ec_tk *tk;
	int ret = 0;

	tk = ec_tk_shlex_new_list(NULL,
		ec_tk_str_new(NULL, "foo"),
		ec_tk_option_new(NULL, ec_tk_str_new(NULL, "toto")),
		ec_tk_str_new(NULL, "bar"),
		EC_TK_ENDLIST);
	if (tk == NULL) {
		ec_log(EC_LOG_ERR, "cannot create tk\n");
		return -1;
	}
	ret |= EC_TEST_CHECK_TK_PARSE(tk, "foo bar", "foo bar");
	ret |= EC_TEST_CHECK_TK_PARSE(tk, " \"foo\" \"bar\"",
		" \"foo\" \"bar\"");
	ret |= EC_TEST_CHECK_TK_PARSE(tk, "foo toto bar", "foo toto bar");
	ret |= EC_TEST_CHECK_TK_PARSE(tk, " foo   bar ", " foo   bar ");
	ret |= EC_TEST_CHECK_TK_PARSE(tk, "foo bar xxx", NULL);
	ret |= EC_TEST_CHECK_TK_PARSE(tk, "foo barxxx", NULL);
	ret |= EC_TEST_CHECK_TK_PARSE(tk, "foo", NULL);
	ret |= EC_TEST_CHECK_TK_PARSE(tk, " \"foo \" \"bar\"", NULL);
	ec_tk_free(tk);

	/* test completion */
	tk = ec_tk_shlex_new_list(NULL,
		ec_tk_str_new(NULL, "foo"),
		ec_tk_option_new(NULL, ec_tk_str_new(NULL, "toto")),
		ec_tk_str_new(NULL, "bar"),
		ec_tk_str_new(NULL, "titi"),
		EC_TK_ENDLIST);
	if (tk == NULL) {
		ec_log(EC_LOG_ERR, "cannot create tk\n");
		return -1;
	}
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "", "foo");
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, " ", "foo");
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "f", "oo");
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "foo", "");
	ret |= EC_TEST_CHECK_TK_COMPLETE_LIST(tk, "foo ",
		"bar", "toto", EC_TK_ENDLIST);
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "foo t", "oto");
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "foo b", "ar");
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "foo bar", "");
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "foo bar ", "titi");
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "foo toto bar ", "titi");
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "x", "");
	ret |= EC_TEST_CHECK_TK_COMPLETE(tk, "foo barx", "");
	ec_tk_free(tk);

	return ret;
}

static struct ec_test ec_tk_shlex_test = {
	.name = "tk_shlex",
	.test = ec_tk_shlex_testcase,
};

EC_REGISTER_TEST(ec_tk_shlex_test);
