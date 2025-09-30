/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <ecoli_malloc.h>
#include <ecoli_node.h>
#include <ecoli_test.h>
#include <ecoli_log.h>
#include <ecoli_dict.h>
#include <ecoli_strvec.h>

EC_LOG_TYPE_REGISTER(strvec);

struct ec_strvec_elt {
	unsigned int refcnt;
	char *str;
	struct ec_dict *attrs;
};

struct ec_strvec {
	size_t len;
	struct ec_strvec_elt **vec;
};

struct ec_strvec *ec_strvec(void)
{
	struct ec_strvec *strvec;

	strvec = ec_calloc(1, sizeof(*strvec));
	if (strvec == NULL)
		return NULL;

	return strvec;
}

static struct ec_strvec_elt *
__ec_strvec_elt(const char *s)
{
	struct ec_strvec_elt *elt;

	elt = ec_calloc(1, sizeof(*elt));
	if (elt == NULL)
		return NULL;

	elt->str = ec_strdup(s);
	if (elt->str == NULL) {
		ec_free(elt);
		return NULL;
	}
	elt->refcnt = 1;

	return elt;
}

static void
__ec_strvec_elt_free(struct ec_strvec_elt *elt)
{
	elt->refcnt--;
	if (elt->refcnt == 0) {
		ec_free(elt->str);
		ec_dict_free(elt->attrs);
		ec_free(elt);
	}
}

int ec_strvec_set(struct ec_strvec *strvec, size_t idx, const char *s)
{
	struct ec_strvec_elt *elt;

	if (strvec == NULL || s == NULL || idx >= strvec->len) {
		errno = EINVAL;
		return -1;
	}

	elt = __ec_strvec_elt(s);
	if (elt == NULL)
		return -1;

	__ec_strvec_elt_free(strvec->vec[idx]);
	strvec->vec[idx] = elt;

	return 0;
}

int ec_strvec_add(struct ec_strvec *strvec, const char *s)
{
	struct ec_strvec_elt *elt, **new_vec;

	if (strvec == NULL || s == NULL) {
		errno = EINVAL;
		return -1;
	}

	new_vec = ec_realloc(strvec->vec,
		sizeof(*strvec->vec) * (strvec->len + 1));
	if (new_vec == NULL)
		return -1;

	strvec->vec = new_vec;

	elt = __ec_strvec_elt(s);
	if (elt == NULL)
		return -1;

	new_vec[strvec->len] = elt;
	strvec->len++;

	return 0;
}

struct ec_strvec *ec_strvec_from_array(const char * const *strarr,
	size_t n)
{
	struct ec_strvec *strvec = NULL;
	size_t i;

	strvec = ec_strvec();
	if (strvec == NULL)
		goto fail;

	for (i = 0; i < n; i++) {
		if (ec_strvec_add(strvec, strarr[i]) < 0)
			goto fail;
	}

	return strvec;

fail:
	ec_strvec_free(strvec);
	return NULL;
}

int ec_strvec_del_last(struct ec_strvec *strvec)
{
	if (strvec->len == 0) {
		errno = EINVAL;
		return -1;
	}

	__ec_strvec_elt_free(strvec->vec[strvec->len - 1]);
	strvec->len--;

	return 0;
}

struct ec_strvec *ec_strvec_ndup(const struct ec_strvec *strvec, size_t off,
	size_t len)
{
	struct ec_strvec *copy = NULL;
	size_t i, veclen;

	veclen = ec_strvec_len(strvec);
	if (off + len > veclen)
		return NULL;

	copy = ec_strvec();
	if (copy == NULL)
		goto fail;

	if (len == 0)
		return copy;

	copy->vec = ec_calloc(len, sizeof(*copy->vec));
	if (copy->vec == NULL)
		goto fail;

	for (i = 0; i < len; i++) {
		copy->vec[i] = strvec->vec[i + off];
		copy->vec[i]->refcnt++;
	}
	copy->len = len;

	return copy;

fail:
	ec_strvec_free(copy);
	return NULL;
}

struct ec_strvec *ec_strvec_dup(const struct ec_strvec *strvec)
{
	return ec_strvec_ndup(strvec, 0, ec_strvec_len(strvec));
}

void ec_strvec_free(struct ec_strvec *strvec)
{
	struct ec_strvec_elt *elt;
	size_t i;

	if (strvec == NULL)
		return;

	for (i = 0; i < ec_strvec_len(strvec); i++) {
		elt = strvec->vec[i];
		__ec_strvec_elt_free(elt);
	}

	ec_free(strvec->vec);
	ec_free(strvec);
}

size_t ec_strvec_len(const struct ec_strvec *strvec)
{
	return strvec->len;
}

const char *ec_strvec_val(const struct ec_strvec *strvec, size_t idx)
{
	if (strvec == NULL || idx >= strvec->len)
		return NULL;

	return strvec->vec[idx]->str;
}

const struct ec_dict *ec_strvec_get_attrs(const struct ec_strvec *strvec,
	size_t idx)
{
	if (strvec == NULL || idx >= strvec->len) {
		errno = EINVAL;
		return NULL;
	}

	return strvec->vec[idx]->attrs;
}

int ec_strvec_set_attrs(struct ec_strvec *strvec, size_t idx,
			struct ec_dict *attrs)
{
	struct ec_strvec_elt *elt;

	if (strvec == NULL || idx >= strvec->len) {
		errno = EINVAL;
		goto fail;
	}

	elt = strvec->vec[idx];
	if (elt->refcnt > 1) {
		if (ec_strvec_set(strvec, idx, elt->str) < 0)
			goto fail;
		elt = strvec->vec[idx];
	}

	if (elt->attrs != NULL)
		ec_dict_free(elt->attrs);

	elt->attrs = attrs;

	return 0;

fail:
	ec_dict_free(attrs);
	return -1;
}

int ec_strvec_cmp(const struct ec_strvec *strvec1,
		const struct ec_strvec *strvec2)
{
	size_t i;

	if (ec_strvec_len(strvec1) != ec_strvec_len(strvec2))
		return -1;

	for (i = 0; i < ec_strvec_len(strvec1); i++) {
		if (strcmp(ec_strvec_val(strvec1, i),
				ec_strvec_val(strvec2, i)))
			return -1;
	}

	return 0;
}

typedef enum {
	SPACE,
	SINGLE_QUOTE,
	DOUBLE_QUOTE,
	BACKSLASH,
	POUND,
	OTHER,
} char_class_t;

typedef enum {
	START,
	IN_WORD,
	ESCAPING,
	ESCAPING_QUOTED,
	IN_DOUBLE_QUOTES,
	IN_SINGLE_QUOTES,
	IN_COMMENT,
} lexer_state_t;

static char_class_t get_char_class(char c)
{
	switch (c) {
	case '\'':
		return SINGLE_QUOTE;
	case '"':
		return DOUBLE_QUOTE;
	case '\\':
		return BACKSLASH;
	case '#':
		return POUND;
	default:
		if (isspace(c))
			return SPACE;
	}
	return OTHER;
}

static int sh_lex_set_attrs(struct ec_strvec *strvec, size_t idx,
			    size_t arg_start, size_t arg_end)
{
	struct ec_dict *attrs = ec_dict();
	if (attrs == NULL)
		return -1;

	if (ec_dict_set(attrs, EC_STRVEC_ATTR_START,
			(void *)(uintptr_t)arg_start, NULL) < 0)
		goto fail;
	if (ec_dict_set(attrs, EC_STRVEC_ATTR_END,
			(void *)(uintptr_t)arg_end, NULL) < 0)
		goto fail;

	return ec_strvec_set_attrs(strvec, idx, attrs);
fail:
	ec_dict_free(attrs);
	return -1;
}

struct ec_strvec *
ec_strvec_sh_lex_str(const char *str, ec_strvec_flag_t flags,
		     char *missing_quote)
{
	struct ec_strvec *strvec = NULL;
	lexer_state_t state = START;
	/* Weird, but we need an empty string to report as having trailing
	 * space when EC_STRVEC_SHLEX_KEEP_TRAILING_SPACE is set in flags. */
	bool trailing_space = true;
	size_t t, i, arg_start;
	char token[BUFSIZ];
	char c, quote;

#define append(buffer, position, character) \
	do { \
		buffer[position++] = character; \
		if (position >= sizeof(buffer)) { \
			errno = ENOBUFS; \
			goto fail; \
		} \
	} while (0)

	if (str == NULL) {
		errno = EINVAL;
		goto fail;
	}

	strvec = ec_strvec();
	if (strvec == NULL)
		goto fail;

	t = 0;
	quote = '\0';
	arg_start = 0;

	for (i = 0; i < strlen(str); i++) {
		c = str[i];

		char_class_t cls = get_char_class(c);

		switch (state) {
		case START:
			switch (cls) {
			case SPACE:
				break;
			case POUND:
				state = IN_COMMENT;
				break;
			case DOUBLE_QUOTE:
				state = IN_DOUBLE_QUOTES;
				quote = c;
				break;
			case SINGLE_QUOTE:
				state = IN_SINGLE_QUOTES;
				quote = c;
				break;
			case BACKSLASH:
				state = ESCAPING;
				break;
			default:
				/* start a new token */
				state = IN_WORD;
				append(token, t, c);
				break;
			}
			trailing_space = cls == SPACE;
			arg_start = i;
			break;
		case IN_WORD:
			switch (cls) {
			case SPACE:
				/* end of token */
				quote = '\0';
				token[t] = '\0';
				if (ec_strvec_add(strvec, token) < 0)
					goto fail;
				if (sh_lex_set_attrs(strvec,
						     ec_strvec_len(strvec) - 1,
						     arg_start, i) < 0)
					goto fail;
				state = START;
				trailing_space = true;
				arg_start = i;
				t = 0;
				break;
			case DOUBLE_QUOTE:
				state = IN_DOUBLE_QUOTES;
				break;
			case SINGLE_QUOTE:
				state = IN_SINGLE_QUOTES;
				break;
			case BACKSLASH:
				state = ESCAPING;
				break;
			default:
				append(token, t, c);
				break;
			}
			break;
		case ESCAPING:
			state = IN_WORD;
			append(token, t, c);
			break;
		case ESCAPING_QUOTED:
			state = IN_DOUBLE_QUOTES;
			append(token, t, c);
			break;
		case IN_DOUBLE_QUOTES:
			switch (cls) {
			case DOUBLE_QUOTE:
				state = IN_WORD;
				break;
			case BACKSLASH:
				state = ESCAPING_QUOTED;
				break;
			default:
				append(token, t, c);
				break;
			}
			break;
		case IN_SINGLE_QUOTES:
			switch (cls) {
			case SINGLE_QUOTE:
				state = IN_WORD;
				break;
			default:
				append(token, t, c);
				break;
			}
			break;
		case IN_COMMENT:
			if (c == '\n' || c == '\r')
				state = START;
			break;
		}
	}

#undef append
	switch (state) {
	case START:
		/* fallthrough */
	case IN_WORD:
		/* fallthrough */
	case IN_COMMENT:
		break;
	default:
		if (missing_quote != NULL) {
			*missing_quote = quote;
		}
		if (flags & EC_STRVEC_STRICT) {
			errno = EBADMSG;
			goto fail;
		}
		state = IN_WORD;
	}
	if (state == IN_WORD && t > 0) {
		token[t] = '\0';
		if (ec_strvec_add(strvec, token) < 0)
			goto fail;
		if (sh_lex_set_attrs(strvec, ec_strvec_len(strvec) - 1,
				     arg_start, i) < 0)
			goto fail;
	} else if (trailing_space && (flags & EC_STRVEC_TRAILSP)) {
		if (ec_strvec_add(strvec, "") < 0)
			goto fail;
		if (sh_lex_set_attrs(strvec, ec_strvec_len(strvec) - 1,
				     arg_start + 1, i + 1) < 0)
			goto fail;
	}

	return strvec;

fail:
	ec_strvec_free(strvec);
	return NULL;
}

static int
cmp_vec_elt(const void *p1, const void *p2, void *arg)
{
	int (*str_cmp)(const char *s1, const char *s2) = arg;
	const struct ec_strvec_elt * const *e1 = p1, * const *e2 = p2;

	return str_cmp((*e1)->str, (*e2)->str);
}

void ec_strvec_sort(struct ec_strvec *strvec,
	int (*str_cmp)(const char *s1, const char *s2))
{
	if (str_cmp == NULL)
		str_cmp = strcmp;
	qsort_r(strvec->vec, ec_strvec_len(strvec),
		sizeof(*strvec->vec), cmp_vec_elt, str_cmp);
}

void ec_strvec_dump(FILE *out, const struct ec_strvec *strvec)
{
	size_t i;

	if (strvec == NULL) {
		fprintf(out, "none\n");
		return;
	}

	fprintf(out, "strvec (len=%zu) [", strvec->len);
	for (i = 0; i < ec_strvec_len(strvec); i++) {
		if (i == 0)
			fprintf(out, "%s", strvec->vec[i]->str);
		else
			fprintf(out, ", %s", strvec->vec[i]->str);
	}
	fprintf(out, "]\n");

}

/* LCOV_EXCL_START */
static int ec_strvec_testcase(void)
{
	struct ec_strvec *strvec = NULL;
	struct ec_strvec *strvec2 = NULL;
	const struct ec_dict *const_attrs = NULL;
	struct ec_dict *attrs = NULL;
	FILE *f = NULL;
	char *buf = NULL;
	size_t buflen = 0;
	int testres = 0;
	char quote;

	strvec = ec_strvec();
	if (strvec == NULL) {
		EC_TEST_ERR("cannot create strvec\n");
		goto fail;
	}
	if (ec_strvec_len(strvec) != 0) {
		EC_TEST_ERR("bad strvec len (0)\n");
		goto fail;
	}
	if (ec_strvec_add(strvec, "0") < 0) {
		EC_TEST_ERR("cannot add (0) in strvec\n");
		goto fail;
	}
	if (ec_strvec_len(strvec) != 1) {
		EC_TEST_ERR("bad strvec len (1)\n");
		goto fail;
	}
	if (ec_strvec_add(strvec, "1") < 0) {
		EC_TEST_ERR("cannot add (1) in strvec\n");
		goto fail;
	}
	if (ec_strvec_len(strvec) != 2) {
		EC_TEST_ERR("bad strvec len (2)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec, 0), "0")) {
		EC_TEST_ERR("invalid element in strvec (0)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec, 1), "1")) {
		EC_TEST_ERR("invalid element in strvec (1)\n");
		goto fail;
	}
	if (ec_strvec_val(strvec, 2) != NULL) {
		EC_TEST_ERR("strvec val should be NULL\n");
		goto fail;
	}

	strvec2 = ec_strvec_dup(strvec);
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec2\n");
		goto fail;
	}
	if (ec_strvec_len(strvec2) != 2) {
		EC_TEST_ERR("bad strvec2 len (2)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec2, 0), "0")) {
		EC_TEST_ERR("invalid element in strvec2 (0)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec2, 1), "1")) {
		EC_TEST_ERR("invalid element in strvec2 (1)\n");
		goto fail;
	}
	if (ec_strvec_val(strvec2, 2) != NULL) {
		EC_TEST_ERR("strvec2 val should be NULL\n");
		goto fail;
	}
	ec_strvec_free(strvec2);

	strvec2 = ec_strvec_ndup(strvec, 0, 0);
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec2\n");
		goto fail;
	}
	if (ec_strvec_len(strvec2) != 0) {
		EC_TEST_ERR("bad strvec2 len (0)\n");
		goto fail;
	}
	if (ec_strvec_val(strvec2, 0) != NULL) {
		EC_TEST_ERR("strvec2 val should be NULL\n");
		goto fail;
	}
	ec_strvec_free(strvec2);

	strvec2 = ec_strvec_ndup(strvec, 1, 1);
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec2\n");
		goto fail;
	}
	if (ec_strvec_len(strvec2) != 1) {
		EC_TEST_ERR("bad strvec2 len (1)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec2, 0), "1")) {
		EC_TEST_ERR("invalid element in strvec2 (1)\n");
		goto fail;
	}
	if (ec_strvec_val(strvec2, 1) != NULL) {
		EC_TEST_ERR("strvec2 val should be NULL\n");
		goto fail;
	}
	ec_strvec_free(strvec2);

	strvec2 = ec_strvec_ndup(strvec, 3, 1);
	if (strvec2 != NULL) {
		EC_TEST_ERR("strvec2 should be NULL\n");
		goto fail;
	}
	ec_strvec_free(strvec2);

	strvec2 = EC_STRVEC("0", "1");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_strvec_cmp(strvec, strvec2) == 0,
		"strvec and strvec2 should be equal\n");
	ec_strvec_free(strvec2);

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_strvec_dump(f, strvec);
	fclose(f);
	f = NULL;
	testres |= EC_TEST_CHECK(
		strstr(buf, "strvec (len=2) [0, 1]"), "bad dump\n");
	free(buf);
	buf = NULL;

	ec_strvec_del_last(strvec);
	strvec2 = EC_STRVEC("0");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_strvec_cmp(strvec, strvec2) == 0,
		"strvec and strvec2 should be equal\n");
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_strvec_dump(f, NULL);
	fclose(f);
	f = NULL;
	testres |= EC_TEST_CHECK(
		strstr(buf, "none"), "bad dump\n");
	free(buf);
	buf = NULL;

	ec_strvec_free(strvec);

	strvec = EC_STRVEC("e", "a", "f", "d", "b", "c");
	if (strvec == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	attrs = ec_dict();
	if (attrs == NULL) {
		EC_TEST_ERR("cannot create attrs\n");
		goto fail;
	}
	if (ec_dict_set(attrs, "key", "value", NULL) < 0) {
		EC_TEST_ERR("cannot set attr\n");
		goto fail;
	}
	if (ec_strvec_set_attrs(strvec, 1, attrs) < 0) {
		attrs = NULL;
		EC_TEST_ERR("cannot set attrs in strvec\n");
		goto fail;
	}
	attrs = NULL;

	ec_strvec_sort(strvec, NULL);

	/* attrs are now at index 0 after sorting */
	const_attrs = ec_strvec_get_attrs(strvec, 0);
	if (const_attrs == NULL) {
		EC_TEST_ERR("cannot get attrs\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_dict_has_key(const_attrs, "key"), "cannot get attrs key\n");

	strvec2 = EC_STRVEC("a", "b", "c", "d", "e", "f");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_strvec_cmp(strvec, strvec2) == 0,
		"strvec and strvec2 should be equal\n");
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	/* lexing */
	strvec = ec_strvec_sh_lex_str("  a    b\tc d   # comment",
		EC_STRVEC_STRICT, NULL);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b", "c", "d");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_strvec_cmp(strvec, strvec2) == 0,
		"strvec and strvec2 should be equal\n");
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	strvec = ec_strvec_sh_lex_str("  a  b   c  d  ",
		EC_STRVEC_TRAILSP, NULL);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b", "c", "d", "");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_strvec_cmp(strvec, strvec2) == 0,
		"strvec and strvec2 should be equal\n");
	for (unsigned i = 0; i < ec_strvec_len(strvec); i++) {
		const_attrs = ec_strvec_get_attrs(strvec, i);
		testres |= EC_TEST_CHECK(const_attrs != NULL,
			"attrs should not be NULL\n");
		if (const_attrs == NULL)
			continue;
		errno = 0;
		unsigned s = (uintptr_t)ec_dict_get(const_attrs, EC_STRVEC_ATTR_START);
		unsigned e = (uintptr_t)ec_dict_get(const_attrs, EC_STRVEC_ATTR_END);
		testres |= EC_TEST_CHECK(errno == 0, "");
		switch (i) {
		case 0:
			testres |= EC_TEST_CHECK(s == 2 && e == 3, "");
			break;
		case 1:
			testres |= EC_TEST_CHECK(s == 5 && e == 6, "");
			break;
		case 2:
			testres |= EC_TEST_CHECK(s == 9 && e == 10, "");
			break;
		case 3:
			testres |= EC_TEST_CHECK(s == 12 && e == 13, "");
			break;
		case 4:
			testres |= EC_TEST_CHECK(s == 15 && e == 16, "");
			break;
		}
	}
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	strvec = ec_strvec_sh_lex_str("a  b  'c  d' ", EC_STRVEC_STRICT, NULL);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b", "c  d");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_strvec_cmp(strvec, strvec2) == 0,
		"strvec and strvec2 should be equal\n");
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	strvec = ec_strvec_sh_lex_str("a  b\\ e  \"c \\\" d\" ",
		EC_STRVEC_STRICT, NULL);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b e", "c \" d");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_strvec_cmp(strvec, strvec2) == 0,
		"strvec and strvec2 should be equal\n");
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	strvec = ec_strvec_sh_lex_str("a  b  'c  d ", EC_STRVEC_STRICT, NULL);
	if (strvec != NULL) {
		testres |= EC_TEST_CHECK(strvec == NULL,
			"shlex should have failed\n");
		ec_strvec_free(strvec);
		strvec = NULL;
	} else {
		testres |= EC_TEST_CHECK(errno == EBADMSG,
			"ec_strvec_shlex_str should report EBADMSG\n");
	}

	quote = '\0';
	strvec = ec_strvec_sh_lex_str("a  'b'  'c  d ",
		EC_STRVEC_TRAILSP, &quote);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b", "c  d ");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_strvec_cmp(strvec, strvec2) == 0,
		"strvec and strvec2 should be equal\n");
	testres |= EC_TEST_CHECK(quote == '\'',
		"missing quote should be '\n");
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	quote = '\0';
	strvec = ec_strvec_sh_lex_str("a  'b'\"x\"  'c  d' ",
		EC_STRVEC_TRAILSP, &quote);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "bx", "c  d", "");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_strvec_cmp(strvec, strvec2) == 0,
		"strvec and strvec2 should be equal\n");
	testres |= EC_TEST_CHECK(quote == '\0',
		"there should be no missing quote\n");
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	return testres;

fail:
	if (f != NULL)
		fclose(f);
	ec_dict_free(attrs);
	ec_strvec_free(strvec);
	ec_strvec_free(strvec2);
	free(buf);

	return -1;
}

static struct ec_test ec_node_str_test = {
	.name = "strvec",
	.test = ec_strvec_testcase,
};

EC_TEST_REGISTER(ec_node_str_test);
/* LCOV_EXCL_STOP */
