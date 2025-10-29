/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_assert Assert
 * @{
 *
 * @brief Assertion helpers
 *
 * Helpers to check at runtime if a condition is true, or otherwise
 * either abort (exit program) or return an error.
 *
 * @internal
 */

#pragma once

#include <stdbool.h>

/**
 * Abort if the condition is false.
 *
 * If expression is false this macro will prints an error message to
 * standard error and terminates the program by calling abort(3).
 *
 * @param expr
 *   The expression to be checked.
 * @param args
 *   The format string, optionally followed by other arguments.
 *
 * @internal
 */
#define ec_assert_print(expr, args...) \
	__ec_assert_print(expr, #expr, args)

/**
 * Actual function invoked by ec_assert_print()
 *
 * @internal
 */
void __ec_assert_print(bool expr, const char *expr_str,
		const char *format, ...);

/**
 * Check a condition or return.
 *
 * If the condition is true, do nothing. If it is false, set
 * errno and return the specified value.
 *
 * @param cond
 *   The condition to test.
 * @param ret
 *   The value to return.
 * @param err
 *   The errno to set.
 *
 * @internal
 */
#define EC_CHECK_ARG(cond, ret, err) do {				\
		if (!(cond)) {						\
			errno = err;					\
			return ret;					\
		}							\
	} while(0)

/** @} */
