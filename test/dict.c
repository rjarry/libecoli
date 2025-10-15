/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>

#include "test.h"

EC_TEST_MAIN()
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
