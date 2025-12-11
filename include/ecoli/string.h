/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_string String manipulation
 * @{
 *
 * @brief Helpers for strings manipulation.
 */

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** count the number of identical chars at the beginning of 2 strings */
size_t ec_strcmp_count(const char *s1, const char *s2);

/** return 1 if 's' starts with 'beginning' */
int ec_str_startswith(const char *s, const char *beginning);

/** return true if string is only composed of spaces (' ', '\n', ...) */
bool ec_str_is_space(const char *s);

/**
 * Parse a string for a signed integer.
 *
 * @param str
 *   The string to parse.
 * @param base
 *   The base (0 means "guess").
 * @param min
 *   The minimum allowed value.
 * @param max
 *   The maximum allowed value.
 * @param val
 *   The pointer to the value to be set on success.
 * @return
 *   On success, return 0. Else, return -1 and set errno.
 */
int ec_str_parse_llint(const char *str, unsigned int base, int64_t min, int64_t max, int64_t *val);

/**
 * Parse a string for an unsigned integer.
 *
 * @param str
 *   The string to parse.
 * @param base
 *   The base (0 means "guess").
 * @param min
 *   The minimum allowed value.
 * @param max
 *   The maximum allowed value.
 * @param val
 *   The pointer to the value to be set on success.
 * @return
 *   On success, return 0. Else, return -1 and set errno.
 */
int ec_str_parse_ullint(
	const char *str,
	unsigned int base,
	uint64_t min,
	uint64_t max,
	uint64_t *val
);

/**
 * Quote a string, escaping nested quotes.
 *
 * @param str
 *   The string to quote.
 * @param quote
 *   The quote character to use: usually " or ' but can be anything. If 0,
 *   select between " or ' automatically.
 * @param force
 *   If true, always add quotes, else add them only if the string contains spaces or quotes.
 * @return
 *   An allocated string, that must be freed by the caller using free().
 */
char *ec_str_quote(const char *str, char quote, bool force);

/**
 * Wrap a text to a maximum number of columns.
 *
 * @param str
 *   The input text.
 * @param max_cols
 *   The maximum number of columns.
 * @param start_off
 *   The number of already consumed columns on the first line, filled with padding in other lines.
 * @return
 *   An allocated string containing the wrapped text. It must be freed by the user using free().
 */
char *ec_str_wrap(const char *str, size_t max_cols, size_t start_off);

/** @} */
