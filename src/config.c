/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <ecoli/config.h>
#include <ecoli/dict.h>
#include <ecoli/log.h>
#include <ecoli/node.h>
#include <ecoli/string.h>
#include <ecoli/utils.h>

EC_LOG_TYPE_REGISTER(config);

const char *ec_config_reserved_keys[] = {
	"id",
	"attrs",
	"help",
	"type",
};

static int
__ec_config_dump(FILE *out, const char *key, const struct ec_config *config, size_t indent);
static int
ec_config_dict_validate(const struct ec_dict *dict, const struct ec_config_schema *schema);

bool ec_config_key_is_reserved(const char *name)
{
	size_t i;

	for (i = 0; i < EC_COUNT_OF(ec_config_reserved_keys); i++) {
		if (!strcmp(name, ec_config_reserved_keys[i]))
			return true;
	}
	return false;
}

/* return ec_value type as a string */
static const char *ec_config_type_str(enum ec_config_type type)
{
	switch (type) {
	case EC_CONFIG_TYPE_BOOL:
		return "bool";
	case EC_CONFIG_TYPE_INT64:
		return "int64";
	case EC_CONFIG_TYPE_UINT64:
		return "uint64";
	case EC_CONFIG_TYPE_STRING:
		return "string";
	case EC_CONFIG_TYPE_NODE:
		return "node";
	case EC_CONFIG_TYPE_LIST:
		return "list";
	case EC_CONFIG_TYPE_DICT:
		return "dict";
	default:
		return "unknown";
	}
}

static size_t ec_config_schema_len(const struct ec_config_schema *schema)
{
	size_t i;

	if (schema == NULL)
		return 0;
	for (i = 0; schema[i].type != EC_CONFIG_TYPE_NONE; i++)
		;
	return i;
}

static int
__ec_config_schema_validate(const struct ec_config_schema *schema, enum ec_config_type type)
{
	size_t i, j;
	int ret;

	if (type == EC_CONFIG_TYPE_LIST) {
		if (schema[0].key != NULL) {
			errno = EINVAL;
			EC_LOG(EC_LOG_ERR, "list schema key must be NULL\n");
			return -1;
		}
	} else if (type == EC_CONFIG_TYPE_DICT) {
		for (i = 0; schema[i].type != EC_CONFIG_TYPE_NONE; i++) {
			if (schema[i].key == NULL) {
				errno = EINVAL;
				EC_LOG(EC_LOG_ERR, "dict schema key should not be NULL\n");
				return -1;
			}
		}
	} else {
		errno = EINVAL;
		EC_LOG(EC_LOG_ERR, "invalid schema type\n");
		return -1;
	}

	for (i = 0; schema[i].type != EC_CONFIG_TYPE_NONE; i++) {
		if (schema[i].key != NULL && ec_config_key_is_reserved(schema[i].key)) {
			errno = EINVAL;
			EC_LOG(EC_LOG_ERR, "key name <%s> is reserved\n", schema[i].key);
			return -1;
		}
		/* check for duplicate name if more than one element */
		for (j = i + 1; schema[j].type != EC_CONFIG_TYPE_NONE; j++) {
			if (!strcmp(schema[i].key, schema[j].key)) {
				errno = EEXIST;
				EC_LOG(EC_LOG_ERR, "duplicate key <%s> in schema\n", schema[i].key);
				return -1;
			}
		}

		switch (schema[i].type) {
		case EC_CONFIG_TYPE_BOOL:
		case EC_CONFIG_TYPE_INT64:
		case EC_CONFIG_TYPE_UINT64:
		case EC_CONFIG_TYPE_STRING:
		case EC_CONFIG_TYPE_NODE:
			if (schema[i].subschema != NULL
			    || ec_config_schema_len(schema[i].subschema) != 0) {
				errno = EINVAL;
				EC_LOG(EC_LOG_ERR,
				       "key <%s> should not have subtype/subschema\n",
				       schema[i].key);
				return -1;
			}
			break;
		case EC_CONFIG_TYPE_LIST:
			if (schema[i].subschema == NULL
			    || ec_config_schema_len(schema[i].subschema) != 1) {
				errno = EINVAL;
				EC_LOG(EC_LOG_ERR,
				       "key <%s> must have subschema of length 1\n",
				       schema[i].key);
				return -1;
			}
			break;
		case EC_CONFIG_TYPE_DICT:
			if (schema[i].subschema == NULL
			    || ec_config_schema_len(schema[i].subschema) == 0) {
				errno = EINVAL;
				EC_LOG(EC_LOG_ERR, "key <%s> must have subschema\n", schema[i].key);
				return -1;
			}
			break;
		default:
			EC_LOG(EC_LOG_ERR, "invalid type for key <%s>\n", schema[i].key);
			errno = EINVAL;
			return -1;
		}

		if (schema[i].subschema == NULL)
			continue;

		ret = __ec_config_schema_validate(schema[i].subschema, schema[i].type);
		if (ret < 0) {
			EC_LOG(EC_LOG_ERR,
			       "cannot parse subschema %s%s\n",
			       schema[i].key ? "key=" : "",
			       schema[i].key ?: "");
			return ret;
		}
	}

	return 0;
}

int ec_config_schema_validate(const struct ec_config_schema *schema)
{
	return __ec_config_schema_validate(schema, EC_CONFIG_TYPE_DICT);
}

static void __ec_config_schema_dump(FILE *out, const struct ec_config_schema *schema, size_t indent)
{
	const struct ec_config_schema *sch;
	char *wrapped_desc = NULL;
	char *desc = NULL;
	size_t i;

	for (i = 0; schema[i].type != EC_CONFIG_TYPE_NONE; i++) {
		sch = &schema[i];

		fprintf(out,
			"%*s%s %s%s {\n",
			(int)indent * 4,
			"",
			ec_config_type_str(sch->type),
			sch->key ?: "",
			sch->key ? " " : "");
		indent += 1;
		wrapped_desc = NULL;
		desc = ec_str_quote(sch->desc, 0, true);
		if (desc != NULL)
			wrapped_desc = ec_str_wrap(desc, 100, 12 + 4 * indent);
		fprintf(out, "%*sdescription %s;\n", (int)indent * 4, "", wrapped_desc);
		free(desc);
		free(wrapped_desc);
		if (sch->flags & EC_CONFIG_F_MANDATORY)
			fprintf(out, "%*smandatory true;\n", (int)indent * 4, "");

		if (sch->subschema != NULL) {
			fprintf(out, "\n");
			__ec_config_schema_dump(out, sch->subschema, indent);
		}
		indent -= 1;
		fprintf(out, "%*s}\n", (int)indent * 4, "");
	}
}

void ec_config_schema_dump(FILE *out, const struct ec_config_schema *schema, const char *name)
{
	if (schema == NULL) {
		fprintf(out, "no schema\n");
		return;
	}

	fprintf(out, "schema %s {\n", name ?: "unknown-name");
	__ec_config_schema_dump(out, schema, 1);
	fprintf(out, "}\n");
}

enum ec_config_type ec_config_get_type(const struct ec_config *config)
{
	return config->type;
}

struct ec_config *ec_config_bool(bool boolean)
{
	struct ec_config *value = NULL;

	value = calloc(1, sizeof(*value));
	if (value == NULL)
		return NULL;

	value->type = EC_CONFIG_TYPE_BOOL;
	value->boolean = boolean;

	return value;
}

struct ec_config *ec_config_i64(int64_t i64)
{
	struct ec_config *value = NULL;

	value = calloc(1, sizeof(*value));
	if (value == NULL)
		return NULL;

	value->type = EC_CONFIG_TYPE_INT64;
	value->i64 = i64;

	return value;
}

struct ec_config *ec_config_u64(uint64_t u64)
{
	struct ec_config *value = NULL;

	value = calloc(1, sizeof(*value));
	if (value == NULL)
		return NULL;

	value->type = EC_CONFIG_TYPE_UINT64;
	value->u64 = u64;

	return value;
}

/* duplicate string */
struct ec_config *ec_config_string(const char *string)
{
	struct ec_config *value = NULL;
	char *s = NULL;

	if (string == NULL)
		goto fail;

	s = strdup(string);
	if (s == NULL)
		goto fail;

	value = calloc(1, sizeof(*value));
	if (value == NULL)
		goto fail;

	value->type = EC_CONFIG_TYPE_STRING;
	value->string = s;

	return value;

fail:
	free(value);
	free(s);
	return NULL;
}

/* "consume" the node */
struct ec_config *ec_config_node(struct ec_node *node)
{
	struct ec_config *value = NULL;

	if (node == NULL)
		goto fail;

	value = calloc(1, sizeof(*value));
	if (value == NULL)
		goto fail;

	value->type = EC_CONFIG_TYPE_NODE;
	value->node = node;

	return value;

fail:
	ec_node_free(node);
	free(value);
	return NULL;
}

struct ec_config *ec_config_dict(void)
{
	struct ec_config *value = NULL;
	struct ec_dict *dict = NULL;

	dict = ec_dict();
	if (dict == NULL)
		goto fail;

	value = calloc(1, sizeof(*value));
	if (value == NULL)
		goto fail;

	value->type = EC_CONFIG_TYPE_DICT;
	value->dict = dict;

	return value;

fail:
	ec_dict_free(dict);
	free(value);
	return NULL;
}

struct ec_config *ec_config_list(void)
{
	struct ec_config *value = NULL;

	value = calloc(1, sizeof(*value));
	if (value == NULL)
		return NULL;

	value->type = EC_CONFIG_TYPE_LIST;
	TAILQ_INIT(&value->list);

	return value;
}

ssize_t ec_config_count(const struct ec_config *config)
{
	const struct ec_config *child;
	ssize_t n;

	switch (config->type) {
	case EC_CONFIG_TYPE_LIST:
		n = 0;
		TAILQ_FOREACH (child, &config->list, next)
			n++;
		return n;
	case EC_CONFIG_TYPE_DICT:
		/* TODO */
	default:
		errno = EINVAL;
		return -1;
	}
}

const struct ec_config_schema *
ec_config_schema_lookup(const struct ec_config_schema *schema, const char *key)
{
	size_t i;

	for (i = 0; schema[i].type != EC_CONFIG_TYPE_NONE; i++) {
		if (!strcmp(key, schema[i].key))
			return &schema[i];
	}

	errno = ENOENT;
	return NULL;
}

enum ec_config_type ec_config_schema_type(const struct ec_config_schema *schema_elt)
{
	return schema_elt->type;
}

const struct ec_config_schema *ec_config_schema_sub(const struct ec_config_schema *schema_elt)
{
	return schema_elt->subschema;
}

void ec_config_free(struct ec_config *value)
{
	if (value == NULL)
		return;

	switch (value->type) {
	case EC_CONFIG_TYPE_STRING:
		free(value->string);
		break;
	case EC_CONFIG_TYPE_NODE:
		ec_node_free(value->node);
		break;
	case EC_CONFIG_TYPE_LIST:
		while (!TAILQ_EMPTY(&value->list)) {
			struct ec_config *v;
			v = TAILQ_FIRST(&value->list);
			TAILQ_REMOVE(&value->list, v, next);
			ec_config_free(v);
		}
		break;
	case EC_CONFIG_TYPE_DICT:
		ec_dict_free(value->dict);
		break;
	default:
		break;
	}

	free(value);
}

static int
ec_config_list_cmp(const struct ec_config_list *list1, const struct ec_config_list *list2)
{
	const struct ec_config *v1, *v2;

	for (v1 = TAILQ_FIRST(list1), v2 = TAILQ_FIRST(list2); v1 != NULL && v2 != NULL;
	     v1 = TAILQ_NEXT(v1, next), v2 = TAILQ_NEXT(v2, next)) {
		if (ec_config_cmp(v1, v2))
			return -1;
	}
	if (v1 != NULL || v2 != NULL)
		return -1;

	return 0;
}

static int ec_config_dict_cmp(const struct ec_dict *d1, const struct ec_dict *d2)
{
	const struct ec_config *v1, *v2;
	struct ec_dict_elt_ref *iter = NULL;
	const char *key;

	if (ec_dict_len(d1) != ec_dict_len(d2))
		return -1;

	for (iter = ec_dict_iter(d1); iter != NULL; iter = ec_dict_iter_next(iter)) {
		key = ec_dict_iter_get_key(iter);
		v1 = ec_dict_iter_get_val(iter);
		v2 = ec_dict_get(d2, key);

		if (ec_config_cmp(v1, v2))
			goto fail;
	}

	return 0;

fail:
	return -1;
}

int ec_config_cmp(const struct ec_config *value1, const struct ec_config *value2)
{
	if (value1 == NULL || value2 == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (value1->type != value2->type)
		return -1;

	switch (value1->type) {
	case EC_CONFIG_TYPE_BOOL:
		if (value1->boolean == value2->boolean)
			return 0;
		break;
	case EC_CONFIG_TYPE_INT64:
		if (value1->i64 == value2->i64)
			return 0;
		break;
	case EC_CONFIG_TYPE_UINT64:
		if (value1->u64 == value2->u64)
			return 0;
		break;
	case EC_CONFIG_TYPE_STRING:
		if (!strcmp(value1->string, value2->string))
			return 0;
		break;
	case EC_CONFIG_TYPE_NODE:
		if (value1->node == value2->node)
			return 0;
		break;
	case EC_CONFIG_TYPE_LIST:
		return ec_config_list_cmp(&value1->list, &value2->list);
	case EC_CONFIG_TYPE_DICT:
		return ec_config_dict_cmp(value1->dict, value2->dict);
	default:
		break;
	}

	return -1;
}

static int
ec_config_list_validate(const struct ec_config_list *list, const struct ec_config_schema *sch)
{
	const struct ec_config *value;

	TAILQ_FOREACH (value, list, next) {
		if (value->type != sch->type) {
			errno = EBADMSG;
			return -1;
		}

		if (value->type == EC_CONFIG_TYPE_LIST) {
			if (ec_config_list_validate(&value->list, sch->subschema) < 0)
				return -1;
		} else if (value->type == EC_CONFIG_TYPE_DICT) {
			if (ec_config_dict_validate(value->dict, sch->subschema) < 0)
				return -1;
		}
	}

	return 0;
}

static int
ec_config_dict_validate(const struct ec_dict *dict, const struct ec_config_schema *schema)
{
	struct ec_dict_elt_ref *iter = NULL;
	const struct ec_config_schema *sch;
	const struct ec_config *value;
	const char *key;
	size_t i;

	for (i = 0; schema[i].type != EC_CONFIG_TYPE_NONE; i++) {
		key = schema[i].key;
		sch = &schema[i];

		value = ec_dict_get(dict, key);
		if (value == NULL && (sch->flags & EC_CONFIG_F_MANDATORY)) {
			errno = EBADMSG;
			goto fail;
		}

		if (value == NULL)
			continue;

		if (sch->type != value->type) {
			errno = EBADMSG;
			goto fail;
		}

		if (value->type == EC_CONFIG_TYPE_LIST) {
			if (ec_config_list_validate(&value->list, sch->subschema) < 0)
				goto fail;
		} else if (value->type == EC_CONFIG_TYPE_DICT) {
			if (ec_config_dict_validate(value->dict, sch->subschema) < 0)
				goto fail;
		}
	}

	for (iter = ec_dict_iter(dict); iter != NULL; iter = ec_dict_iter_next(iter)) {
		key = ec_dict_iter_get_key(iter);
		sch = ec_config_schema_lookup(schema, key);
		if (sch == NULL) {
			errno = EBADMSG;
			goto fail;
		}
	}

	return 0;

fail:
	return -1;
}

int ec_config_validate(const struct ec_config *dict, const struct ec_config_schema *schema)
{
	if (dict->type != EC_CONFIG_TYPE_DICT || schema == NULL) {
		errno = EINVAL;
		goto fail;
	}

	if (ec_config_dict_validate(dict->dict, schema) < 0)
		goto fail;

	return 0;
fail:
	return -1;
}

struct ec_config *ec_config_dict_get(const struct ec_config *config, const char *key)
{
	if (config == NULL) {
		errno = EINVAL;
		return NULL;
	}

	if (config->type != EC_CONFIG_TYPE_DICT) {
		errno = EINVAL;
		return NULL;
	}

	return ec_dict_get(config->dict, key);
}

struct ec_config *ec_config_list_first(struct ec_config *list)
{
	if (list == NULL || list->type != EC_CONFIG_TYPE_LIST) {
		errno = EINVAL;
		return NULL;
	}

	return TAILQ_FIRST(&list->list);
}

struct ec_config *ec_config_list_next(struct ec_config *list, struct ec_config *config)
{
	(void)list;
	return TAILQ_NEXT(config, next);
}

/* value is consumed */
int ec_config_dict_set(struct ec_config *config, const char *key, struct ec_config *value)
{
	void (*free_cb)(struct ec_config *) = ec_config_free;

	if (config == NULL || key == NULL || value == NULL) {
		errno = EINVAL;
		goto fail;
	}
	if (config->type != EC_CONFIG_TYPE_DICT) {
		errno = EINVAL;
		goto fail;
	}

	return ec_dict_set(config->dict, key, value, (void (*)(void *))free_cb);

fail:
	ec_config_free(value);
	return -1;
}

int ec_config_dict_del(struct ec_config *config, const char *key)
{
	if (config == NULL || key == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (config->type != EC_CONFIG_TYPE_DICT) {
		errno = EINVAL;
		return -1;
	}

	return ec_dict_del(config->dict, key);
}

/* value is consumed */
int ec_config_list_add(struct ec_config *list, struct ec_config *value)
{
	if (list == NULL || list->type != EC_CONFIG_TYPE_LIST || value == NULL) {
		errno = EINVAL;
		goto fail;
	}

	TAILQ_INSERT_TAIL(&list->list, value, next);

	return 0;

fail:
	ec_config_free(value);
	return -1;
}

int ec_config_list_del(struct ec_config *list, struct ec_config *config)
{
	if (list == NULL || list->type != EC_CONFIG_TYPE_LIST) {
		errno = EINVAL;
		return -1;
	}

	TAILQ_REMOVE(&list->list, config, next);
	ec_config_free(config);
	return 0;
}

static struct ec_config *ec_config_list_dup(const struct ec_config_list *list)
{
	struct ec_config *dup = NULL, *v, *value;

	dup = ec_config_list();
	if (dup == NULL)
		goto fail;

	TAILQ_FOREACH (v, list, next) {
		value = ec_config_dup(v);
		if (value == NULL)
			goto fail;
		if (ec_config_list_add(dup, value) < 0)
			goto fail;
	}

	return dup;

fail:
	ec_config_free(dup);
	return NULL;
}

static struct ec_config *ec_config_dict_dup(const struct ec_dict *dict)
{
	struct ec_config *dup = NULL, *value;
	struct ec_dict_elt_ref *iter = NULL;
	const char *key;

	dup = ec_config_dict();
	if (dup == NULL)
		goto fail;

	for (iter = ec_dict_iter(dict); iter != NULL; iter = ec_dict_iter_next(iter)) {
		key = ec_dict_iter_get_key(iter);
		value = ec_config_dup(ec_dict_iter_get_val(iter));
		if (value == NULL)
			goto fail;
		if (ec_config_dict_set(dup, key, value) < 0)
			goto fail;
	}

	return dup;

fail:
	ec_config_free(dup);
	return NULL;
}

struct ec_config *ec_config_dup(const struct ec_config *config)
{
	if (config == NULL) {
		errno = EINVAL;
		return NULL;
	}

	switch (config->type) {
	case EC_CONFIG_TYPE_BOOL:
		return ec_config_bool(config->boolean);
	case EC_CONFIG_TYPE_INT64:
		return ec_config_i64(config->i64);
	case EC_CONFIG_TYPE_UINT64:
		return ec_config_u64(config->u64);
	case EC_CONFIG_TYPE_STRING:
		return ec_config_string(config->string);
	case EC_CONFIG_TYPE_NODE:
		return ec_config_node(ec_node_clone(config->node));
	case EC_CONFIG_TYPE_LIST:
		return ec_config_list_dup(&config->list);
	case EC_CONFIG_TYPE_DICT:
		return ec_config_dict_dup(config->dict);
	default:
		errno = EINVAL;
		break;
	}

	return NULL;
}

static int
ec_config_list_dump(FILE *out, const char *key, const struct ec_config_list *list, size_t indent)
{
	const struct ec_config *v;

	fprintf(out,
		"%*s"
		"%s%s%stype=list\n",
		(int)indent * 4,
		"",
		key ? "key=" : "",
		key ? key : "",
		key ? " " : "");

	TAILQ_FOREACH (v, list, next) {
		if (__ec_config_dump(out, NULL, v, indent + 1) < 0)
			return -1;
	}

	return 0;
}

static int
ec_config_dict_dump(FILE *out, const char *key, const struct ec_dict *dict, size_t indent)
{
	const struct ec_config *value;
	struct ec_dict_elt_ref *iter;
	const char *k;

	fprintf(out,
		"%*s"
		"%s%s%stype=dict\n",
		(int)indent * 4,
		"",
		key ? "key=" : "",
		key ? key : "",
		key ? " " : "");

	for (iter = ec_dict_iter(dict); iter != NULL; iter = ec_dict_iter_next(iter)) {
		k = ec_dict_iter_get_key(iter);
		value = ec_dict_iter_get_val(iter);
		if (__ec_config_dump(out, k, value, indent + 1) < 0)
			goto fail;
	}
	return 0;

fail:
	return -1;
}

static int
__ec_config_dump(FILE *out, const char *key, const struct ec_config *value, size_t indent)
{
	char *val_str = NULL;
	int ret;

	switch (value->type) {
	case EC_CONFIG_TYPE_BOOL:
		if (value->boolean)
			ret = asprintf(&val_str, "true");
		else
			ret = asprintf(&val_str, "false");
		break;
	case EC_CONFIG_TYPE_INT64:
		ret = asprintf(&val_str, "%" PRIi64, value->i64);
		break;
	case EC_CONFIG_TYPE_UINT64:
		ret = asprintf(&val_str, "%" PRIu64, value->u64);
		break;
	case EC_CONFIG_TYPE_STRING:
		ret = asprintf(&val_str, "%s", value->string);
		break;
	case EC_CONFIG_TYPE_NODE:
		ret = asprintf(&val_str, "%p", value->node);
		break;
	case EC_CONFIG_TYPE_LIST:
		return ec_config_list_dump(out, key, &value->list, indent);
	case EC_CONFIG_TYPE_DICT:
		return ec_config_dict_dump(out, key, value->dict, indent);
	default:
		ret = -1;
		errno = EINVAL;
		break;
	}

	/* errno is already set on error */
	if (ret < 0 || val_str == NULL)
		goto fail;

	fprintf(out,
		"%*s"
		"%s%s%stype=%s val=%s\n",
		(int)indent * 4,
		"",
		key ? "key=" : "",
		key ? key : "",
		key ? " " : "",
		ec_config_type_str(value->type),
		val_str);

	free(val_str);
	return 0;

fail:
	free(val_str);
	return -1;
}

void ec_config_dump(FILE *out, const struct ec_config *config)
{
	fprintf(out, "------------------- config dump:\n");

	if (config == NULL) {
		fprintf(out, "no config\n");
		return;
	}

	if (__ec_config_dump(out, NULL, config, 0) < 0)
		fprintf(out, "error while dumping\n");
}
