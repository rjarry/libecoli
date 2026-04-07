/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_utils Utils
 * @{
 *
 * @brief Misc utils
 */

#pragma once

/**
 * Cast a variable into a type, ensuring its initial type first
 */
#define EC_CAST(x, old_type, new_type)                                                             \
	({                                                                                         \
		__typeof__(old_type) __x = (x);                                                    \
		(__typeof__(new_type))__x;                                                         \
	})

/**
 * Mark the end of the arguments list in some functions.
 */
#define EC_VA_END ((void *)1)

/**
 * Count number of elements in an array.
 */
#define EC_COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

/**
 * Mark a function or type as deprecated.
 */
#define EC_DEPRECATED(msg) __attribute__((deprecated(msg)))

/**
 * Emit a deprecation warning when a macro is expanded.
 */
#define EC_DEPRECATED_MACRO_(x) _Pragma(#x)
#define EC_DEPRECATED_MACRO(msg) EC_DEPRECATED_MACRO_(GCC warning msg)

/** @} */
