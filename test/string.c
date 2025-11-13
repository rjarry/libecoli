/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

const char *text = " \n\n dedddesde desd desd        desdesd des desd des des des des\n"
		   "dedddesde desd desd desdesd des desd des des des des\n"
		   "dedddesde desd desd        desdesd des desd des des des des\n"
		   "dedddddddddddddddddddddddddddcccddddddddcdddddddddededes\n"
		   "dedddesde desd desd        desdesd des desd des des des des\n"
		   "dedddesde desd desd        desdesd des desd des des des des\n "
		   "\n"
		   "dedddesde desd desd        desdesd des desd des des des des  \n"
		   "dedddddddddddddddddddddddddddcccddddddddcdddddddddededes\n"
		   "dedddesde desd desd        desdesd des desd des des des des\n\n  \n \n"
		   "dedddesde desd desd        desdesd des desd des des des des\n"
		   "dedddesde desd desd        desdesd des desd des des des des\n"
		   "\n";

const char *wrapped_text = "dedddesde desd desd desdesd des desd des\n"
			   "                              des des des dedddesde desd desd desdesd\n"
			   "                              des desd des des des des dedddesde desd\n"
			   "                              desd desdesd des desd des des des des\n"
			   "                              "
			   "dedddddddddddddddddddddddddddcccddddddddcdddddddddededes\n"
			   "                              dedddesde desd desd desdesd des desd "
			   "des\n"
			   "                              des des des dedddesde desd desd desdesd\n"
			   "                              des desd des des des des dedddesde desd\n"
			   "\n"
			   "                              desd desdesd des desd des des des des\n"
			   "                              "
			   "dedddddddddddddddddddddddddddcccddddddddcdddddddddededes\n"
			   "                              dedddesde desd desd desdesd des desd "
			   "des\n"
			   "                              des des des dedddesde desd desd desdesd\n"
			   "\n"
			   "                              des desd des des des des dedddesde desd\n"
			   "                              desd desdesd des desd des des des des";

EC_TEST_MAIN()
{
	char *out = NULL;
	int testres = 0;

	out = ec_str_wrap(text, 72, 30);
	if (out == NULL) {
		EC_TEST_ERR("cannot to wrap text\n");
		goto fail;
	}
	if (strcmp(out, wrapped_text)) {
		EC_TEST_ERR("text wrapped incorrectly\n");
		goto fail;
	}
	free(out);
	out = NULL;

	return testres;

fail:
	free(out);

	return -1;
}
