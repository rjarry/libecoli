/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <ecoli/murmurhash.h>

uint32_t ec_murmurhash3(const void *key, int len, uint32_t seed)
{
	const uint8_t *data = (const uint8_t *)key;
	const uint8_t *tail;
	const int nblocks = len / 4;
	uint32_t h1 = seed;
	uint32_t k1;
	const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);
	int i;

	for (i = -nblocks; i; i++) {
		k1 = blocks[i];

		h1 = ec_murmurhash3_add32(h1, k1);
		h1 = ec_murmurhash3_mix32(h1);
	}

	tail = (const uint8_t *)(data + nblocks * 4);
	k1 = 0;

	switch(len & 3) {
	case 3: k1 ^= tail[2] << 16; /* fallthrough */
	case 2: k1 ^= tail[1] << 8; /* fallthrough */
	case 1: k1 ^= tail[0];
		h1 = ec_murmurhash3_add32(h1, k1);
	};

	/* finalization */
	h1 ^= len;
	h1 = ec_murmurhash3_fmix32(h1);
	return h1;
}
