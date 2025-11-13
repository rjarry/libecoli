/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli/assert.h>
#include <ecoli/string.h>

/* count the number of identical chars at the beginning of 2 strings */
size_t ec_strcmp_count(const char *s1, const char *s2)
{
	size_t i = 0;

	while (s1[i] && s2[i] && s1[i] == s2[i])
		i++;

	return i;
}

int ec_str_startswith(const char *s, const char *beginning)
{
	size_t len;

	len = ec_strcmp_count(s, beginning);
	if (beginning[len] == '\0')
		return 1;

	return 0;
}

bool ec_str_is_space(const char *s)
{
	while (*s) {
		if (!isspace(*s))
			return false;
		s++;
	}
	return true;
}

int ec_str_parse_llint(const char *str, unsigned int base, int64_t min, int64_t max, int64_t *val)
{
	char *endptr;
	int save_errno = errno;

	errno = 0;
	*val = strtoll(str, &endptr, base);

	if ((errno == ERANGE && (*val == LLONG_MAX || *val == LLONG_MIN))
	    || (errno != 0 && *val == 0))
		return -1;

	if (*val < min) {
		errno = ERANGE;
		return -1;
	}

	if (*val > max) {
		errno = ERANGE;
		return -1;
	}

	if (*endptr != 0) {
		errno = EINVAL;
		return -1;
	}

	errno = save_errno;
	return 0;
}

int ec_str_parse_ullint(
	const char *str,
	unsigned int base,
	uint64_t min,
	uint64_t max,
	uint64_t *val
)
{
	char *endptr;
	int save_errno = errno;

	/* since a negative input is silently converted to a positive
	 * one by strtoull(), first check that it is positive */
	if (strchr(str, '-'))
		return -1;

	errno = 0;
	*val = strtoull(str, &endptr, base);

	if ((errno == ERANGE && *val == ULLONG_MAX) || (errno != 0 && *val == 0))
		return -1;

	if (*val < min)
		return -1;

	if (*val > max)
		return -1;

	if (*endptr != 0)
		return -1;

	errno = save_errno;
	return 0;
}

char *ec_str_quote(const char *str, char quote)
{
	size_t len = 3; /* start/end quotes + \0 */
	const char *s;
	char *out, *o;
	char c;

	if (quote == 0) {
		if (strchr(str, '"') == NULL)
			quote = '"';
		else
			quote = '\'';
	}

	for (s = str, c = *s; c != '\0'; s++, c = *s) {
		if (c == quote || c == '\\')
			len += 2;
		else if (!isprint(c) && c != '\n')
			len += 4;
		else
			len += 1;
	}

	out = malloc(len);
	if (out == NULL)
		return NULL;

	o = out;
	*o++ = quote;

	for (s = str, c = *s; c != '\0'; s++, c = *s) {
		if (c == quote) {
			*o++ = '\\';
			*o++ = quote;
		} else if (c == '\\') {
			*o++ = '\\';
			*o++ = '\\';
		} else if (!isprint(c) && c != '\n') {
			char buf[5];

			snprintf(buf, sizeof(buf), "\\x%2.2x", c);
			memcpy(o, buf, 4);
			o += 4;
		} else {
			*o++ = c;
		}
	}

	*o++ = quote;
	*o = '\0';

	return out;
}

struct wrap_state {
	size_t line_no; /* current line number */
	size_t line_length; /* current line length */
	size_t start_off; /* start offset for first line (implies padding for other lines) */
	size_t max_cols; /* wrap text at max_cols */
	bool new_para; /* true if a new paragraph is needed */
	char *output; /* output buffer */
	size_t len; /* current buffer length */
	size_t size; /* current buffer size */
};

static int append_token(struct wrap_state *state, const char *token, size_t token_len)
{
	size_t written = state->line_length - state->start_off;
	int ret;

	/* allocate a larger buffer if needed (the "5" below is a margin above the worst case) */
	if (state->output == NULL || state->size - state->len < token_len + state->start_off + 5) {
		size_t new_size;
		char *tmp;

		new_size = state->size == 0 ? 256 : state->size * 2;
		tmp = malloc(new_size);
		if (tmp == NULL)
			return -1;
		memcpy(tmp, state->output, state->len);
		free(state->output);
		state->output = tmp;
		state->size = new_size;
	}

	/* add new line */
	if (state->line_length + token_len + 1 > state->max_cols && written > 0) {
		ret = sprintf(
			&state->output[state->len],
			"\n%s%*s",
			state->new_para ? "\n" : "",
			(int)state->start_off,
			""
		);
		if (ret < 0)
			return -1;

		state->len += ret;
		state->line_length = state->start_off;
		state->line_no++;
		state->new_para = false;
		written = 0;
	}

	/* add token */
	ret = sprintf(
		&state->output[state->len], "%s%.*s", written > 0 ? " " : "", (int)token_len, token
	);
	if (ret < 0)
		return -1;

	state->len += ret;
	state->line_length += token_len + !!(written > 0);

	return 0;
}

char *ec_str_wrap(const char *str, size_t max_cols, size_t start_off)
{
	struct wrap_state state = {
		.line_no = 0,
		.line_length = start_off,
		.start_off = start_off,
		.max_cols = max_cols,
		.new_para = false,
		.output = NULL,
	};
	const char *start;
	size_t len;
	size_t cr;

	while (*str != '\0') {
		cr = 0;

		while (isspace(*str)) {
			if (*str == '\n')
				cr++;
			str++;
		}

		if (state.line_no != 0 && cr >= 2)
			state.new_para = true;

		start = str;

		while (!isspace(*str) && *str != '\0')
			str++;
		len = str - start;

		if (len > 0 && append_token(&state, start, len) < 0)
			goto fail;
	}

	if (state.output == NULL)
		return strdup("");

	return state.output;

fail:
	free(state.output);
	return NULL;
}
