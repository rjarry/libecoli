/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <string.h>

#include "test.h"

static const struct ec_config_schema sch_intlist_elt[] = {
	{
		.desc = "This is a description for int",
		.type = EC_CONFIG_TYPE_INT64,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static const struct ec_config_schema sch_dict[] = {
	{
		.key = "my_int",
		.desc = "This is a description for int",
		.type = EC_CONFIG_TYPE_INT64,
	},
	{
		.key = "my_int2",
		.desc = "This is a description for int2",
		.type = EC_CONFIG_TYPE_INT64,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static const struct ec_config_schema sch_dictlist_elt[] = {
	{
		.desc = "This is a description for dict",
		.type = EC_CONFIG_TYPE_DICT,
		.subschema = sch_dict,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

static const struct ec_config_schema sch_baseconfig[] = {
	{
		.key = "my_bool",
		.desc = "This is a description for bool",
		.type = EC_CONFIG_TYPE_BOOL,
	},
	{
		.key = "my_int",
		.desc = "This is a description for int",
		.type = EC_CONFIG_TYPE_INT64,
	},
	{
		.key = "my_string",
		.desc = "This is a description for string",
		.type = EC_CONFIG_TYPE_STRING,
	},
	{
		.key = "my_node",
		.desc = "This is a description for node",
		.type = EC_CONFIG_TYPE_NODE,
	},
	{
		.key = "my_intlist",
		.desc = "This is a description for list",
		.type = EC_CONFIG_TYPE_LIST,
		.subschema = sch_intlist_elt,
	},
	{
		.key = "my_dictlist",
		.desc = "This is a description for list",
		.type = EC_CONFIG_TYPE_LIST,
		.subschema = sch_dictlist_elt,
	},
	{
		.type = EC_CONFIG_TYPE_NONE,
	},
};

EC_TEST_MAIN()
{
	struct ec_node *node = NULL;
	struct ec_dict *dict = NULL;
	const struct ec_config *value = NULL;
	struct ec_config *config = NULL, *config2 = NULL;
	struct ec_config *list = NULL, *subconfig = NULL;
	struct ec_config *list_, *config_;
	int testres = 0;
	int ret;

	testres |= EC_TEST_CHECK(ec_config_key_is_reserved("id"), "'id' should be reserved");
	testres |= EC_TEST_CHECK(!ec_config_key_is_reserved("foo"), "'foo' should not be reserved");

	node = ec_node("empty", EC_NO_ID);
	if (node == NULL)
		goto fail;

	if (ec_config_schema_validate(sch_baseconfig) < 0) {
		EC_LOG(EC_LOG_ERR, "invalid config schema\n");
		goto fail;
	}

	ec_config_schema_dump(stdout, sch_baseconfig);

	config = ec_config_dict();
	if (config == NULL)
		goto fail;

	ret = ec_config_dict_set(config, "my_bool", ec_config_bool(true));
	testres |= EC_TEST_CHECK(ret == 0, "cannot set boolean");
	value = ec_config_dict_get(config, "my_bool");
	testres |= EC_TEST_CHECK(
		value != NULL && value->type == EC_CONFIG_TYPE_BOOL && value->boolean == true,
		"unexpected boolean value"
	);

	ret = ec_config_dict_set(config, "my_int", ec_config_i64(1234));
	testres |= EC_TEST_CHECK(ret == 0, "cannot set int");
	value = ec_config_dict_get(config, "my_int");
	testres |= EC_TEST_CHECK(
		value != NULL && value->type == EC_CONFIG_TYPE_INT64 && value->i64 == 1234,
		"unexpected int value"
	);

	testres |= EC_TEST_CHECK(
		ec_config_validate(config, sch_baseconfig) == 0, "cannot validate config\n"
	);

	ret = ec_config_dict_set(config, "my_string", ec_config_string("toto"));
	testres |= EC_TEST_CHECK(ret == 0, "cannot set string");
	value = ec_config_dict_get(config, "my_string");
	testres |= EC_TEST_CHECK(
		value != NULL && value->type == EC_CONFIG_TYPE_STRING
			&& !strcmp(value->string, "toto"),
		"unexpected string value"
	);

	list = ec_config_list();
	if (list == NULL)
		goto fail;

	subconfig = ec_config_dict();
	if (subconfig == NULL)
		goto fail;

	ret = ec_config_dict_set(subconfig, "my_int", ec_config_i64(1));
	testres |= EC_TEST_CHECK(ret == 0, "cannot set int");
	value = ec_config_dict_get(subconfig, "my_int");
	testres |= EC_TEST_CHECK(
		value != NULL && value->type == EC_CONFIG_TYPE_INT64 && value->i64 == 1,
		"unexpected int value"
	);

	ret = ec_config_dict_set(subconfig, "my_int2", ec_config_i64(2));
	testres |= EC_TEST_CHECK(ret == 0, "cannot set int");
	value = ec_config_dict_get(subconfig, "my_int2");
	testres |= EC_TEST_CHECK(
		value != NULL && value->type == EC_CONFIG_TYPE_INT64 && value->i64 == 2,
		"unexpected int value"
	);

	testres |= EC_TEST_CHECK(
		ec_config_validate(subconfig, sch_dict) == 0, "cannot validate subconfig\n"
	);

	ret = ec_config_list_add(list, subconfig);
	subconfig = NULL; /* freed */
	testres |= EC_TEST_CHECK(ret == 0, "cannot add in list");

	subconfig = ec_config_dict();
	if (subconfig == NULL)
		goto fail;

	ret = ec_config_dict_set(subconfig, "my_int", ec_config_i64(3));
	testres |= EC_TEST_CHECK(ret == 0, "cannot set int");
	value = ec_config_dict_get(subconfig, "my_int");
	testres |= EC_TEST_CHECK(
		value != NULL && value->type == EC_CONFIG_TYPE_INT64 && value->i64 == 3,
		"unexpected int value"
	);

	ret = ec_config_dict_set(subconfig, "my_int2", ec_config_i64(4));
	testres |= EC_TEST_CHECK(ret == 0, "cannot set int");
	value = ec_config_dict_get(subconfig, "my_int2");
	testres |= EC_TEST_CHECK(
		value != NULL && value->type == EC_CONFIG_TYPE_INT64 && value->i64 == 4,
		"unexpected int value"
	);

	testres |= EC_TEST_CHECK(
		ec_config_validate(subconfig, sch_dict) == 0, "cannot validate subconfig\n"
	);

	ret = ec_config_list_add(list, subconfig);
	subconfig = NULL; /* freed */
	testres |= EC_TEST_CHECK(ret == 0, "cannot add in list");

	ret = ec_config_dict_set(config, "my_dictlist", list);
	list = NULL;
	testres |= EC_TEST_CHECK(ret == 0, "cannot set list");

	testres |= EC_TEST_CHECK(
		ec_config_validate(config, sch_baseconfig) == 0, "cannot validate config\n"
	);

	list_ = ec_config_dict_get(config, "my_dictlist");
	for (config_ = ec_config_list_first(list_); config_ != NULL;
	     config_ = ec_config_list_next(list_, config_)) {
		ec_config_dump(stdout, config_);
	}

	ec_config_dump(stdout, config);

	config2 = ec_config_dup(config);
	testres |= EC_TEST_CHECK(config2 != NULL, "cannot duplicate config");
	testres |= EC_TEST_CHECK(ec_config_cmp(config, config2) == 0, "fail to compare config");
	ec_config_free(config2);
	config2 = NULL;

	/* remove the first element */
	ec_config_list_del(list_, ec_config_list_first(list_));
	testres |= EC_TEST_CHECK(
		ec_config_validate(config, sch_baseconfig) == 0, "cannot validate config\n"
	);

	ec_config_dump(stdout, config);

	ec_config_free(list);
	ec_config_free(subconfig);
	ec_config_free(config);
	ec_dict_free(dict);
	ec_node_free(node);

	return testres;

fail:
	ec_config_free(list);
	ec_config_free(subconfig);
	ec_config_free(config);
	ec_config_free(config2);
	ec_dict_free(dict);
	ec_node_free(node);

	return -1;
}
