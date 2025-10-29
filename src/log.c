/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <ecoli/log.h>
#include <ecoli/string.h>

/* Test exports */
int ec_log_lookup(const char *name);

EC_LOG_TYPE_REGISTER(log);

ec_log_t ec_log_fct = ec_log_default_cb;
void *ec_log_opaque;

struct ec_log_type {
	char *name;
	enum ec_log_level level;
};

static struct ec_log_type *log_types;
static size_t log_types_len;
static enum ec_log_level global_level = EC_LOG_WARNING;

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

int ec_log_default_cb(int type, enum ec_log_level level, void *opaque,
		const char *str)
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

int
ec_log_lookup(const char *name)
{
	size_t i;

	for (i = 0; i < log_types_len; i++) {
		if (log_types[i].name == NULL)
			continue;
		if (strcmp(name, log_types[i].name) == 0)
			return i;
	}

	return -1;
}

int
ec_log_type_register(const char *name)
{
	struct ec_log_type *new_types;
	char *copy;
	int id;

	id = ec_log_lookup(name);
	if (id >= 0)
		return id;

	// XXX not that good to allocate in constructor
	new_types = realloc(log_types,
		sizeof(*new_types) * (log_types_len + 1));
	if (new_types == NULL)
		return -1; /* errno is set */
	log_types = new_types;

	copy = strdup(name);
	if (copy == NULL)
		return -1; /* errno is set */

	id = log_types_len++;
	log_types[id].name = copy;
	log_types[id].level = EC_LOG_DEBUG;

	return id;
}

const char *
ec_log_name(int type)
{
	if (type < 0 || (unsigned int)type >= log_types_len)
		return "unknown";
	return log_types[type].name;
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
