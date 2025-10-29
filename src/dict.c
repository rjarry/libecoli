/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ecoli/dict.h>
#include <ecoli/init.h>
#include <ecoli/log.h>

#include "htable_private.h"

EC_LOG_TYPE_REGISTER(dict);

static size_t _strlen(const char *s)
{
	if (s == NULL)
		return 0;
	return strlen(s);
}

struct ec_dict_elt {
	struct ec_htable_elt htable;
};

struct ec_dict_elt_ref {
	struct ec_htable_elt_ref htable;
};

struct ec_dict {
	struct ec_htable htable;
};

struct ec_dict *ec_dict(void)
{
	return (struct ec_dict *)ec_htable();
}

bool ec_dict_has_key(const struct ec_dict *dict, const char *key)
{
	return ec_htable_has_key(&dict->htable, key, _strlen(key) + 1);
}

void *ec_dict_get(const struct ec_dict *dict, const char *key)
{
	return ec_htable_get(&dict->htable, key, _strlen(key) + 1);
}

int ec_dict_del(struct ec_dict *dict, const char *key)
{
	return ec_htable_del(&dict->htable, key, _strlen(key) + 1);
}

int ec_dict_set(struct ec_dict *dict, const char *key, void *val, ec_dict_elt_free_t free_cb)
{
	return ec_htable_set(&dict->htable, key, _strlen(key) + 1, val, free_cb);
}

void ec_dict_free(struct ec_dict *dict)
{
	ec_htable_free(&dict->htable);
}

size_t ec_dict_len(const struct ec_dict *dict)
{
	return ec_htable_len(&dict->htable);
}

struct ec_dict_elt_ref *ec_dict_iter(const struct ec_dict *dict)
{
	return (struct ec_dict_elt_ref *)ec_htable_iter(&dict->htable);
}

struct ec_dict_elt_ref *ec_dict_iter_next(struct ec_dict_elt_ref *iter)
{
	return (struct ec_dict_elt_ref *)ec_htable_iter_next(&iter->htable);
}

const char *ec_dict_iter_get_key(const struct ec_dict_elt_ref *iter)
{
	return (const char *)ec_htable_iter_get_key(&iter->htable);
}

void *ec_dict_iter_get_val(const struct ec_dict_elt_ref *iter)
{
	return (struct ec_dict_elt_ref *)ec_htable_iter_get_val(&iter->htable);
}

void ec_dict_dump(FILE *out, const struct ec_dict *dict)
{
	struct ec_dict_elt_ref *iter;

	if (dict == NULL) {
		fprintf(out, "empty dict\n");
		return;
	}

	fprintf(out, "dict:\n");
	for (iter = ec_dict_iter(dict); iter != NULL; iter = ec_dict_iter_next(iter)) {
		fprintf(out, "  %s: %p\n", ec_dict_iter_get_key(iter), ec_dict_iter_get_val(iter));
	}
}

struct ec_dict *ec_dict_dup(const struct ec_dict *dict)
{
	return (struct ec_dict *)ec_htable_dup(&dict->htable);
}
