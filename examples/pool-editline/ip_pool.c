/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "ip_pool.h"

#include <ecoli.h>

struct ip_pool {
	struct ec_dict *addrs;
};

static struct ec_dict *pools = NULL;

int ip_pool_init(void)
{
	pools = ec_dict();
	if (pools == NULL)
		return -1;

	return 0;
}

void ip_pool_exit(void)
{
	ec_dict_free(pools);
}

static void __ip_pool_free(void *_pool)
{
	struct ip_pool *pool = _pool;

	if (pool == NULL)
		return;

	ec_dict_free(pool->addrs);
	free(pool);
}

/* create an ip pool */
struct ip_pool *ip_pool(const char *name)
{
	struct ip_pool *pool = NULL;

	if (ec_dict_has_key(pools, name)) {
		errno = EEXIST;
		goto fail;
	}

	pool = calloc(1, sizeof(*pool));
	if (pool == NULL)
		goto fail;

	pool->addrs = ec_dict();
	if (pool->addrs == NULL)
		goto fail;

	if (ec_dict_set(pools, name, pool, __ip_pool_free) < 0)
		goto fail;

	return pool;

fail:
	__ip_pool_free(pool);
	return NULL;
}

/* lookup for an ip pool */
struct ip_pool *ip_pool_lookup(const char *name)
{
	return ec_dict_get(pools, name);
}

/* list ip pool names */
struct ec_strvec *ip_pool_list(void)
{
	struct ec_dict_elt_ref *iter = NULL;
	struct ec_strvec *names = NULL;
	const char *key;

	names = ec_strvec();
	if (names == NULL)
		goto fail;

	for (iter = ec_dict_iter(pools); iter != NULL; iter = ec_dict_iter_next(iter)) {
		key = ec_dict_iter_get_key(iter);

		if (ec_strvec_add(names, key) < 0)
			goto fail;
	}

	return names;

fail:
	ec_strvec_free(names);
	return NULL;
}

/* destroy the ip pool */
void ip_pool_free(const char *name)
{
	struct ip_pool *pool;

	pool = ec_dict_get(pools, name);
	if (pool == NULL)
		return;

	ec_dict_del(pools, name);
}

/* add an IP */
int ip_pool_addr_add(struct ip_pool *pool, const char *addr)
{
	if (ec_dict_has_key(pool->addrs, addr)) {
		errno = EEXIST;
		return -1;
	}

	if (ec_dict_set(pool->addrs, addr, NULL, NULL) < 0)
		return -1;

	return 0;
}

/* del an IP */
int ip_pool_addr_del(struct ip_pool *pool, const char *addr)
{
	return ec_dict_del(pool->addrs, addr);
}

/* list pool IP addresses */
struct ec_strvec *ip_pool_addr_list(const struct ip_pool *pool)
{
	struct ec_dict_elt_ref *iter = NULL;
	struct ec_strvec *addrs = NULL;
	const char *key;

	addrs = ec_strvec();
	if (addrs == NULL)
		goto fail;

	for (iter = ec_dict_iter(pool->addrs); iter != NULL; iter = ec_dict_iter_next(iter)) {
		key = ec_dict_iter_get_key(iter);

		if (ec_strvec_add(addrs, key) < 0)
			goto fail;
	}

	return addrs;

fail:
	ec_strvec_free(addrs);
	return NULL;
}
