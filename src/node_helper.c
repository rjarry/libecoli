/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

#include <sys/types.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#include <ecoli_utils.h>
#include <ecoli_config.h>
#include <ecoli_node.h>
#include <ecoli_node_helper.h>

struct ec_node **
ec_node_config_node_list_to_table(const struct ec_config *config,
				size_t *len)
{
	struct ec_node **table = NULL;
	struct ec_config *child;
	ssize_t n, i;

	*len = 0;

	if (config == NULL) {
		errno = EINVAL;
		return NULL;
	}

	if (ec_config_get_type(config) != EC_CONFIG_TYPE_LIST) {
		errno = EINVAL;
		return NULL;
	}

	n = ec_config_count(config);
	if (n < 0)
		return NULL;

	table = calloc(n, sizeof(*table));
	if (table == NULL)
		goto fail;

	n = 0;
	TAILQ_FOREACH(child, &config->list, next) {
		if (ec_config_get_type(child) != EC_CONFIG_TYPE_NODE) {
			errno = EINVAL;
			goto fail;
		}
		table[n] = ec_node_clone(child->node);
		n++;
	}

	*len = n;

	return table;

fail:
	if (table != NULL) {
		for (i = 0; i < n; i++)
			ec_node_free(table[i]);
	}
	free(table);

	return NULL;
}

struct ec_config *
ec_node_config_node_list_from_vargs(va_list ap)
{
	struct ec_config *list = NULL;
	struct ec_node *node = va_arg(ap, struct ec_node *);

	list = ec_config_list();
	if (list == NULL)
		goto fail;

	for (; node != EC_VA_END; node = va_arg(ap, struct ec_node *)) {
		if (node == NULL)
			goto fail;

		if (ec_config_list_add(list, ec_config_node(node)) < 0)
			goto fail;
	}

	return list;

fail:
	for (; node != EC_VA_END; node = va_arg(ap, struct ec_node *))
		ec_node_free(node);
	ec_config_free(list);

	return NULL;
}
