/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_dict Dictionary
 * @{
 *
 * @brief Simple hash table API (string keys)
 *
 * This file provides functions to store objects in hash tables, using strings
 * as keys.
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>

/** Callback function for freeing dictionary elements. */
typedef void (*ec_dict_elt_free_t)(void *);

/** Dictionary (string key hash table). */
struct ec_dict;

/** Dictionary element reference. */
struct ec_dict_elt_ref;

/**
 * Create a hash table.
 *
 * @return
 *   The hash table, or NULL on error (errno is set).
 */
struct ec_dict *ec_dict(void);

/**
 * Get a value from the hash table.
 *
 * @param dict
 *   The hash table.
 * @param key
 *   The key string. Must not be NULL.
 * @return
 *   The element if it is found, or NULL on error (errno is set).
 *   In case of success but the element is NULL, errno is set to 0.
 *   If key is NULL, errno is set to EINVAL.
 */
void *ec_dict_get(const struct ec_dict *dict, const char *key);

/**
 * Check if the hash table contains this key.
 *
 * @param dict
 *   The hash table.
 * @param key
 *   The key string. Must not be NULL.
 * @return
 *   true if it contains the key, false otherwise.
 *   If key is NULL, false is returned and errno is set to EINVAL.
 */
bool ec_dict_has_key(const struct ec_dict *dict, const char *key);

/**
 * Delete an object from the hash table.
 *
 * @param dict
 *   The hash table.
 * @param key
 *   The key string. Must not be NULL.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_dict_del(struct ec_dict *dict, const char *key);

/**
 * Add/replace an object in the hash table.
 *
 * @param dict
 *   The hash table.
 * @param key
 *   The key string. Must not be NULL.
 * @param val
 *   The pointer to be saved in the hash table.
 * @param free_cb
 *   An optional pointer to a destructor function called when an
 *   object is destroyed (ec_dict_del() or ec_dict_free()).
 * @return
 *   0 on success, or -1 on error (errno is set).
 *   On error, the passed value is freed (free_cb(val) is called).
 */
int ec_dict_set(struct ec_dict *dict, const char *key, void *val, ec_dict_elt_free_t free_cb);

/**
 * Free a hash table and all its objects.
 *
 * @param dict
 *   The hash table.
 */
void ec_dict_free(struct ec_dict *dict);

/**
 * Get the length of a hash table.
 *
 * @param dict
 *   The hash table.
 * @return
 *   The length of the hash table.
 */
size_t ec_dict_len(const struct ec_dict *dict);

/**
 * Duplicate a hash table.
 *
 * A reference counter is shared between the clones of
 * hash tables so that the objects are freed only when
 * the last reference is destroyed.
 *
 * @param dict
 *   The hash table.
 * @return
 *   The duplicated hash table, or NULL on error (errno is set).
 */
struct ec_dict *ec_dict_dup(const struct ec_dict *dict);

/**
 * Dump a hash table.
 *
 * @param out
 *   The stream where the dump is sent.
 * @param dict
 *   The hash table.
 */
void ec_dict_dump(FILE *out, const struct ec_dict *dict);

/**
 * Iterate the elements in the hash table.
 *
 * The typical usage is as below:
 *
 * @code{.c}
 *	for (iter = ec_dict_iter(dict);
 *	     iter != NULL;
 *	     iter = ec_dict_iter_next(iter)) {
 *		printf("  %s: %p\n",
 *			ec_dict_iter_get_key(iter),
 *			ec_dict_iter_get_val(iter));
 *	}
 * @endcode
 *
 * @param dict
 *   The hash table.
 * @return
 *   An iterator element, or NULL if the dict is empty.
 */
struct ec_dict_elt_ref *ec_dict_iter(const struct ec_dict *dict);

/**
 * Make the iterator point to the next element in the hash table.
 *
 * @param iter
 *   The hash table iterator.
 * @return
 *   An iterator element, or NULL if there is no more element.
 */
struct ec_dict_elt_ref *ec_dict_iter_next(struct ec_dict_elt_ref *iter);

/**
 * Get a pointer to the key of the current element.
 *
 * @param iter
 *   The hash table iterator.
 * @return
 *   The current element key, or NULL if the iterator points to an
 *   invalid element.
 */
const char *ec_dict_iter_get_key(const struct ec_dict_elt_ref *iter);

/**
 * Get the value of the current element.
 *
 * @param iter
 *   The hash table iterator.
 * @return
 *   The current element value, or NULL if the iterator points to an
 *   invalid element.
 */
void *ec_dict_iter_get_val(const struct ec_dict_elt_ref *iter);

/** @} */
