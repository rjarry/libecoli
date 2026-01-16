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

#include <ecoli.h>

/** @internal */
#define EC_TEST_MAIN()                                                                             \
	EC_LOG_TYPE_REGISTER(__file__);                                                            \
	static void __attribute__((constructor, used)) __init(void)                                \
	{                                                                                          \
		ec_htable_force_seed(42);                                                          \
		ec_init();                                                                         \
	}                                                                                          \
	static void __attribute__((destructor, used)) __exit(void)                                 \
	{                                                                                          \
		ec_exit();                                                                         \
	}                                                                                          \
	int main(void)

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
#define EC_TEST_ERR(fmt, ...)                                                                      \
	EC_LOG(EC_LOG_ERR, "%s:%d: error: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);

/**
 * Verify a condition or fail a test with a message.
 *
 * @internal
 */
#define EC_TEST_CHECK(cond, fmt, ...)                                                              \
	({                                                                                         \
		int ret_ = 0;                                                                      \
		if (!(cond)) {                                                                     \
			EC_TEST_ERR("(" #cond ") is wrong. " fmt, ##__VA_ARGS__);                  \
			ret_ = -1;                                                                 \
		}                                                                                  \
		ret_;                                                                              \
	})

/**
 * node, input, [expected1, expected2, ...]
 *
 * @internal
 */
#define EC_TEST_CHECK_PARSE(node, args...)                                                         \
	({                                                                                         \
		int ret_ = ec_test_check_parse(node, args, EC_VA_END);                             \
		if (ret_)                                                                          \
			EC_TEST_ERR("parse test failed");                                          \
		ret_;                                                                              \
	})

/** @internal */
int ec_test_check_complete(struct ec_node *node, enum ec_comp_type type, ...);

/** @internal */
#define EC_TEST_CHECK_COMPLETE(node, args...)                                                      \
	({                                                                                         \
		int ret_ = ec_test_check_complete(node, EC_COMP_FULL, args);                       \
		if (ret_)                                                                          \
			EC_TEST_ERR("complete test failed");                                       \
		ret_;                                                                              \
	})

/** @internal */
#define EC_TEST_CHECK_COMPLETE_PARTIAL(node, args...)                                              \
	({                                                                                         \
		int ret_ = ec_test_check_complete(node, EC_COMP_PARTIAL, args);                    \
		if (ret_)                                                                          \
			EC_TEST_ERR("complete test failed");                                       \
		ret_;                                                                              \
	})

/** @} */
