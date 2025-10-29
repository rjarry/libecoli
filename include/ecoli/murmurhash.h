/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_murmurhash Murmurhash
 * @{
 *
 * @brief Hash calculation using murmurhash algorithm
 *
 * MurmurHash3 is a hash implementation that was written by Austin Appleby, and
 * is placed in the public domain. The author hereby disclaims copyright to this
 * source code.
 */

#pragma once

#include <stdint.h>

/** Hash rotation */
static inline uint32_t ec_murmurhash_rotl32(uint32_t x, int8_t r)
{
	return (x << r) | (x >> (32 - r));
}

/** Add 32-bit to the hash */
static inline uint32_t ec_murmurhash3_add32(uint32_t h, uint32_t data)
{
	data *= 0xcc9e2d51;
	data = ec_murmurhash_rotl32(data, 15);
	data *= 0x1b873593;
	h ^= data;
	return h;
}

/** Intermediate mix */
static inline uint32_t ec_murmurhash3_mix32(uint32_t h)
{
	h = ec_murmurhash_rotl32(h, 13);
	h = h * 5 + 0xe6546b64;
	return h;
}

/** Final mix: force all bits of a hash block to avalanche */
static inline uint32_t ec_murmurhash3_fmix32(uint32_t h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

/**
 * Calculate a 32-bit murmurhash3
 *
 * @param key
 *   The key (the unaligned variable-length array of bytes).
 * @param len
 *   The length of the key, counting by bytes.
 * @param seed
 *   Can be any 4-byte value initialization value.
 * @return
 *   A 32-bit hash.
 */
uint32_t ec_murmurhash3(const void *key, int len, uint32_t seed);

/** @} */
