/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_vec Vectors
 * @{
 *
 * @brief Helpers for vectors manipulation.
 *
 * The ec_vec API provide helpers to manipulate vectors of objects
 * of any kind.
 */

#pragma once

#include <stdint.h>
#include <sys/types.h>

/**
 * Custom free callback.
 *
 * If NULL, default does nothing
 */
typedef void (*ec_vec_elt_free_t)(void *ptr);

/**
 * Custom copy callback
 *
 * If NULL, default is: `memcpy(dst, src, vec->elt_size)`
 */
typedef void (*ec_vec_elt_copy_t)(void *dst, void *src);

/** Create a new vector. */
struct ec_vec *ec_vec(size_t elt_size, size_t size, ec_vec_elt_copy_t copy, ec_vec_elt_free_t free);

/** Add reference to a vector. */
int ec_vec_add_by_ref(struct ec_vec *vec, void *ptr);

/** Add opaque element to a vector. */
int ec_vec_add_ptr(struct ec_vec *vec, void *elt);

/** Add uint8_t value to a vector. */
int ec_vec_add_u8(struct ec_vec *vec, uint8_t elt);
/** Add uint16_t value to a vector. */
int ec_vec_add_u16(struct ec_vec *vec, uint16_t elt);
/** Add uint32_t value to a vector. */
int ec_vec_add_u32(struct ec_vec *vec, uint32_t elt);
/** Add uint64_t value to a vector. */
int ec_vec_add_u64(struct ec_vec *vec, uint64_t elt);

/** Get element located at an offset. */
int ec_vec_get(void *ptr, const struct ec_vec *vec, size_t idx);

/** Duplicate a vector. */
struct ec_vec *ec_vec_dup(const struct ec_vec *vec);
/** Duplicate a portion of a vector. */
struct ec_vec *ec_vec_ndup(const struct ec_vec *vec, size_t off, size_t len);
/** Free a vector and all its contents. */
void ec_vec_free(struct ec_vec *vec);

/** Get the size of a vector. */
__attribute__((pure)) size_t ec_vec_len(const struct ec_vec *vec);

/** @} */
