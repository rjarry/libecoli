/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <string.h>
#include <syslog.h>

#include "test.h"

/* Internal symbols from src/log.c */
extern ec_log_t ec_log_fct;
extern void *ec_log_opaque;
extern int ec_log_lookup(const char *name);

EC_LOG_TYPE_REGISTER(log_test);

static int log_cb(int type, enum ec_log_level level, void *opaque, const char *str)
{
	(void)type;
	(void)level;
	(void)str;
	*(int *)opaque = 1;

	return 0;
}

EC_TEST_MAIN()
{
	ec_log_t prev_log_cb;
	void *prev_opaque;
	const char *logname;
	int testres = 0;
	int check_cb = 0;
	int logtype;
	int level;
	int ret;

	prev_log_cb = ec_log_fct;
	prev_opaque = ec_log_opaque;

	ret = ec_log_fct_register(log_cb, &check_cb);
	testres |= EC_TEST_CHECK(ret == 0, "cannot register log function\n");
	EC_LOG(LOG_ERR, "test\n");
	testres |= EC_TEST_CHECK(check_cb == 1, "log callback was not invoked\n");
	logtype = ec_log_lookup("dsdedesdes");
	testres |= EC_TEST_CHECK(logtype == -1, "lookup invalid name should return -1");
	logtype = ec_log_lookup("log");
	logname = ec_log_name(logtype);
	testres |= EC_TEST_CHECK(
		logname != NULL && !strcmp(logname, "log"), "cannot get log name\n"
	);
	logname = ec_log_name(-1);
	testres |= EC_TEST_CHECK(
		logname != NULL && !strcmp(logname, "unknown"), "cannot get invalid log name\n"
	);
	logname = ec_log_name(34324);
	testres |= EC_TEST_CHECK(
		logname != NULL && !strcmp(logname, "unknown"), "cannot get invalid log name\n"
	);
	level = ec_log_level_get();
	ret = ec_log_level_set(2);
	testres |= EC_TEST_CHECK(ret == 0 && ec_log_level_get() == 2, "cannot set log level\n");
	ret = ec_log_level_set(10);
	testres |= EC_TEST_CHECK(ret != 0, "should not be able to set log level\n");

	ec_log_fct_register(NULL, NULL);
	ec_log_level_set(LOG_DEBUG);
	EC_LOG(LOG_DEBUG, "test log\n");
	ec_log_level_set(LOG_INFO);
	EC_LOG(LOG_DEBUG, "test log (not displayed)\n");
	ec_log_level_set(level);

	ec_log_fct = prev_log_cb;
	ec_log_opaque = prev_opaque;

	return testres;
}
