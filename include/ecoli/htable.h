/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_htable Hash table
 * @{
 *
 * @brief Simple hash table API (any key)
 *
 * This file provides functions to store objects in hash tables,
 * using arbitrary data as keys.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef void (*ec_htable_elt_free_t)(void *);

struct ec_htable;
struct ec_htable_elt_ref;

/**
 * Create a hash table.
 *
 * @return
 *   The hash table, or NULL on error (errno is set).
 */
struct ec_htable *ec_htable(void);

/**
 * Get a value from the hash table.
 *
 * @param htable
 *   The hash table.
 * @param key
 *   The key.
 * @param key_len
 *   The key length.
 * @return
 *   The element if it is found, or NULL on error (errno is set).
 *   In case of success but the element is NULL, errno is set to 0.
 */
void *ec_htable_get(const struct ec_htable *htable, const void *key, size_t key_len);

/**
 * Check if the hash table contains this key.
 *
 * @param htable
 *   The hash table.
 * @param key
 *   The key.
 * @param key_len
 *   The key length.
 * @return
 *   true if it contains the key, else false.
 */
bool ec_htable_has_key(const struct ec_htable *htable, const void *key, size_t key_len);

/**
 * Delete an object from the hash table.
 *
 * @param htable
 *   The hash table.
 * @param key
 *   The key.
 * @param key_len
 *   The key length.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_htable_del(struct ec_htable *htable, const void *key, size_t key_len);

/**
 * Add/replace an object in the hash table.
 *
 * @param htable
 *   The hash table.
 * @param key
 *   The key.
 * @param key_len
 *   The key length.
 * @param val
 *   The pointer to be saved in the hash table.
 * @param free_cb
 *   An optional pointer to a destructor function called when an
 *   object is destroyed (ec_htable_del() or ec_htable_free()).
 * @return
 *   0 on success, or -1 on error (errno is set).
 *   On error, the passed value is freed (free_cb(val) is called).
 */
int ec_htable_set(
	struct ec_htable *htable,
	const void *key,
	size_t key_len,
	void *val,
	ec_htable_elt_free_t free_cb
);

/**
 * Free a hash table an all its objects.
 *
 * @param htable
 *   The hash table.
 */
void ec_htable_free(struct ec_htable *htable);

/**
 * Get the length of a hash table.
 *
 * @param htable
 *   The hash table.
 * @return
 *   The length of the hash table.
 */
size_t ec_htable_len(const struct ec_htable *htable);

/**
 * Duplicate a hash table
 *
 * A reference counter is shared between the clones of
 * hash tables so that the objects are freed only when
 * the last reference is destroyed.
 *
 * @param htable
 *   The hash table.
 * @return
 *   The duplicated hash table, or NULL on error (errno is set).
 */
struct ec_htable *ec_htable_dup(const struct ec_htable *htable);

/**
 * Force a seed for the hash function.
 * This function must be called *before* ec_init().
 * By default, a random value is determined during ec_init().
 *
 * @param seed
 *   The seed value.
 */
void ec_htable_force_seed(uint32_t seed);

/**
 * Dump a hash table.
 *
 * @param out
 *   The stream where the dump is sent.
 * @param htable
 *   The hash table.
 */
void ec_htable_dump(FILE *out, const struct ec_htable *htable);

/**
 * Iterate the elements in the hash table.
 *
 * The typical usage is as below:
 *
 *	// dump elements
 *	for (iter = ec_htable_iter(htable);
 *	     iter != NULL;
 *	     iter = ec_htable_iter_next(iter)) {
 *		printf("  %s: %p\n",
 *			ec_htable_iter_get_key(iter),
 *			ec_htable_iter_get_val(iter));
 *	}
 *
 * @param htable
 *   The hash table.
 * @return
 *   An iterator element, or NULL if the dict is empty.
 */
struct ec_htable_elt_ref *ec_htable_iter(const struct ec_htable *htable);

/**
 * Make the iterator point to the next element in the hash table.
 *
 * @param iter
 *   The hash table iterator.
 * @return
 *   An iterator element, or NULL there is no more element.
 */
struct ec_htable_elt_ref *ec_htable_iter_next(struct ec_htable_elt_ref *iter);

/**
 * Get the key of the current element.
 *
 * @param iter
 *   The hash table iterator.
 * @return
 *   The current element key, or NULL if the iterator points to an
 *   invalid element.
 */
const void *ec_htable_iter_get_key(const struct ec_htable_elt_ref *iter);

/**
 * Get the key length of the current element.
 *
 * @param iter
 *   The hash table iterator.
 * @return
 *   The current element key length, or 0 if the iterator points to an
 *   invalid element.
 */
size_t ec_htable_iter_get_key_len(const struct ec_htable_elt_ref *iter);

/**
 * Get the value of the current element.
 *
 * @param iter
 *   The hash table iterator.
 * @return
 *   The current element value, or NULL if the iterator points to an
 *   invalid element.
 */
void *ec_htable_iter_get_val(const struct ec_htable_elt_ref *iter);

/** @} */
