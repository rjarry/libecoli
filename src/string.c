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
