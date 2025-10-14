/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <ecoli_init.h>
#include <ecoli_log.h>
#include <ecoli_murmurhash.h>
#include <ecoli_htable.h>

#include "ecoli_htable_private.h"

#define FACTOR 3

EC_LOG_TYPE_REGISTER(htable);

static uint32_t ec_htable_seed;

struct ec_htable *ec_htable(void)
{
	struct ec_htable *htable;

	htable = calloc(1, sizeof(*htable));
	if (htable == NULL)
		return NULL;
	TAILQ_INIT(&htable->list);

	return htable;
}

static struct ec_htable_elt_ref *
ec_htable_lookup(const struct ec_htable *htable, const void *key,
		size_t key_len)
{
	struct ec_htable_elt_ref *ref;
	uint32_t h, mask = htable->table_size - 1;

	if (htable == NULL || key == NULL) {
		errno = EINVAL;
		return NULL;
	}
	if (htable->table_size == 0) {
		errno = ENOENT;
		return NULL;
	}

	h = ec_murmurhash3(key, key_len, ec_htable_seed);
	TAILQ_FOREACH(ref, &htable->table[h & mask], hnext) {
		if (ref->elt->key_len != key_len)
			continue;
		if (memcmp(ref->elt->key, key, key_len) == 0)
			return ref;
	}

	errno = ENOENT;
	return NULL;
}

static void ec_htable_elt_ref_free(struct ec_htable_elt_ref *ref)
{
	struct ec_htable_elt *elt;

	if (ref == NULL)
		return;

	elt = ref->elt;
	if (elt != NULL && --elt->refcount == 0) {
		free(elt->key);
		if (elt->free != NULL)
			elt->free(elt->val);
		free(elt);
	}
	free(ref);
}

bool ec_htable_has_key(const struct ec_htable *htable, const void *key,
		size_t key_len)
{
	return !!ec_htable_lookup(htable, key, key_len);
}

void *ec_htable_get(const struct ec_htable *htable, const void *key,
		size_t key_len)
{
	struct ec_htable_elt_ref *ref;

	ref = ec_htable_lookup(htable, key, key_len);
	if (ref == NULL)
		return NULL;

	return ref->elt->val;
}

int ec_htable_del(struct ec_htable *htable, const void *key, size_t key_len)
{
	struct ec_htable_elt_ref *ref;
	size_t idx;

	ref = ec_htable_lookup(htable, key, key_len);
	if (ref == NULL)
		return -1;

	/* we could resize table here */

	TAILQ_REMOVE(&htable->list, ref, next);
	idx = ref->elt->hash & (htable->table_size - 1);
	TAILQ_REMOVE(&htable->table[idx], ref, hnext);
	ec_htable_elt_ref_free(ref);
	htable->len--;

	return 0;
}

static int ec_htable_table_resize(struct ec_htable *htable, size_t new_size)
{
	struct ec_htable_elt_ref_list *new_table;
	struct ec_htable_elt_ref *ref;
	size_t i;

	if (new_size == 0 || (new_size & (new_size - 1))) {
		errno = EINVAL;
		return -1;
	}

	new_table = calloc(new_size, sizeof(*htable->table));
	if (new_table == NULL)
		return -1;
	for (i = 0; i < new_size; i++)
		TAILQ_INIT(&new_table[i]);

	TAILQ_FOREACH(ref, &htable->list, next) {
		i = ref->elt->hash & (htable->table_size - 1);
		TAILQ_REMOVE(&htable->table[i], ref, hnext);
		i = ref->elt->hash & (new_size - 1);
		TAILQ_INSERT_TAIL(&new_table[i], ref, hnext);
	}

	free(htable->table);
	htable->table = new_table;
	htable->table_size = new_size;

	return 0;
}

static int
__ec_htable_set(struct ec_htable *htable, struct ec_htable_elt_ref *ref)
{
	size_t new_size;
	uint32_t mask;
	int ret;

	/* remove previous entry if any */
	ec_htable_del(htable, ref->elt->key, ref->elt->key_len);

	if (htable->len >= htable->table_size) {
		if (htable->table_size != 0)
			new_size =  htable->table_size << FACTOR;
		else
			new_size =  1 << FACTOR;
		ret = ec_htable_table_resize(htable, new_size);
		if (ret < 0)
			return ret;
	}

	mask = htable->table_size - 1;
	TAILQ_INSERT_TAIL(&htable->table[ref->elt->hash & mask], ref, hnext);
	TAILQ_INSERT_TAIL(&htable->list, ref, next);
	htable->len++;

	return 0;
}

int ec_htable_set(struct ec_htable *htable, const void *key, size_t key_len,
		void *val, ec_htable_elt_free_t free_cb)
{
	struct ec_htable_elt *elt = NULL;
	struct ec_htable_elt_ref *ref = NULL;
	uint32_t h;

	if (htable == NULL || key == NULL || key_len == 0) {
		errno = EINVAL;
		return -1;
	}

	ref = calloc(1, sizeof(*ref));
	if (ref == NULL)
		goto fail;

	elt = calloc(1, sizeof(*elt));
	if (elt == NULL)
		goto fail;

	ref->elt = elt;
	elt->refcount = 1;
	elt->val = val;
	val = NULL;
	elt->free = free_cb;
	elt->key_len = key_len;
	elt->key = malloc(key_len);
	if (elt->key == NULL)
		goto fail;
	memcpy(elt->key, key, key_len);
	h = ec_murmurhash3(key, key_len, ec_htable_seed);
	elt->hash = h;

	if (__ec_htable_set(htable, ref) < 0)
		goto fail;

	return 0;

fail:
	if (free_cb != NULL && val != NULL)
		free_cb(val);
	ec_htable_elt_ref_free(ref);
	return -1;
}

void ec_htable_free(struct ec_htable *htable)
{
	struct ec_htable_elt_ref *ref;
	size_t idx;

	if (htable == NULL)
		return;

	while (!TAILQ_EMPTY(&htable->list)) {
		ref = TAILQ_FIRST(&htable->list);
		TAILQ_REMOVE(&htable->list, ref, next);
		idx = ref->elt->hash & (htable->table_size - 1);
		TAILQ_REMOVE(&htable->table[idx], ref, hnext);
		ec_htable_elt_ref_free(ref);
	}
	free(htable->table);
	free(htable);
}

size_t ec_htable_len(const struct ec_htable *htable)
{
	return htable->len;
}

struct ec_htable_elt_ref *
ec_htable_iter(const struct ec_htable *htable)
{
	if (htable == NULL)
		return NULL;

	return TAILQ_FIRST(&htable->list);
}

struct ec_htable_elt_ref *
ec_htable_iter_next(struct ec_htable_elt_ref *iter)
{
	if (iter == NULL)
		return NULL;

	return TAILQ_NEXT(iter, next);
}

const void *
ec_htable_iter_get_key(const struct ec_htable_elt_ref *iter)
{
	if (iter == NULL)
		return NULL;

	return iter->elt->key;
}

size_t
ec_htable_iter_get_key_len(const struct ec_htable_elt_ref *iter)
{
	if (iter == NULL)
		return 0;

	return iter->elt->key_len;
}

void *
ec_htable_iter_get_val(const struct ec_htable_elt_ref *iter)
{
	if (iter == NULL)
		return NULL;

	return iter->elt->val;
}

void ec_htable_dump(FILE *out, const struct ec_htable *htable)
{
	if (htable == NULL) {
		fprintf(out, "empty htable\n");
		return;
	}

	fprintf(out, "htable: %zd elements\n", htable->len);
}

struct ec_htable *ec_htable_dup(const struct ec_htable *htable)
{
	struct ec_htable *dup = NULL;
	struct ec_htable_elt_ref *ref, *dup_ref = NULL;

	dup = ec_htable();
	if (dup == NULL)
		return NULL;

	TAILQ_FOREACH(ref, &htable->list, next) {
		dup_ref = calloc(1, sizeof(*ref));
		if (dup_ref == NULL)
			goto fail;
		dup_ref->elt = ref->elt;
		ref->elt->refcount++;

		if (__ec_htable_set(dup, dup_ref) < 0)
			goto fail;
	}

	return dup;

fail:
	ec_htable_elt_ref_free(dup_ref);
	ec_htable_free(dup);
	return NULL;
}

static int ec_htable_init_func(void)
{
	int fd;
	ssize_t ret;

	return 0;//XXX for test reproduceability

	fd = open("/dev/urandom", 0);
	if (fd == -1) {
		fprintf(stderr, "failed to open /dev/urandom\n");
		return -1;
	}
	ret = read(fd, &ec_htable_seed, sizeof(ec_htable_seed));
	if (ret != sizeof(ec_htable_seed)) {
		fprintf(stderr, "failed to read /dev/urandom\n");
		return -1;
	}
	close(fd);
	return 0;
}

static struct ec_init ec_htable_init = {
	.init = ec_htable_init_func,
	.priority = 50,
};

EC_INIT_REGISTER(ec_htable_init);
