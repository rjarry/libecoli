/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdlib.h>
#include <string.h>

#include "test.h"

EC_LOG_TYPE_REGISTER(vec);

static void str_free(void *elt)
{
	char **s = elt;

	free(*s);
}

#define GOTO_FAIL                                                                                  \
	do {                                                                                       \
		EC_LOG(EC_LOG_ERR, "%s:%d: test failed\n", __FILE__, __LINE__);                    \
		goto fail;                                                                         \
	} while (0)

EC_TEST_MAIN()
{
	struct ec_vec *vec = NULL;
	struct ec_vec *vec2 = NULL;
	uint8_t val8;
	uint16_t val16;
	uint32_t val32;
	uint64_t val64;
	void *valp;
	char *vals;

	/* uint8_t vector */
	vec = ec_vec(sizeof(val8), 0, NULL, NULL);
	if (vec == NULL)
		GOTO_FAIL;

	if (ec_vec_add_u8(vec, 0) < 0)
		GOTO_FAIL;
	if (ec_vec_add_u8(vec, 1) < 0)
		GOTO_FAIL;
	if (ec_vec_add_u8(vec, 2) < 0)
		GOTO_FAIL;
	/* should fail */
	if (ec_vec_add_u16(vec, 3) == 0)
		GOTO_FAIL;
	if (ec_vec_add_u32(vec, 3) == 0)
		GOTO_FAIL;
	if (ec_vec_add_u64(vec, 3) == 0)
		GOTO_FAIL;
	if (ec_vec_add_ptr(vec, (void *)3) == 0)
		GOTO_FAIL;

	if (ec_vec_get(&val8, vec, 0) < 0)
		GOTO_FAIL;
	if (val8 != 0)
		GOTO_FAIL;
	if (ec_vec_get(&val8, vec, 1) < 0)
		GOTO_FAIL;
	if (val8 != 1)
		GOTO_FAIL;
	if (ec_vec_get(&val8, vec, 2) < 0)
		GOTO_FAIL;
	if (val8 != 2)
		GOTO_FAIL;

	/* duplicate the vector */
	vec2 = ec_vec_dup(vec);
	if (vec2 == NULL)
		GOTO_FAIL;
	if (ec_vec_get(&val8, vec2, 0) < 0)
		GOTO_FAIL;
	if (val8 != 0)
		GOTO_FAIL;
	if (ec_vec_get(&val8, vec2, 1) < 0)
		GOTO_FAIL;
	if (val8 != 1)
		GOTO_FAIL;
	if (ec_vec_get(&val8, vec2, 2) < 0)
		GOTO_FAIL;
	if (val8 != 2)
		GOTO_FAIL;

	ec_vec_free(vec2);
	vec2 = NULL;

	/* dup at offset 1 */
	vec2 = ec_vec_ndup(vec, 1, 2);
	if (vec2 == NULL)
		GOTO_FAIL;
	if (ec_vec_get(&val8, vec2, 0) < 0)
		GOTO_FAIL;
	if (val8 != 1)
		GOTO_FAIL;
	if (ec_vec_get(&val8, vec2, 1) < 0)
		GOTO_FAIL;
	if (val8 != 2)
		GOTO_FAIL;

	ec_vec_free(vec2);
	vec2 = NULL;

	/* len = 0, duplicate is empty */
	vec2 = ec_vec_ndup(vec, 2, 0);
	if (vec2 == NULL)
		GOTO_FAIL;
	if (ec_vec_get(&val8, vec2, 0) == 0)
		GOTO_FAIL;

	ec_vec_free(vec2);
	vec2 = NULL;

	/* bad dup args */
	vec2 = ec_vec_ndup(vec, 10, 1);
	if (vec2 != NULL)
		GOTO_FAIL;

	ec_vec_free(vec);
	vec = NULL;

	/* uint16_t vector */
	vec = ec_vec(sizeof(val16), 0, NULL, NULL);
	if (vec == NULL)
		GOTO_FAIL;

	if (ec_vec_add_u16(vec, 0) < 0)
		GOTO_FAIL;
	if (ec_vec_add_u16(vec, 1) < 0)
		GOTO_FAIL;
	if (ec_vec_add_u16(vec, 2) < 0)
		GOTO_FAIL;
	/* should fail */
	if (ec_vec_add_u8(vec, 3) == 0)
		GOTO_FAIL;

	if (ec_vec_get(&val16, vec, 0) < 0)
		GOTO_FAIL;
	if (val16 != 0)
		GOTO_FAIL;
	if (ec_vec_get(&val16, vec, 1) < 0)
		GOTO_FAIL;
	if (val16 != 1)
		GOTO_FAIL;
	if (ec_vec_get(&val16, vec, 2) < 0)
		GOTO_FAIL;
	if (val16 != 2)
		GOTO_FAIL;

	ec_vec_free(vec);
	vec = NULL;

	/* uint32_t vector */
	vec = ec_vec(sizeof(val32), 0, NULL, NULL);
	if (vec == NULL)
		GOTO_FAIL;

	if (ec_vec_add_u32(vec, 0) < 0)
		GOTO_FAIL;
	if (ec_vec_add_u32(vec, 1) < 0)
		GOTO_FAIL;
	if (ec_vec_add_u32(vec, 2) < 0)
		GOTO_FAIL;

	if (ec_vec_get(&val32, vec, 0) < 0)
		GOTO_FAIL;
	if (val32 != 0)
		GOTO_FAIL;
	if (ec_vec_get(&val32, vec, 1) < 0)
		GOTO_FAIL;
	if (val32 != 1)
		GOTO_FAIL;
	if (ec_vec_get(&val32, vec, 2) < 0)
		GOTO_FAIL;
	if (val32 != 2)
		GOTO_FAIL;

	ec_vec_free(vec);
	vec = NULL;

	/* uint64_t vector */
	vec = ec_vec(sizeof(val64), 0, NULL, NULL);
	if (vec == NULL)
		GOTO_FAIL;

	if (ec_vec_add_u64(vec, 0) < 0)
		GOTO_FAIL;
	if (ec_vec_add_u64(vec, 1) < 0)
		GOTO_FAIL;
	if (ec_vec_add_u64(vec, 2) < 0)
		GOTO_FAIL;

	if (ec_vec_get(&val64, vec, 0) < 0)
		GOTO_FAIL;
	if (val64 != 0)
		GOTO_FAIL;
	if (ec_vec_get(&val64, vec, 1) < 0)
		GOTO_FAIL;
	if (val64 != 1)
		GOTO_FAIL;
	if (ec_vec_get(&val64, vec, 2) < 0)
		GOTO_FAIL;
	if (val64 != 2)
		GOTO_FAIL;

	ec_vec_free(vec);
	vec = NULL;

	/* ptr vector */
	vec = ec_vec(sizeof(valp), 0, NULL, NULL);
	if (vec == NULL)
		GOTO_FAIL;

	if (ec_vec_add_ptr(vec, (void *)0) < 0)
		GOTO_FAIL;
	if (ec_vec_add_ptr(vec, (void *)1) < 0)
		GOTO_FAIL;
	if (ec_vec_add_ptr(vec, (void *)2) < 0)
		GOTO_FAIL;

	if (ec_vec_get(&valp, vec, 0) < 0)
		GOTO_FAIL;
	if (valp != (void *)0)
		GOTO_FAIL;
	if (ec_vec_get(&valp, vec, 1) < 0)
		GOTO_FAIL;
	if (valp != (void *)1)
		GOTO_FAIL;
	if (ec_vec_get(&valp, vec, 2) < 0)
		GOTO_FAIL;
	if (valp != (void *)2)
		GOTO_FAIL;

	ec_vec_free(vec);
	vec = NULL;

	/* string vector */
	vec = ec_vec(sizeof(valp), 0, NULL, str_free);
	if (vec == NULL)
		GOTO_FAIL;

	if (ec_vec_add_ptr(vec, strdup("0")) < 0)
		GOTO_FAIL;
	if (ec_vec_add_ptr(vec, strdup("1")) < 0)
		GOTO_FAIL;
	if (ec_vec_add_ptr(vec, strdup("2")) < 0)
		GOTO_FAIL;

	if (ec_vec_get(&vals, vec, 0) < 0)
		GOTO_FAIL;
	if (vals == NULL || strcmp(vals, "0"))
		GOTO_FAIL;
	if (ec_vec_get(&vals, vec, 1) < 0)
		GOTO_FAIL;
	if (vals == NULL || strcmp(vals, "1"))
		GOTO_FAIL;
	if (ec_vec_get(&vals, vec, 2) < 0)
		GOTO_FAIL;
	if (vals == NULL || strcmp(vals, "2"))
		GOTO_FAIL;

	ec_vec_free(vec);
	vec = NULL;

	/* invalid args */
	vec = ec_vec(0, 0, NULL, NULL);
	if (vec != NULL)
		GOTO_FAIL;

	return 0;

fail:
	ec_vec_free(vec);
	ec_vec_free(vec2);
	return -1;
}
