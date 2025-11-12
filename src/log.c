/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <syslog.h>

#include <ecoli/log.h>
#include <ecoli/string.h>

/* Test exports */
int ec_log_lookup(const char *name);

EC_LOG_TYPE_REGISTER(log);

ec_log_t ec_log_fct = ec_log_default_cb;
void *ec_log_opaque;

static size_t log_types_len;
static enum ec_log_level global_level = EC_LOG_WARNING;

TAILQ_HEAD(ec_log_type_list, ec_log_type);
static struct ec_log_type_list log_type_list = TAILQ_HEAD_INITIALIZER(log_type_list);

int ec_log_level_set(enum ec_log_level level)
{
	if (level > EC_LOG_DEBUG)
		return -1;
	global_level = level;

	return 0;
}

enum ec_log_level ec_log_level_get(void)
{
	return global_level;
}

int ec_log_default_cb(int type, enum ec_log_level level, void *opaque, const char *str)
{
	(void)opaque;

	if (level > ec_log_level_get())
		return 0;

	if (fprintf(stderr, "[%d] %-12s %s", level, ec_log_name(type), str) < 0)
		return -1;

	return 0;
}

int ec_log_fct_register(ec_log_t usr_log, void *opaque)
{
	if (usr_log == NULL) {
		ec_log_fct = ec_log_default_cb;
		ec_log_opaque = NULL;
	} else {
		ec_log_fct = usr_log;
		ec_log_opaque = opaque;
	}

	return 0;
}

int ec_log_lookup(const char *name)
{
	struct ec_log_type *type;

	TAILQ_FOREACH (type, &log_type_list, next) {
		if (type->name != NULL && strcmp(name, type->name) == 0)
			return type->id;
	}

	return -1;
}

int ec_log_type_register(struct ec_log_type *type)
{
	int id;

	id = ec_log_lookup(type->name);
	if (id >= 0)
		return id;

	TAILQ_INSERT_HEAD(&log_type_list, type, next);
	type->level = EC_LOG_DEBUG;
	type->id = log_types_len++;

	return type->id;
}

const char *ec_log_name(int type_id)
{
	struct ec_log_type *type;

	TAILQ_FOREACH (type, &log_type_list, next) {
		if (type->id == type_id && type->name != NULL)
			return type->name;
	}

	return "unknown";
}

int ec_vlog(int type, enum ec_log_level level, const char *format, va_list ap)
{
	char *s;
	int ret;

	ret = vasprintf(&s, format, ap);
	if (ret < 0)
		return ret;

	ret = ec_log_fct(type, level, ec_log_opaque, s);
	free(s);

	return ret;
}

int ec_log(int type, enum ec_log_level level, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = ec_vlog(type, level, format, ap);
	va_end(ap);

	return ret;
}
