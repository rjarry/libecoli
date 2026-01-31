/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_log Log
 * @{
 *
 * @brief Log API
 *
 * This file provide logging helpers:
 * - logging functions, supporting printf-like format
 * - several debug level (similar to syslog)
 * - named log types
 * - redirection of log to a user functions (default logs nothing)
 */

#pragma once

#include <stdarg.h>
#include <sys/queue.h>
#include <syslog.h>

#include <ecoli/assert.h>

/**
 * Log levels, from most critical to least critical.
 */
enum ec_log_level {
	EC_LOG_EMERG = 0, /**< system is unusable */
	EC_LOG_ALERT = 1, /**< action must be taken immediately */
	EC_LOG_CRIT = 2, /**< critical conditions */
	EC_LOG_ERR = 3, /**< error conditions */
	EC_LOG_WARNING = 4, /**< warning conditions */
	EC_LOG_NOTICE = 5, /**< normal but significant condition */
	EC_LOG_INFO = 6, /**< informational */
	EC_LOG_DEBUG = 7, /**< debug-level messages */
};

/**
 * A structure describing a log type.
 *
 * It is defined as a static structure by the EC_LOG_TYPE_REGISTER() macro.
 */
struct ec_log_type {
	TAILQ_ENTRY(ec_log_type) next; /**< Next in list. */
	const char *name; /**< Log type name. */
	enum ec_log_level level; /**< The log level for this type. */
	int id; /**< The log identifier. */
};

/**
 * Register a log type.
 *
 * This macro defines a function that will be called at startup (using
 * the "constructor" attribute). This function registers the named type
 * passed as argument, and sets a static global variable
 * "ec_log_local_type". This variable is used as the default log type
 * for this file when using EC_LOG() or EC_VLOG().
 *
 * This macro can be present several times in a file. In this case, the
 * local log type is set to the last registered type.
 *
 * On error, the function aborts.
 *
 * @param name
 *   The name of the log to be registered.
 */
#define EC_LOG_TYPE_REGISTER(name)                                                                 \
	static struct ec_log_type name##_log_type = {                                              \
		.n##ame = #name,                                                                   \
	};                                                                                         \
	static int ec_log_local_type;                                                              \
	__attribute__((constructor, used)) static void ec_log_register_##name(void)                \
	{                                                                                          \
		ec_log_local_type = ec_log_type_register(&name##_log_type);                        \
		ec_assert_print(ec_log_local_type >= 0, "cannot register log type.\n");            \
	}

/**
 * User log function type.
 *
 * It is advised that a user-defined log function drops all messages
 * that are at least as critical as ec_log_level_get(), as done by the
 * default handler.
 *
 * @param type
 *   The log type identifier.
 * @param level
 *   The log level.
 * @param opaque
 *   The opaque pointer that was passed to ec_log_fct_register().
 * @param str
 *   The string to log.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
typedef int (*ec_log_t)(int type, enum ec_log_level level, void *opaque, const char *str);

/**
 * Register a user log function.
 *
 * @param usr_log
 *   Function pointer that will be invoked for each log call.
 *   If the parameter is NULL, ec_log_default_cb() is used.
 * @param opaque
 *   Opaque pointer passed to the log function.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_log_fct_register(ec_log_t usr_log, void *opaque);

/**
 * Register a named log type.
 *
 * Register a new log type, which is identified by its name. The
 * function returns a log identifier associated to the log name. If the
 * name is already registered, the function just returns its identifier.
 *
 * @param type
 *   The log type to register.
 * @return
 *   The log type identifier on success (positive or zero), -1 on
 *   error (errno is set).
 */
int ec_log_type_register(struct ec_log_type *type);

/**
 * Return the log name associated to the log type identifier.
 *
 * @param type
 *   The log type identifier.
 * @return
 *   The name associated to the log type, or "unknown". It always return
 *   a valid string (never NULL).
 */
const char *ec_log_name(int type);

/**
 * Log a formatted string.
 *
 * @param type
 *   The log type identifier.
 * @param level
 *   The log level.
 * @param format
 *   The format string, followed by optional arguments.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_log(int type, enum ec_log_level level, const char *format, ...)
	__attribute__((format(__printf__, 3, 4)));

/**
 * Log a formatted string.
 *
 * @param type
 *   The log type identifier.
 * @param level
 *   The log level.
 * @param format
 *   The format string.
 * @param ap
 *   The list of arguments.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_vlog(int type, enum ec_log_level level, const char *format, va_list ap);

/**
 * Log a formatted string using the local log type.
 *
 * This macro requires that a log type is previously register with
 * EC_LOG_TYPE_REGISTER() since it uses the "ec_log_local_type"
 * variable.
 *
 * @param level
 *   The log level.
 * @param args
 *   The format string, followed by optional arguments.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
#define EC_LOG(level, args...) ec_log(ec_log_local_type, level, args)

/**
 * Log a formatted string using the local log type.
 *
 * This macro requires that a log type is previously register with
 * EC_LOG_TYPE_REGISTER() since it uses the "ec_log_local_type"
 * variable.
 *
 * @param level
 *   The log level.
 * @param fmt
 *   The format string.
 * @param ap
 *   The list of arguments.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
#define EC_VLOG(level, fmt, ap) ec_vlog(ec_log_local_type, level, fmt, ap)

/**
 * Default log handler.
 *
 * This is the default log function that is used by the library. By
 * default, it prints all logs whose level is WARNING or more critical.
 * This level can be changed with ec_log_level_set().
 *
 * @param type
 *   The log type identifier.
 * @param level
 *   The log level.
 * @param opaque
 *   Unused.
 * @param str
 *   The string to be logged.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_log_default_cb(int type, enum ec_log_level level, void *opaque, const char *str);

/**
 * Set the global log level.
 *
 * This level is used by the default log handler, ec_log_default_cb().
 * All messages that are at least as critical as the default level are
 * displayed.
 *
 * It is advised
 *
 * @param level
 *   The log level to be set.
 * @return
 *   0 on success, -1 on error.
 */
int ec_log_level_set(enum ec_log_level level);

/**
 * Get the global log level.
 *
 * This level is used by the default log handler, ec_log_default_cb().
 * All messages that are at least as critical as the default level are
 * displayed.
 *
 * @return
 *   The current global log level
 */
enum ec_log_level ec_log_level_get(void);

/** @} */
