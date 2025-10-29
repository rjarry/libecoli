/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ecoli/init.h>

static struct ec_init_list init_list = TAILQ_HEAD_INITIALIZER(init_list);

/* register an init function */
void ec_init_register(struct ec_init *init)
{
	struct ec_init *cur;

	if (TAILQ_EMPTY(&init_list)) {
		TAILQ_INSERT_HEAD(&init_list, init, next);
		return;
	}


	TAILQ_FOREACH(cur, &init_list, next) {
		if (init->priority > cur->priority)
			continue;

		TAILQ_INSERT_BEFORE(cur, init, next);
		return;
	}

	TAILQ_INSERT_TAIL(&init_list, init, next);
}

int ec_init(void)
{
	struct ec_init *init;

	TAILQ_FOREACH(init, &init_list, next) {
		if (init->init != NULL && init->init() < 0)
			return -1;
	}

	return 0;
}

void ec_exit(void)
{
	struct ec_init *init;

	TAILQ_FOREACH_REVERSE(init, &init_list, ec_init_list, next) {
		if (init->exit != NULL)
			init->exit();
	}
}
