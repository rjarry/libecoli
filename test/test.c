/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "test.h"

EC_LOG_TYPE_REGISTER(test);

int ec_test_check_parse(struct ec_node *tk, int expected, ...)
{
	struct ec_pnode *p;
	struct ec_strvec *vec = NULL;
	const char *s;
	int ret = -1, match;
	va_list ap;

	va_start(ap, expected);

	/* build a string vector */
	vec = ec_strvec();
	if (vec == NULL)
		goto out;

	for (s = va_arg(ap, const char *);
	     s != EC_VA_END;
	     s = va_arg(ap, const char *)) {
		if (s == NULL)
			goto out;

		if (ec_strvec_add(vec, s) < 0)
			goto out;
	}

	p = ec_parse_strvec(tk, vec);
	if (p == NULL) {
		EC_LOG(EC_LOG_ERR, "parse is NULL\n");
	}
	if (ec_pnode_matches(p))
		match = ec_pnode_len(p);
	else
		match = -1;
	if (expected == match) {
		ret = 0;
	} else {
		EC_LOG(EC_LOG_ERR,
			"parse len (%d) does not match expected (%d)\n",
			match, expected);
	}

	ec_pnode_free(p);

out:
	ec_strvec_free(vec);
	va_end(ap);
	return ret;
}

int ec_test_check_complete(struct ec_node *tk, enum ec_comp_type type, ...)
{
	struct ec_comp *c = NULL;
	struct ec_strvec *vec = NULL;
	const char *s;
	int ret = 0;
	size_t count = 0;
	va_list ap;

	va_start(ap, type);

	/* build a string vector */
	vec = ec_strvec();
	if (vec == NULL)
		goto out;

	for (s = va_arg(ap, const char *);
	     s != EC_VA_END;
	     s = va_arg(ap, const char *)) {
		if (s == NULL)
			goto out;

		if (ec_strvec_add(vec, s) < 0)
			goto out;
	}

	c = ec_complete_strvec(tk, vec);
	if (c == NULL) {
		ret = -1;
		goto out;
	}

	/* for each expected completion, check it is there */
	for (s = va_arg(ap, const char *);
	     s != EC_VA_END;
	     s = va_arg(ap, const char *)) {
		struct ec_comp_item *item;

		if (s == NULL) {
			ret = -1;
			goto out;
		}

		count++;

		/* only check matching completions */
		EC_COMP_FOREACH(item, c, type) {
			const char *str = ec_comp_item_get_str(item);
			if (str != NULL && strcmp(str, s) == 0)
				break;
		}

		if (item == NULL) {
			EC_LOG(EC_LOG_ERR,
				"completion <%s> not in list\n", s);
			ret = -1;
		}
	}

	/* check if we have more completions (or less) than expected */
	if (count != ec_comp_count(c, type)) {
		EC_LOG(EC_LOG_ERR,
			"nb_completion (%zu) does not match (%zu)\n",
			count, ec_comp_count(c, type));
		ec_comp_dump(stdout, c);
		ret = -1;
	}

out:
	ec_strvec_free(vec);
	ec_comp_free(c);
	va_end(ap);
	return ret;
}
