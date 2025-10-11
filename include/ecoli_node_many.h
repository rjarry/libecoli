/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

/*
 * if min == max == 0, there is no limit
 */
struct ec_node *ec_node_many(const char *id, struct ec_node *child,
	unsigned int min, unsigned int max);

int
ec_node_many_set_params(struct ec_node *gen_node, struct ec_node *child,
	unsigned int min, unsigned int max);

/** @} */
