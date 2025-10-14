/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <ecoli_assert.h>
#include <ecoli_malloc.h>
#include <ecoli_log.h>
#include <ecoli_test.h>
#include <ecoli_vec.h>

EC_LOG_TYPE_REGISTER(vec);

struct ec_vec {
	size_t len;
	size_t size;
	size_t elt_size;
	ec_vec_elt_copy_t copy;
	ec_vec_elt_free_t free;
	void *vec;
};

static void *get_obj(const struct ec_vec *vec, size_t idx)
{
	assert(vec->elt_size != 0);
	return (char *)vec->vec + (idx * vec->elt_size);
}

struct ec_vec *
ec_vec(size_t elt_size, size_t size, ec_vec_elt_copy_t elt_copy,
	ec_vec_elt_free_t elt_free)
{
	struct ec_vec *vec;

	if (elt_size == 0) {
		errno = EINVAL;
		return NULL;
	}

	vec = ec_calloc(1, sizeof(*vec));
	if (vec == NULL)
		return NULL;

	vec->elt_size = elt_size;
	vec->copy = elt_copy;
	vec->free = elt_free;

	if (size == 0)
		return vec;

	vec->vec = ec_calloc(size, vec->elt_size);
	if (vec->vec == NULL) {
		ec_free(vec);
		return NULL;
	}

	return vec;
}

int ec_vec_add_by_ref(struct ec_vec *vec, void *ptr)
{
	void *new_vec;

	if (vec->len + 1 > vec->size) {
		new_vec = ec_realloc(vec->vec, vec->elt_size * (vec->len + 1));
		if (new_vec == NULL)
			return -1;
		vec->size = vec->len + 1;
		vec->vec = new_vec;
	}

	memcpy(get_obj(vec, vec->len), ptr, vec->elt_size);
	vec->len++;

	return 0;
}

int ec_vec_add_ptr(struct ec_vec *vec, void *elt)
{
	EC_CHECK_ARG(vec->elt_size == sizeof(elt), -1, EINVAL);

	return ec_vec_add_by_ref(vec, &elt);
}

int ec_vec_add_u8(struct ec_vec *vec, uint8_t elt)
{
	EC_CHECK_ARG(vec->elt_size == sizeof(elt), -1, EINVAL);

	return ec_vec_add_by_ref(vec, &elt);
}

int ec_vec_add_u16(struct ec_vec *vec, uint16_t elt)
{
	EC_CHECK_ARG(vec->elt_size == sizeof(elt), -1, EINVAL);

	return ec_vec_add_by_ref(vec, &elt);
}

int ec_vec_add_u32(struct ec_vec *vec, uint32_t elt)
{
	EC_CHECK_ARG(vec->elt_size == sizeof(elt), -1, EINVAL);

	return ec_vec_add_by_ref(vec, &elt);
}

int ec_vec_add_u64(struct ec_vec *vec, uint64_t elt)
{
	EC_CHECK_ARG(vec->elt_size == sizeof(elt), -1, EINVAL);

	return ec_vec_add_by_ref(vec, &elt);
}

struct ec_vec *ec_vec_ndup(const struct ec_vec *vec, size_t off,
	size_t len)
{
	struct ec_vec *copy = NULL;
	size_t i, veclen;

	veclen = ec_vec_len(vec);
	if (off + len > veclen)
		return NULL;

	copy = ec_vec(vec->elt_size, len, vec->copy, vec->free);
	if (copy == NULL)
		goto fail;

	if (len == 0)
		return copy;

	for (i = 0; i < len; i++) {
		if (vec->copy)
			vec->copy(get_obj(copy, i), get_obj(vec, i + off));
		else
			memcpy(get_obj(copy, i), get_obj(vec, i + off),
				vec->elt_size);
	}
	copy->len = len;

	return copy;

fail:
	ec_vec_free(copy);
	return NULL;
}

size_t ec_vec_len(const struct ec_vec *vec)
{
	if (vec == NULL)
		return 0;

	return vec->len;
}

struct ec_vec *ec_vec_dup(const struct ec_vec *vec)
{
	return ec_vec_ndup(vec, 0, ec_vec_len(vec));
}

void ec_vec_free(struct ec_vec *vec)
{
	size_t i;

	if (vec == NULL)
		return;

	for (i = 0; i < ec_vec_len(vec); i++) {
		if (vec->free)
			vec->free(get_obj(vec, i));
	}

	ec_free(vec->vec);
	ec_free(vec);
}

int ec_vec_get(void *ptr, const struct ec_vec *vec, size_t idx)
{
	if (vec == NULL || idx >= vec->len) {
		errno = EINVAL;
		return -1;
	}

	memcpy(ptr, get_obj(vec, idx), vec->elt_size);

	return 0;
}

static void str_free(void *elt)
{
	char **s = elt;

	ec_free(*s);
}

#define GOTO_FAIL do {					     \
		EC_LOG(EC_LOG_ERR, "%s:%d: test failed\n",   \
			__FILE__, __LINE__);		     \
		goto fail;				     \
	} while(0)

/* LCOV_EXCL_START */
static int ec_vec_testcase(void)
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

	if (ec_vec_add_ptr(vec, ec_strdup("0")) < 0)
		GOTO_FAIL;
	if (ec_vec_add_ptr(vec, ec_strdup("1")) < 0)
		GOTO_FAIL;
	if (ec_vec_add_ptr(vec, ec_strdup("2")) < 0)
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

static struct ec_test ec_vec_test = {
	.name = "vec",
	.test = ec_vec_testcase,
};

EC_TEST_REGISTER(ec_vec_test);
/* LCOV_EXCL_STOP */
