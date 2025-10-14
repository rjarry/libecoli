/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#include <ecoli_assert.h>
#include <ecoli_malloc.h>
#include <ecoli_string.h>

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

int ec_vasprintf(char **buf, const char *fmt, va_list ap)
{
	char dummy;
	int buflen, ret;
	va_list aq;

	va_copy(aq, ap);
	*buf = NULL;
	ret = vsnprintf(&dummy, 1, fmt, aq);
	va_end(aq);
	if (ret < 0)
		return ret;

	buflen = ret + 1;
	*buf = ec_malloc(buflen);
	if (*buf == NULL)
		return -1;

	va_copy(aq, ap);
	ret = vsnprintf(*buf, buflen, fmt, aq);
	va_end(aq);

	ec_assert_print(ret < buflen, "invalid return value for vsnprintf");
	if (ret < 0) {
		free(*buf);
		*buf = NULL;
		return -1;
	}

	return ret;
}

int ec_asprintf(char **buf, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = ec_vasprintf(buf, fmt, ap);
	va_end(ap);

	return ret;
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

int ec_str_parse_llint(const char *str, unsigned int base, int64_t min,
		int64_t max, int64_t *val)
{
	char *endptr;
	int save_errno = errno;

	errno = 0;
	*val = strtoll(str, &endptr, base);

	if ((errno == ERANGE && (*val == LLONG_MAX || *val == LLONG_MIN)) ||
			(errno != 0 && *val == 0))
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

int ec_str_parse_ullint(const char *str, unsigned int base, uint64_t min,
			uint64_t max, uint64_t *val)
{
	char *endptr;
	int save_errno = errno;

	/* since a negative input is silently converted to a positive
	 * one by strtoull(), first check that it is positive */
	if (strchr(str, '-'))
		return -1;

	errno = 0;
	*val = strtoull(str, &endptr, base);

	if ((errno == ERANGE && *val == ULLONG_MAX) ||
			(errno != 0 && *val == 0))
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
