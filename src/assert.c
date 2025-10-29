/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <ecoli_assert.h>

void __ec_assert_print(bool expr, const char *expr_str, const char *format, ...)
{
	va_list ap;

	if (expr)
		return;

	va_start(ap, format);
	fprintf(stderr, "assertion failed: '%s' is false\n", expr_str);
	vfprintf(stderr, format, ap);
	va_end(ap);
	abort();
}
