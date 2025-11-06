/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

/* A very simple IP pool. */

#include <stdint.h>

struct ec_strvec;
struct ip_pool;
struct ip_address;

/* create htable of ip pools */
int ip_pool_init(void);

/* free ip pools */
void ip_pool_exit(void);

/* create an ip pool */
struct ip_pool *ip_pool(const char *name);

/* lookup for an ip pool, by name */
struct ip_pool *ip_pool_lookup(const char *name);

/* list ip pool names */
struct ec_strvec *ip_pool_list(void);

/* destroy the ip pool */
void ip_pool_free(const char *name);

/* add an IP */
int ip_pool_addr_add(struct ip_pool *pool, const char *addr);

/* del an IP */
int ip_pool_addr_del(struct ip_pool *pool, const char *addr);

/* list pool IP addresses */
struct ec_strvec *ip_pool_addr_list(const struct ip_pool *pool);
