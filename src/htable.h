/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2019, Olivier MATZ <zer0@droids-corp.org>
 */

#pragma once

#include <stdint.h>
#include <sys/queue.h>

#include <ecoli_htable.h>

struct ec_htable_elt {
	void *key;
	size_t key_len;
	void *val;
	uint32_t hash;
	ec_htable_elt_free_t free;
	unsigned int refcount;
};

struct ec_htable_elt_ref {
	TAILQ_ENTRY(ec_htable_elt_ref) next;
	TAILQ_ENTRY(ec_htable_elt_ref) hnext;
	struct ec_htable_elt *elt;
};

TAILQ_HEAD(ec_htable_elt_ref_list, ec_htable_elt_ref);

struct ec_htable {
	size_t len;
	size_t table_size;
	struct ec_htable_elt_ref_list list;
	struct ec_htable_elt_ref_list *table;
};
