/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_config Node configuration
 * @{
 *
 * @brief Configure nodes behavior through generic API.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/queue.h>

struct ec_config;
struct ec_dict;

/**
 * The type identifier for a config value.
 */
enum ec_config_type {
	EC_CONFIG_TYPE_NONE = 0,
	EC_CONFIG_TYPE_BOOL,
	EC_CONFIG_TYPE_INT64,
	EC_CONFIG_TYPE_UINT64,
	EC_CONFIG_TYPE_STRING,
	EC_CONFIG_TYPE_NODE,
	EC_CONFIG_TYPE_LIST,
	EC_CONFIG_TYPE_DICT,
};

/**
 * Structure describing the format of a configuration value.
 *
 * This structure is used in a const array which is referenced by a
 * struct ec_config. Each entry of the array represents a key/value
 * storage of the configuration dictionary.
 */
struct ec_config_schema {
	const char *key; /**< The key string (NULL for list elts). */
	const char *desc; /**< A description of the value. */
	enum ec_config_type type; /**< Type of the value */
	/* XXX flags: mandatory */
	/* XXX default */

	/** If type is dict or list, the schema of the dict or list
	 * elements. Else must be NULL. */
	const struct ec_config_schema *subschema;
};

TAILQ_HEAD(ec_config_list, ec_config);

/**
 * Structure storing the configuration data.
 */
struct ec_config {
	/** type of value stored in the union */
	enum ec_config_type type;

	union {
		bool boolean; /**< Boolean value */
		int64_t i64; /**< Signed integer value */
		uint64_t u64; /**< Unsigned integer value */
		char *string; /**< String value */
		struct ec_node *node; /**< Node value */
		struct ec_dict *dict; /**< Hash table value */
		struct ec_config_list list; /**< List value */
	};

	/**
	 * Next in list, only valid if type is list.
	 */
	TAILQ_ENTRY(ec_config) next;
};

/* schema */

/**
 * Validate a configuration schema array.
 *
 * @param schema
 *   Pointer to the first element of the schema array. The array
 *   must be terminated by a sentinel entry (type == EC_CONFIG_TYPE_NONE).
 * @return
 *   0 if the schema is valid, or -1 on error (errno is set).
 */
int ec_config_schema_validate(const struct ec_config_schema *schema);

/**
 * Dump a configuration schema array.
 *
 * @param out
 *   Output stream on which the dump will be sent.
 * @param schema
 *   Pointer to the first element of the schema array. The array
 *   must be terminated by a sentinel entry (type == EC_CONFIG_TYPE_NONE).
 */
void ec_config_schema_dump(FILE *out, const struct ec_config_schema *schema);

/**
 * Find a schema entry matching the key.
 *
 * Browse the schema array and lookup for the given key.
 *
 * @param schema
 *   Pointer to the first element of the schema array. The array
 *   must be terminated by a sentinel entry (type == EC_CONFIG_TYPE_NONE).
 * @param key
 *   Schema key name.
 * @return
 *   The schema entry if it matches a key, or NULL if not found.
 */
const struct ec_config_schema *
ec_config_schema_lookup(const struct ec_config_schema *schema, const char *key);

/**
 * Get the type of a schema entry.
 *
 * @param schema_elt
 *   Pointer to an element of the schema array.
 * @return
 *   The type of the schema entry.
 */
enum ec_config_type ec_config_schema_type(const struct ec_config_schema *schema_elt);

/**
 * Get the subschema of a schema entry.
 *
 * @param schema_elt
 *   Pointer to an element of the schema array.
 * @return
 *   The subschema if any, or NULL.
 */
const struct ec_config_schema *ec_config_schema_sub(const struct ec_config_schema *schema_elt);

/**
 * Check if a key name is reserved in a config dict.
 *
 * Some key names are reserved and should not be used in configs.
 *
 * @param name
 *   The name of the key to test.
 * @return
 *   True if the key name is reserved and must not be used, else false.
 */
bool ec_config_key_is_reserved(const char *name);

/**
 * Array of reserved key names.
 */
extern const char *ec_config_reserved_keys[];

/* config */

/**
 * Get the type of the configuration.
 *
 * @param config
 *   The configuration.
 * @return
 *   The configuration type.
 */
enum ec_config_type ec_config_get_type(const struct ec_config *config);

/**
 * Create a boolean configuration value.
 *
 * @param boolean
 *   The boolean value to be set.
 * @return
 *   The configuration object, or NULL on error (errno is set).
 */
struct ec_config *ec_config_bool(bool boolean);

/**
 * Create a signed integer configuration value.
 *
 * @param i64
 *   The signed integer value to be set.
 * @return
 *   The configuration object, or NULL on error (errno is set).
 */
struct ec_config *ec_config_i64(int64_t i64);

/**
 * Create an unsigned configuration value.
 *
 * @param u64
 *   The unsigned integer value to be set.
 * @return
 *   The configuration object, or NULL on error (errno is set).
 */
struct ec_config *ec_config_u64(uint64_t u64);

/**
 * Create a string configuration value.
 *
 * @param string
 *   The string value to be set. The string is copied into the
 *   configuration object.
 * @return
 *   The configuration object, or NULL on error (errno is set).
 */
struct ec_config *ec_config_string(const char *string);

/**
 * Create a node configuration value.
 *
 * @param node
 *   The node pointer to be set. The node is "consumed" by
 *   the function and should not be used by the caller, even
 *   on error. The caller can use ec_node_clone() to keep a
 *   reference on the node.
 * @return
 *   The configuration object, or NULL on error (errno is set).
 */
struct ec_config *ec_config_node(struct ec_node *node);

/**
 * Create a hash table configuration value.
 *
 * @return
 *   A configuration object containing an empty hash table, or NULL on
 *   error (errno is set).
 */
struct ec_config *ec_config_dict(void);

/**
 * Create a list configuration value.
 *
 * @return
 *   The configuration object containing an empty list, or NULL on
 *   error (errno is set).
 */
struct ec_config *ec_config_list(void);

/**
 * Add a config object into a list.
 *
 * @param list
 *   The list configuration in which the value will be added.
 * @param value
 *   The value configuration to add in the list. The value object
 *   will be freed when freeing the list object. On error, the
 *   value object is also freed.
 * @return
 *   0 on success, else -1 (errno is set).
 */
int ec_config_list_add(struct ec_config *list, struct ec_config *value);

/**
 * Remove an element from a list.
 *
 * The element is freed and should not be accessed.
 *
 * @param list
 *   The list configuration.
 * @param config
 *   The element to remove from the list.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_config_list_del(struct ec_config *list, struct ec_config *config);

/**
 * Count the number of elements in a list or dict.
 *
 * @param config
 *   The configuration that must be a list or a dict.
 * @return
 *   The number of elements, or -1 on error (errno is set).
 */
ssize_t ec_config_count(const struct ec_config *config);

/**
 * Validate a configuration.
 *
 * @param dict
 *   A hash table configuration to validate.
 * @param schema
 *   Pointer to the first element of the schema array. The array
 *   must be terminated by a sentinel entry (type == EC_CONFIG_TYPE_NONE).
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_config_validate(const struct ec_config *dict, const struct ec_config_schema *schema);

/**
 * Set a value in a hash table configuration
 *
 * @param dict
 *   A hash table configuration to validate.
 * @param key
 *   The key to update.
 * @param value
 *   The value to set. The value object will be freed when freeing the
 *   dict object. On error, the value object is also freed.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_config_dict_set(struct ec_config *dict, const char *key, struct ec_config *value);

/**
 * Remove an element from a hash table configuration.
 *
 * The element is freed and should not be accessed.
 *
 * @param dict
 *   A hash table configuration to validate.
 * @param key
 *   The key of the configuration to delete.
 * @return
 *   0 on success, -1 on error (errno is set).
 */
int ec_config_dict_del(struct ec_config *dict, const char *key);

/**
 * Compare two configurations.
 */
int ec_config_cmp(const struct ec_config *config1, const struct ec_config *config2);

/**
 * Get configuration value.
 */
struct ec_config *ec_config_dict_get(const struct ec_config *config, const char *key);

/**
 * Get the first element of a list.
 *
 * Example of use:
 *
 * for (config = ec_config_list_iter(list);
 *	config != NULL;
 *	config = ec_config_list_next(list, config)) {
 *		...
 * }
 *
 * @param list
 *   The list configuration to iterate.
 * @return
 *   The first configuration element, or NULL on error (errno is set).
 */
struct ec_config *ec_config_list_first(struct ec_config *list);

/**
 * Get next element in list.
 *
 * @param list
 *   The list configuration beeing iterated.
 * @param config
 *   The current configuration element.
 * @return
 *   The next configuration element, or NULL if there is no more element.
 */
struct ec_config *ec_config_list_next(struct ec_config *list, struct ec_config *config);

/**
 * Free a configuration.
 *
 * @param config
 *   The element to free.
 */
void ec_config_free(struct ec_config *config);

/**
 * Compare two configurations.
 *
 * @return
 *   0 if the configurations are equal, else -1.
 */
int ec_config_cmp(const struct ec_config *value1, const struct ec_config *value2);

/**
 * Duplicate a configuration.
 *
 * @param config
 *   The configuration to duplicate.
 * @return
 *   The duplicated configuration, or NULL on error (errno is set).
 */
struct ec_config *ec_config_dup(const struct ec_config *config);

/**
 * Dump a configuration.
 *
 * @param out
 *   Output stream on which the dump will be sent.
 * @param config
 *   The configuration to dump.
 */
void ec_config_dump(FILE *out, const struct ec_config *config);

/** @} */
