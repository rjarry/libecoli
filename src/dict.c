/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <ecoli_init.h>
#include <ecoli_malloc.h>
#include <ecoli_log.h>
#include <ecoli_test.h>
#include <ecoli_dict.h>

#include "htable.h"

EC_LOG_TYPE_REGISTER(dict);

static size_t
_strlen(const char *s)
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
	return ec_htable_has_key(
		&dict->htable, key, _strlen(key) + 1);
}

void *ec_dict_get(const struct ec_dict *dict, const char *key)
{
	return ec_htable_get(&dict->htable, key, _strlen(key) + 1);
}

int ec_dict_del(struct ec_dict *dict, const char *key)
{
	return ec_htable_del(&dict->htable, key, _strlen(key) + 1);
}

int ec_dict_set(struct ec_dict *dict, const char *key, void *val,
	ec_dict_elt_free_t free_cb)
{
	return ec_htable_set(&dict->htable, key, _strlen(key) + 1,
		val, free_cb);
}

void ec_dict_free(struct ec_dict *dict)
{
	ec_htable_free(&dict->htable);
}

size_t ec_dict_len(const struct ec_dict *dict)
{
	return ec_htable_len(&dict->htable);
}

struct ec_dict_elt_ref *
ec_dict_iter(const struct ec_dict *dict)
{
	return (struct ec_dict_elt_ref *)ec_htable_iter(&dict->htable);
}

struct ec_dict_elt_ref *
ec_dict_iter_next(struct ec_dict_elt_ref *iter)
{
	return (struct ec_dict_elt_ref *)ec_htable_iter_next(&iter->htable);
}

const char *
ec_dict_iter_get_key(const struct ec_dict_elt_ref *iter)
{
	return (const char *)ec_htable_iter_get_key(&iter->htable);
}

void *
ec_dict_iter_get_val(const struct ec_dict_elt_ref *iter)
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
	for (iter = ec_dict_iter(dict);
	     iter != NULL;
	     iter = ec_dict_iter_next(iter)) {
		fprintf(out, "  %s: %p\n",
			ec_dict_iter_get_key(iter),
			ec_dict_iter_get_val(iter));
	}
}

struct ec_dict *ec_dict_dup(const struct ec_dict *dict)
{
	return (struct ec_dict *)ec_htable_dup(&dict->htable);
}

/* LCOV_EXCL_START */
static int ec_dict_testcase(void)
{
	struct ec_dict *dict, *dup;
	struct ec_dict_elt_ref *iter;
	char *val;
	size_t i, count;
	int ret, testres = 0;
	FILE *f = NULL;
	char *buf = NULL;
	size_t buflen = 0;

	dict = ec_dict();
	if (dict == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create dict\n");
		return -1;
	}

	count = 0;
	for (iter = ec_dict_iter(dict);
	     iter != NULL;
	     iter = ec_dict_iter_next(iter)) {
		count++;
	}
	testres |= EC_TEST_CHECK(count == 0, "invalid count in iterator");

	testres |= EC_TEST_CHECK(ec_dict_len(dict) == 0, "bad dict len");
	ret = ec_dict_set(dict, "key1", "val1", NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot set key");
	ret = ec_dict_set(dict, "key2", ec_strdup("val2"), ec_free_func);
	testres |= EC_TEST_CHECK(ret == 0, "cannot set key");
	testres |= EC_TEST_CHECK(ec_dict_len(dict) == 2, "bad dict len");

	val = ec_dict_get(dict, "key1");
	testres |= EC_TEST_CHECK(val != NULL && !strcmp(val, "val1"),
				"invalid dict value");
	val = ec_dict_get(dict, "key2");
	testres |= EC_TEST_CHECK(val != NULL && !strcmp(val, "val2"),
				"invalid dict value");
	val = ec_dict_get(dict, "key3");
	testres |= EC_TEST_CHECK(val == NULL, "key3 should be NULL");

	ret = ec_dict_set(dict, "key1", "another_val1", NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot set key");
	ret = ec_dict_set(dict, "key2", ec_strdup("another_val2"),
			ec_free_func);
	testres |= EC_TEST_CHECK(ret == 0, "cannot set key");
	testres |= EC_TEST_CHECK(ec_dict_len(dict) == 2,
				"bad dict len");

	val = ec_dict_get(dict, "key1");
	testres |= EC_TEST_CHECK(val != NULL && !strcmp(val, "another_val1"),
		"invalid dict value");
	val = ec_dict_get(dict, "key2");
	testres |= EC_TEST_CHECK(val != NULL && !strcmp(val, "another_val2"),
		"invalid dict value");
	testres |= EC_TEST_CHECK(ec_dict_has_key(dict, "key1"),
		"key1 should be in dict");

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_dict_dump(f, NULL);
	fclose(f);
	f = NULL;
	free(buf);
	buf = NULL;

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_dict_dump(f, dict);
	fclose(f);
	f = NULL;
	free(buf);
	buf = NULL;

	ret = ec_dict_del(dict, "key1");
	testres |= EC_TEST_CHECK(ret == 0, "cannot del key1");
	testres |= EC_TEST_CHECK(ec_dict_len(dict) == 1,
		"invalid dict len");
	ret = ec_dict_del(dict, "key2");
	testres |= EC_TEST_CHECK(ret == 0, "cannot del key2");
	testres |= EC_TEST_CHECK(ec_dict_len(dict) == 0,
		"invalid dict len");

	for (i = 0; i < 100; i++) {
		char key[8];
		snprintf(key, sizeof(key), "k%zd", i);
		ret = ec_dict_set(dict, key, "val", NULL);
		testres |= EC_TEST_CHECK(ret == 0, "cannot set key");
	}
	dup = ec_dict_dup(dict);
	testres |= EC_TEST_CHECK(dup != NULL, "cannot duplicate dict");
	if (dup != NULL) {
		for (i = 0; i < 100; i++) {
			char key[8];
			snprintf(key, sizeof(key), "k%zd", i);
			val = ec_dict_get(dup, key);
			testres |= EC_TEST_CHECK(
				val != NULL && !strcmp(val, "val"),
				"invalid dict value");
		}
		ec_dict_free(dup);
		dup = NULL;
	}

	count = 0;
	for (iter = ec_dict_iter(dict);
	     iter != NULL;
	     iter = ec_dict_iter_next(iter)) {
		count++;
	}
	testres |= EC_TEST_CHECK(count == 100, "invalid count in iterator");

	/* einval */
	ret = ec_dict_set(dict, NULL, "val1", NULL);
	testres |= EC_TEST_CHECK(ret == -1, "should not be able to set key");
	val = ec_dict_get(dict, NULL);
	testres |= EC_TEST_CHECK(val == NULL, "get(NULL) should no success");

	ec_dict_free(dict);

	return testres;

fail:
	ec_dict_free(dict);
	if (f)
		fclose(f);
	free(buf);
	return -1;
}

static struct ec_test ec_dict_test = {
	.name = "dict",
	.test = ec_dict_testcase,
};

EC_TEST_REGISTER(ec_dict_test);
/* LCOV_EXCL_STOP */
