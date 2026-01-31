/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_strvec String vector
 * @{
 *
 * @brief Helpers for strings vectors manipulation.
 *
 * The ::ec_strvec API provides helpers to manipulate string vectors.
 * When duplicating vectors, the strings are not duplicated in memory,
 * a reference counter is used.
 */

#pragma once
#include <stdio.h>

/** String vector */
struct ec_strvec;

/**
 * Allocate a new empty string vector.
 *
 * @return
 *   The new strvec object, or NULL on error (errno is set).
 */
struct ec_strvec *ec_strvec(void);

/**
 * Allocate a new string vector.
 *
 * The string vector is initialized with the list of const strings
 * passed as arguments.
 *
 * @return
 *   The new strvec object, or NULL on error (errno is set).
 */
#define EC_STRVEC(args...)                                                                         \
	({                                                                                         \
		const char *_arr[] = {args};                                                       \
		ec_strvec_from_array(_arr, EC_COUNT_OF(_arr));                                     \
	})
/**
 * Allocate a new string vector.
 *
 * The string vector is initialized with the array of const strings
 * passed as arguments.
 *
 * @param strarr
 *   The array of const strings.
 * @param n
 *   The number of strings in the array.
 * @return
 *   The new strvec object, or NULL on error (errno is set).
 */
struct ec_strvec *ec_strvec_from_array(const char *const *strarr, size_t n);

/**
 * Options for ec_strvec_sh_lex_str().
 */
typedef enum {
	/**
	 * Fail if a quote is not closed properly or if the provided
	 * string ends with an unterminated escape sequence.
	 */
	EC_STRVEC_STRICT = 0x1,
	/**
	 * If there is trailing white space, add an empty element to the
	 * output string vector.
	 */
	EC_STRVEC_TRAILSP = 0x2,
} ec_strvec_flag_t;

/**
 * All elements of the vector returned by ec_strvec_sh_lex_str() have two
 * attributes accessible via ec_strvec_get_attrs().
 */
/** The start index in the original line passed to ec_strvec_sh_lex_str(). */
#define EC_STRVEC_ATTR_START "start"
/** The end index in the original line passed to ec_strvec_sh_lex_str(). */
#define EC_STRVEC_ATTR_END "end"

/**
 * Split a string into multiple tokens following basic shell lexing rules.
 *
 * @param str
 *   The string to split.
 * @param flags
 *   Options for controlling behavior.
 * @param unclosed_quote
 *   If not NULL and if the provided string has an unterminated quote. The
 *   opening quote will be stored here.
 *
 * @return
 *   The new strvec object, or NULL on error (errno is set).
 */
struct ec_strvec *
ec_strvec_sh_lex_str(const char *str, ec_strvec_flag_t flags, char *unclosed_quote);

/**
 * Set a string in the vector at specified index.
 *
 * @param strvec
 *   The pointer to the string vector.
 * @param idx
 *   The index of the string to set.
 * @param s
 *   The string to be set.
 * @return
 *   0 on success or -1 on error (errno is set).
 */
int ec_strvec_set(struct ec_strvec *strvec, size_t idx, const char *s);

/**
 * Add a string in a vector.
 *
 * @param strvec
 *   The pointer to the string vector.
 * @param s
 *   The string to be added at the end of the vector.
 * @return
 *   0 on success or -1 on error (errno is set).
 */
int ec_strvec_add(struct ec_strvec *strvec, const char *s);

/**
 * Delete the last entry in the string vector.
 *
 * @param strvec
 *   The pointer to the string vector.
 * @return
 *   0 on success or -1 on error (errno is set).
 */
int ec_strvec_del_last(struct ec_strvec *strvec);

/**
 * Duplicate a string vector.
 *
 * Attributes are duplicated if any.
 *
 * @param strvec
 *   The pointer to the string vector.
 * @return
 *   The duplicated strvec object, or NULL on error (errno is set).
 */
struct ec_strvec *ec_strvec_dup(const struct ec_strvec *strvec);

/**
 * Duplicate a part of a string vector.
 *
 * Attributes are duplicated if any.
 *
 * @param strvec
 *   The pointer to the string vector.
 * @param off
 *   The index of the first string to duplicate.
 * @param len
 *   The number of strings to duplicate.
 * @return
 *   The duplicated strvec object, or NULL on error (errno is set).
 */
struct ec_strvec *ec_strvec_ndup(const struct ec_strvec *strvec, size_t off, size_t len);

/**
 * Free a string vector.
 *
 * @param strvec
 *   The pointer to the string vector.
 */
void ec_strvec_free(struct ec_strvec *strvec);

/**
 * Get the length of a string vector.
 *
 * @param strvec
 *   The pointer to the string vector.
 * @return
 *   The length of the vector.
 */
size_t ec_strvec_len(const struct ec_strvec *strvec);

/**
 * Get a string element from a vector.
 *
 * @param strvec
 *   The pointer to the string vector.
 * @param idx
 *   The index of the string to get.
 * @return
 *   The string stored at given index, or NULL on error (strvec is NULL
 *   or invalid index).
 */
const char *ec_strvec_val(const struct ec_strvec *strvec, size_t idx);

/**
 * Get the attributes of a vector element.
 *
 * @param strvec
 *   The pointer to the string vector.
 * @param idx
 *   The index of the string to get.
 * @return
 *   The read-only attributes (dictionary) of the string at specified
 *   index, or NULL if there is no attribute.
 */
const struct ec_dict *ec_strvec_get_attrs(const struct ec_strvec *strvec, size_t idx);

/**
 * Set the attributes of a vector element.
 *
 * @param strvec
 *   The pointer to the string vector.
 * @param idx
 *   The index of the string to get.
 * @param attrs
 *   The attributes to be set.
 * @return
 *   0 on success, -1 on error (errno is set). On error, attrs
 *   are freed and must not be used by the caller.
 */
int ec_strvec_set_attrs(struct ec_strvec *strvec, size_t idx, struct ec_dict *attrs);

/**
 * Compare two string vectors.
 *
 * @param strvec1
 *   The pointer to the first string vector.
 * @param strvec2
 *   The pointer to the second string vector.
 * @return
 *   0 if the string vectors are equal.
 */
int ec_strvec_cmp(const struct ec_strvec *strvec1, const struct ec_strvec *strvec2);

/**
 * Sort the string vector.
 *
 * Attributes are not compared.
 *
 * @param strvec
 *   The pointer to the first string vector.
 * @param str_cmp
 *   The sort function to use. If NULL, use strcmp.
 */
void ec_strvec_sort(struct ec_strvec *strvec, int (*str_cmp)(const char *s1, const char *s2));

/**
 * Dump a string vector.
 *
 * @param out
 *   The stream where the dump is sent.
 * @param strvec
 *   The pointer to the string vector.
 */
void ec_strvec_dump(FILE *out, const struct ec_strvec *strvec);

/** @} */
