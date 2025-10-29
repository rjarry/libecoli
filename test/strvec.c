/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

EC_TEST_MAIN()
{
	struct ec_strvec *strvec = NULL;
	struct ec_strvec *strvec2 = NULL;
	const struct ec_dict *const_attrs = NULL;
	struct ec_dict *attrs = NULL;
	FILE *f = NULL;
	char *buf = NULL;
	size_t buflen = 0;
	int testres = 0;
	char quote;

	strvec = ec_strvec();
	if (strvec == NULL) {
		EC_TEST_ERR("cannot create strvec\n");
		goto fail;
	}
	if (ec_strvec_len(strvec) != 0) {
		EC_TEST_ERR("bad strvec len (0)\n");
		goto fail;
	}
	if (ec_strvec_add(strvec, "0") < 0) {
		EC_TEST_ERR("cannot add (0) in strvec\n");
		goto fail;
	}
	if (ec_strvec_len(strvec) != 1) {
		EC_TEST_ERR("bad strvec len (1)\n");
		goto fail;
	}
	if (ec_strvec_add(strvec, "1") < 0) {
		EC_TEST_ERR("cannot add (1) in strvec\n");
		goto fail;
	}
	if (ec_strvec_len(strvec) != 2) {
		EC_TEST_ERR("bad strvec len (2)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec, 0), "0")) {
		EC_TEST_ERR("invalid element in strvec (0)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec, 1), "1")) {
		EC_TEST_ERR("invalid element in strvec (1)\n");
		goto fail;
	}
	if (ec_strvec_val(strvec, 2) != NULL) {
		EC_TEST_ERR("strvec val should be NULL\n");
		goto fail;
	}

	strvec2 = ec_strvec_dup(strvec);
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec2\n");
		goto fail;
	}
	if (ec_strvec_len(strvec2) != 2) {
		EC_TEST_ERR("bad strvec2 len (2)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec2, 0), "0")) {
		EC_TEST_ERR("invalid element in strvec2 (0)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec2, 1), "1")) {
		EC_TEST_ERR("invalid element in strvec2 (1)\n");
		goto fail;
	}
	if (ec_strvec_val(strvec2, 2) != NULL) {
		EC_TEST_ERR("strvec2 val should be NULL\n");
		goto fail;
	}
	ec_strvec_free(strvec2);

	strvec2 = ec_strvec_ndup(strvec, 0, 0);
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec2\n");
		goto fail;
	}
	if (ec_strvec_len(strvec2) != 0) {
		EC_TEST_ERR("bad strvec2 len (0)\n");
		goto fail;
	}
	if (ec_strvec_val(strvec2, 0) != NULL) {
		EC_TEST_ERR("strvec2 val should be NULL\n");
		goto fail;
	}
	ec_strvec_free(strvec2);

	strvec2 = ec_strvec_ndup(strvec, 1, 1);
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec2\n");
		goto fail;
	}
	if (ec_strvec_len(strvec2) != 1) {
		EC_TEST_ERR("bad strvec2 len (1)\n");
		goto fail;
	}
	if (strcmp(ec_strvec_val(strvec2, 0), "1")) {
		EC_TEST_ERR("invalid element in strvec2 (1)\n");
		goto fail;
	}
	if (ec_strvec_val(strvec2, 1) != NULL) {
		EC_TEST_ERR("strvec2 val should be NULL\n");
		goto fail;
	}
	ec_strvec_free(strvec2);

	strvec2 = ec_strvec_ndup(strvec, 3, 1);
	if (strvec2 != NULL) {
		EC_TEST_ERR("strvec2 should be NULL\n");
		goto fail;
	}
	ec_strvec_free(strvec2);

	strvec2 = EC_STRVEC("0", "1");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_strvec_cmp(strvec, strvec2) == 0, "strvec and strvec2 should be equal\n"
	);
	ec_strvec_free(strvec2);

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_strvec_dump(f, strvec);
	fclose(f);
	f = NULL;
	testres |= EC_TEST_CHECK(strstr(buf, "strvec (len=2) [\"0\", \"1\"]"), "bad dump\n");
	free(buf);
	buf = NULL;

	ec_strvec_del_last(strvec);
	strvec2 = EC_STRVEC("0");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_strvec_cmp(strvec, strvec2) == 0, "strvec and strvec2 should be equal\n"
	);
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_strvec_dump(f, NULL);
	fclose(f);
	f = NULL;
	testres |= EC_TEST_CHECK(strstr(buf, "none"), "bad dump\n");
	free(buf);
	buf = NULL;

	ec_strvec_free(strvec);

	strvec = EC_STRVEC("e", "a", "f", "d", "b", "c");
	if (strvec == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	attrs = ec_dict();
	if (attrs == NULL) {
		EC_TEST_ERR("cannot create attrs\n");
		goto fail;
	}
	if (ec_dict_set(attrs, "key", "value", NULL) < 0) {
		EC_TEST_ERR("cannot set attr\n");
		goto fail;
	}
	if (ec_strvec_set_attrs(strvec, 1, attrs) < 0) {
		attrs = NULL;
		EC_TEST_ERR("cannot set attrs in strvec\n");
		goto fail;
	}
	attrs = NULL;

	ec_strvec_sort(strvec, NULL);

	/* attrs are now at index 0 after sorting */
	const_attrs = ec_strvec_get_attrs(strvec, 0);
	if (const_attrs == NULL) {
		EC_TEST_ERR("cannot get attrs\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(ec_dict_has_key(const_attrs, "key"), "cannot get attrs key\n");

	strvec2 = EC_STRVEC("a", "b", "c", "d", "e", "f");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_strvec_cmp(strvec, strvec2) == 0, "strvec and strvec2 should be equal\n"
	);
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	/* lexing */
	strvec = ec_strvec_sh_lex_str("  a    b\tc d   # comment", EC_STRVEC_STRICT, NULL);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b", "c", "d");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_strvec_cmp(strvec, strvec2) == 0, "strvec and strvec2 should be equal\n"
	);
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	strvec = ec_strvec_sh_lex_str("  a  b   c  d  ", EC_STRVEC_TRAILSP, NULL);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b", "c", "d", "");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_strvec_cmp(strvec, strvec2) == 0, "strvec and strvec2 should be equal\n"
	);
	for (unsigned i = 0; i < ec_strvec_len(strvec); i++) {
		const_attrs = ec_strvec_get_attrs(strvec, i);
		testres |= EC_TEST_CHECK(const_attrs != NULL, "attrs should not be NULL\n");
		if (const_attrs == NULL)
			continue;
		errno = 0;
		unsigned s = (uintptr_t)ec_dict_get(const_attrs, EC_STRVEC_ATTR_START);
		unsigned e = (uintptr_t)ec_dict_get(const_attrs, EC_STRVEC_ATTR_END);
		testres |= EC_TEST_CHECK(errno == 0, "");
		switch (i) {
		case 0:
			testres |= EC_TEST_CHECK(s == 2 && e == 3, "");
			break;
		case 1:
			testres |= EC_TEST_CHECK(s == 5 && e == 6, "");
			break;
		case 2:
			testres |= EC_TEST_CHECK(s == 9 && e == 10, "");
			break;
		case 3:
			testres |= EC_TEST_CHECK(s == 12 && e == 13, "");
			break;
		case 4:
			testres |= EC_TEST_CHECK(s == 15 && e == 16, "");
			break;
		}
	}
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	strvec = ec_strvec_sh_lex_str("a  b  'c  d' ", EC_STRVEC_STRICT, NULL);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b", "c  d");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_strvec_cmp(strvec, strvec2) == 0, "strvec and strvec2 should be equal\n"
	);
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	strvec = ec_strvec_sh_lex_str("a  b\\ e  \"c \\\" d\" ", EC_STRVEC_STRICT, NULL);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b e", "c \" d");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_strvec_cmp(strvec, strvec2) == 0, "strvec and strvec2 should be equal\n"
	);
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	strvec = ec_strvec_sh_lex_str("a  b  'c  d ", EC_STRVEC_STRICT, NULL);
	if (strvec != NULL) {
		testres |= EC_TEST_CHECK(strvec == NULL, "shlex should have failed\n");
		ec_strvec_free(strvec);
		strvec = NULL;
	} else {
		testres |= EC_TEST_CHECK(
			errno == EBADMSG, "ec_strvec_shlex_str should report EBADMSG\n"
		);
	}

	quote = '\0';
	strvec = ec_strvec_sh_lex_str("a  'b'  'c  d ", EC_STRVEC_TRAILSP, &quote);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "b", "c  d ");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_strvec_cmp(strvec, strvec2) == 0, "strvec and strvec2 should be equal\n"
	);
	testres |= EC_TEST_CHECK(quote == '\'', "missing quote should be '\n");
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	quote = '\0';
	strvec = ec_strvec_sh_lex_str("a  'b'\"x\"  'c  d' ", EC_STRVEC_TRAILSP, &quote);
	if (strvec == NULL) {
		EC_TEST_ERR("cannot lex strvec from string\n");
		goto fail;
	}
	strvec2 = EC_STRVEC("a", "bx", "c  d", "");
	if (strvec2 == NULL) {
		EC_TEST_ERR("cannot create strvec from array\n");
		goto fail;
	}
	testres |= EC_TEST_CHECK(
		ec_strvec_cmp(strvec, strvec2) == 0, "strvec and strvec2 should be equal\n"
	);
	testres |= EC_TEST_CHECK(quote == '\0', "there should be no missing quote\n");
	ec_strvec_free(strvec);
	strvec = NULL;
	ec_strvec_free(strvec2);
	strvec2 = NULL;

	return testres;

fail:
	if (f != NULL)
		fclose(f);
	ec_dict_free(attrs);
	ec_strvec_free(strvec);
	ec_strvec_free(strvec2);
	free(buf);

	return -1;
}
