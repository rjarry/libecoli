/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <ecoli/assert.h>
#include <ecoli/log.h>
#include <ecoli/vec.h>

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
ec_vec(size_t elt_size, size_t size, ec_vec_elt_copy_t elt_copy, ec_vec_elt_free_t elt_free)
{
	struct ec_vec *vec;

	if (elt_size == 0) {
		errno = EINVAL;
		return NULL;
	}

	vec = calloc(1, sizeof(*vec));
	if (vec == NULL)
		return NULL;

	vec->elt_size = elt_size;
	vec->copy = elt_copy;
	vec->free = elt_free;

	if (size == 0)
		return vec;

	vec->vec = calloc(size, vec->elt_size);
	if (vec->vec == NULL) {
		free(vec);
		return NULL;
	}

	return vec;
}

int ec_vec_add_by_ref(struct ec_vec *vec, void *ptr)
{
	void *new_vec;

	if (vec->len + 1 > vec->size) {
		new_vec = realloc(vec->vec, vec->elt_size * (vec->len + 1));
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

struct ec_vec *ec_vec_ndup(const struct ec_vec *vec, size_t off, size_t len)
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
			memcpy(get_obj(copy, i), get_obj(vec, i + off), vec->elt_size);
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

	free(vec->vec);
	free(vec);
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
