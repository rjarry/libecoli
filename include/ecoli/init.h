/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @addtogroup ecoli_nodes
 * @{
 */

#pragma once

#include <sys/queue.h>

/**
 * Register initialization and exit callbacks. These callbacks are
 * ordered by priority: for initialization, the lowest priority is called
 * first. For exit, the callbacks are invoked in reverse order.
 *
 * Priority policy:
 *  0 .. 99 : reserved for libecoli internal use.
 *  100 ..  : available for user code (recommended).
 *
 * Do not use priorities < 100 for application code; internal libecoli
 * components may depend on those priorities and using them can lead to
 * uninitialized state, crashes, or undefined behaviour.
 */
#define EC_INIT_REGISTER(t)						\
	static void ec_init_init_##t(void);				\
	static void __attribute__((constructor, used))			\
	ec_init_init_##t(void)						\
	{								\
		 ec_init_register(&t);					\
	}

/**
 * Type of init function. Return 0 on success, -1 on error.
 */
typedef int (ec_init_t)(void);

/**
 * Type of exit function.
 */
typedef void (ec_exit_t)(void);

TAILQ_HEAD(ec_init_list, ec_init);

/**
 * A structure describing a test case.
 */
struct ec_init {
	TAILQ_ENTRY(ec_init) next;  /**< Next in list. */
	ec_init_t *init;            /**< Init function. */
	ec_exit_t *exit;            /**< Exit function. */
	unsigned int priority;      /**< Priority (0=first, 99=last) for internal */
};

/**
 * Register an initialization function.
 *
 * @param test
 *   A pointer to a ec_init structure to be registered.
 */
void ec_init_register(struct ec_init *test);

/**
 * Initialize ecoli library.
 *
 * Must be called before any other function from libecoli.
 *
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_init(void);

/**
 * Uninitialize ecoli library.
 */
void ec_exit(void);

/** @} */
