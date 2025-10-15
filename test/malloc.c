/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include "test.h"

EC_TEST_MAIN()
{
	int ret, testres = 0;
	char *ptr, *ptr2;

	ret = ec_malloc_register(NULL, NULL, NULL);
	testres |= EC_TEST_CHECK(ret == -1,
		"should not be able to register NULL malloc handlers");
	ret = ec_malloc_register(__ec_malloc, __ec_free, __ec_realloc);
	testres |= EC_TEST_CHECK(ret == -1,
		"should not be able to register after init");

	/* registration is tested in the test main.c */

	ptr = ec_malloc(10);
	if (ptr == NULL)
		return -1;
	memset(ptr, 0, 10);
	ptr2 = ec_realloc(ptr, 20);
	testres |= EC_TEST_CHECK(ptr2 != NULL, "cannot realloc ptr\n");
	if (ptr2 == NULL)
		ec_free(ptr);
	else
		ec_free(ptr2);
	ptr = NULL;
	ptr2 = NULL;

	ptr = ec_malloc_func(10);
	if (ptr == NULL)
		return -1;
	memset(ptr, 0, 10);
	ec_free_func(ptr);
	ptr = NULL;

	return testres;
}
