/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdlib.h>
#include <string.h>

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_htable *htable;
	struct ec_htable_elt_ref *iter;
	size_t count;
	int ret, testres = 0;
	FILE *f = NULL;
	char *buf = NULL;
	size_t buflen = 0;

	/* Minimal test, since ec_dict also uses this code and is better
	 * tested. */

	htable = ec_htable();
	if (htable == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create htable\n");
		return -1;
	}

	count = 0;
	for (iter = ec_htable_iter(htable);
	     iter != NULL;
	     iter = ec_htable_iter_next(iter)) {
		count++;
	}
	testres |= EC_TEST_CHECK(count == 0, "invalid count in iterator");

	testres |= EC_TEST_CHECK(ec_htable_len(htable) == 0, "bad htable len");
	ret = ec_htable_set(htable, "key1", 4, "val1", NULL);
	testres |= EC_TEST_CHECK(ret == 0, "cannot set key");
	ret = ec_htable_set(htable, "key2", 4, strdup("val2"), free);
	testres |= EC_TEST_CHECK(ret == 0, "cannot set key");
	testres |= EC_TEST_CHECK(ec_htable_len(htable) == 2, "bad htable len");

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_htable_dump(f, NULL);
	fclose(f);
	f = NULL;
	free(buf);
	buf = NULL;

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_htable_dump(f, htable);
	fclose(f);
	f = NULL;
	free(buf);
	buf = NULL;

	ec_htable_free(htable);

	return testres;

fail:
	ec_htable_free(htable);
	if (f)
		fclose(f);
	free(buf);
	return -1;
}
