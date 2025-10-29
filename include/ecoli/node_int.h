/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <stdint.h>

#include <ecoli/node.h>

/* ec_node("int", ...) can be used too
 * default is no limit, base 10 */

struct ec_node *ec_node_int(const char *id, int64_t min, int64_t max, unsigned int base);

int ec_node_int_getval(const struct ec_node *node, const char *str, int64_t *result);

struct ec_node *ec_node_uint(const char *id, uint64_t min, uint64_t max, unsigned int base);

int ec_node_uint_getval(const struct ec_node *node, const char *str, uint64_t *result);

/** @} */
