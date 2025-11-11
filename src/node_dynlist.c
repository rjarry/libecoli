/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli/complete.h>
#include <ecoli/dict.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/node_dynlist.h>
#include <ecoli/node_many.h>
#include <ecoli/node_str.h>
#include <ecoli/parse.h>
#include <ecoli/string.h>
#include <ecoli/strvec.h>

EC_LOG_TYPE_REGISTER(node_dynlist);

struct ec_node_dynlist {
	ec_node_dynlist_get_t get;
	void *opaque;
	enum ec_node_dynlist_flags flags;
	char *re_str;
	regex_t re;
};

static int ec_node_dynlist_parse(
	const struct ec_node *node,
	struct ec_pnode *parse,
	const struct ec_strvec *strvec
)
{
	struct ec_node_dynlist *priv = ec_node_priv(node);
	struct ec_strvec *names = NULL;
	const char *name;
	const char *str;
	regmatch_t pos;
	size_t len;
	size_t i;
	int ret;

	if (priv->get == NULL || priv->re_str == NULL) {
		errno = ENOENT;
		return -1;
	}

	if (ec_strvec_len(strvec) == 0)
		return EC_PARSE_NOMATCH;

	str = ec_strvec_val(strvec, 0);

	names = priv->get(parse, priv->opaque);
	if (names == NULL)
		goto fail;

	len = ec_strvec_len(names);
	for (i = 0; i < len; i++) {
		name = ec_strvec_val(names, i);
		if (strcmp(name, str))
			continue;
		if (priv->flags & DYNLIST_EXCLUDE_LIST) {
			ret = EC_PARSE_NOMATCH;
			goto end;
		}
		if (priv->flags & DYNLIST_MATCH_LIST) {
			ret = 1;
			goto end;
			;
		}
	}

	if (priv->re_str != NULL && priv->flags & DYNLIST_MATCH_REGEXP) {
		if (regexec(&priv->re, str, 1, &pos, 0) == 0 && pos.rm_so == 0
		    && pos.rm_eo == (int)strlen(str)) {
			ret = 1;
			goto end;
		}
	}

	ret = EC_PARSE_NOMATCH;

end:
	ec_strvec_free(names);
	return ret;

fail:
	ret = -1;
	goto end;
}

static int ec_node_dynlist_complete(
	const struct ec_node *node,
	struct ec_comp *comp,
	const struct ec_strvec *strvec
)
{
	struct ec_node_dynlist *priv = ec_node_priv(node);
	const struct ec_comp_item *item = NULL;
	struct ec_strvec *names = NULL;
	const char *name;
	const char *str;
	size_t len;
	size_t i;

	if (priv->get == NULL || priv->re_str == NULL) {
		errno = ENOENT;
		return -1;
	}

	if (ec_strvec_len(strvec) != 1)
		return 0;

	str = ec_strvec_val(strvec, 0);

	item = ec_comp_add_item(comp, node, EC_COMP_UNKNOWN, NULL, NULL);
	if (item == NULL)
		goto fail;

	if (priv->flags & DYNLIST_MATCH_LIST) {
		names = priv->get(ec_comp_get_cur_pstate(comp), priv->opaque);
		if (names == NULL)
			goto fail;

		len = ec_strvec_len(names);
		for (i = 0; i < len; i++) {
			name = ec_strvec_val(names, i);

			if (!ec_str_startswith(name, str))
				continue;

			item = ec_comp_add_item(comp, node, EC_COMP_FULL, str, name);
			if (item == NULL)
				goto fail;
		}
	}

	ec_strvec_free(names);
	return 0;

fail:
	ec_strvec_free(names);
	return -1;
}

static void ec_node_dynlist_free_priv(struct ec_node *node)
{
	struct ec_node_dynlist *priv = ec_node_priv(node);

	if (priv->re_str != NULL) {
		free(priv->re_str);
		regfree(&priv->re);
	}
}

static struct ec_node_type ec_node_dynlist_type = {
	.name = "dynlist",
	.parse = ec_node_dynlist_parse,
	.complete = ec_node_dynlist_complete,
	.size = sizeof(struct ec_node_dynlist),
	.free_priv = ec_node_dynlist_free_priv,
};

struct ec_node *ec_node_dynlist(
	const char *id,
	ec_node_dynlist_get_t get,
	void *opaque,
	const char *re_str,
	enum ec_node_dynlist_flags flags
)
{
	struct ec_node *node = NULL;
	struct ec_node_dynlist *priv;
	regex_t re;
	int ret;

	if (get == NULL) {
		errno = EINVAL;
		goto fail;
	}

	node = ec_node_from_type(&ec_node_dynlist_type, id);
	if (node == NULL)
		goto fail;

	priv = ec_node_priv(node);
	priv->re_str = strdup(re_str);
	if (priv->re_str == NULL)
		goto fail;

	ret = regcomp(&re, re_str, REG_EXTENDED);
	if (ret != 0) {
		if (ret == REG_ESPACE)
			errno = ENOMEM;
		else
			errno = EINVAL;
		goto fail;
	}
	priv->re = re;
	priv->get = get;
	priv->opaque = opaque;
	priv->flags = flags;

	return node;

fail:
	ec_node_free(node);
	return NULL;
}

EC_NODE_TYPE_REGISTER(ec_node_dynlist_type);
