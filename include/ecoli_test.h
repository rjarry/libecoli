/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_test Test
 * @{
 *
 * @brief Helpers for unit tests
 */

#pragma once

#include <sys/queue.h>

#include <ecoli_log.h>
#include <ecoli_utils.h>

struct ec_node;
enum ec_comp_type;

/**
 * Register a test case.
 *
 * @param t
 *   A pointer to a ec_test structure describing the test
 *   to be registered.
 *
 * @internal
 */
#define EC_TEST_REGISTER(t)						\
	static void ec_test_init_##t(void);				\
	static void __attribute__((constructor, used))			\
	ec_test_init_##t(void)						\
	{								\
		if (ec_test_register(&t) < 0)				\
			fprintf(stderr, "cannot register test %s\n",	\
				t.name);				\
	}

/**
 * Type of test function. Return 0 on success, -1 on error.
 *
 * @internal
 */
typedef int (ec_test_t)(void);

/**
 * Test linked list.
 *
 * @internal
 */
TAILQ_HEAD(ec_test_list, ec_test);

/**
 * A structure describing a test case.
 *
 * @internal
 */
struct ec_test {
	TAILQ_ENTRY(ec_test) next;  /**< Next in list. */
	const char *name;           /**< Test name. */
	ec_test_t *test;            /**< Test function. */
};

/**
 * Register a test case.
 *
 * @internal
 *
 * @param test
 *   A pointer to a ec_test structure describing the test
 *   to be registered.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_test_register(struct ec_test *test);

/**
 * Run all tests.
 *
 * @internal
 */
int ec_test_all(void);

/**
 * Run one test.
 *
 * @internal
 */
int ec_test_one(const char *name);

/**
 * expected == -1 means no match
 * @internal
 */
int ec_test_check_parse(struct ec_node *node, int expected, ...);

/**
 * Fail a test with a message.
 *
 * @internal
 */
#define EC_TEST_ERR(fmt, ...)						\
	EC_LOG(EC_LOG_ERR, "%s:%d: error: " fmt "\n",			\
		__FILE__, __LINE__, ##__VA_ARGS__);			\

/**
 * Verify a condition or fail a test with a message.
 *
 * @internal
 */
#define EC_TEST_CHECK(cond, fmt, ...) ({				\
	int ret_ = 0;							\
	if (!(cond)) {							\
		EC_TEST_ERR("(" #cond ") is wrong. " fmt		\
			##__VA_ARGS__);					\
		ret_ = -1;						\
	}								\
	ret_;								\
})

/**
 * node, input, [expected1, expected2, ...]
 *
 * @internal
 */
#define EC_TEST_CHECK_PARSE(node, args...) ({				\
	int ret_ = ec_test_check_parse(node, args, EC_VA_END);	\
	if (ret_)							\
		EC_TEST_ERR("parse test failed");			\
	ret_;								\
})

/** @internal */
int ec_test_check_complete(struct ec_node *node,
			enum ec_comp_type type, ...);

/** @internal */
#define EC_TEST_CHECK_COMPLETE(node, args...) ({			\
	int ret_ = ec_test_check_complete(node, EC_COMP_FULL, args);	\
	if (ret_)							\
		EC_TEST_ERR("complete test failed");			\
	ret_;								\
})

/** @internal */
#define EC_TEST_CHECK_COMPLETE_PARTIAL(node, args...) ({		\
	int ret_ = ec_test_check_complete(node, EC_COMP_PARTIAL, args);	\
	if (ret_)							\
		EC_TEST_ERR("complete test failed");			\
	ret_;								\
})

/** @} */
