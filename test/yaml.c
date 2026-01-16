/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2026, Free Mobile, Vincent Jardin <vjardin@free.fr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test.h"

/*
 * Test YAML import and export round-trip.
 *
 * This test verifies that:
 * 1. A node tree can be exported to YAML
 * 2. The exported YAML can be imported back
 * 3. The re-imported node parses the same inputs
 */

EC_TEST_MAIN()
{
	struct ec_node *node1 = NULL, *node2 = NULL;
	struct ec_pnode *p1 = NULL, *p2 = NULL;
	char tmpfile[] = "/tmp/ecoli_yaml_test_XXXXXX";
	FILE *fp = NULL;
	int fd = -1;
	int testres = 0;

	/* Test 1: export/import roundtrip */

	/* Build a simple grammar: "hello" | "world" */
	node1 = EC_NODE_OR(
		EC_NO_ID, ec_node_str("hello_id", "hello"), ec_node_str("world_id", "world")
	);
	if (node1 == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot create node\n");
		return -1;
	}

	/* Create a temporary file for export */
	fd = mkstemp(tmpfile);
	if (fd < 0) {
		EC_LOG(EC_LOG_ERR, "cannot create temp file\n");
		ec_node_free(node1);
		return -1;
	}

	fp = fdopen(fd, "w");
	if (fp == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot fdopen temp file\n");
		close(fd);
		unlink(tmpfile);
		ec_node_free(node1);
		return -1;
	}

	/* Export the node to YAML */
	if (ec_yaml_export(fp, node1) < 0) {
		EC_LOG(EC_LOG_ERR, "cannot export node to YAML\n");
		fclose(fp);
		unlink(tmpfile);
		ec_node_free(node1);
		return -1;
	}
	fclose(fp);

	/* Re-import the exported YAML */
	node2 = ec_yaml_import(tmpfile);
	if (node2 == NULL) {
		EC_LOG(EC_LOG_ERR, "cannot re-import exported YAML\n");
		unlink(tmpfile);
		ec_node_free(node1);
		return -1;
	}

	/* Verify both nodes parse "hello" */
	p1 = ec_parse(node1, "hello");
	p2 = ec_parse(node2, "hello");
	testres |= EC_TEST_CHECK(p1 != NULL && ec_pnode_matches(p1), "node1 should match 'hello'");
	testres |= EC_TEST_CHECK(p2 != NULL && ec_pnode_matches(p2), "node2 should match 'hello'");
	ec_pnode_free(p1);
	ec_pnode_free(p2);

	/* Verify both nodes parse "world" */
	p1 = ec_parse(node1, "world");
	p2 = ec_parse(node2, "world");
	testres |= EC_TEST_CHECK(p1 != NULL && ec_pnode_matches(p1), "node1 should match 'world'");
	testres |= EC_TEST_CHECK(p2 != NULL && ec_pnode_matches(p2), "node2 should match 'world'");
	ec_pnode_free(p1);
	ec_pnode_free(p2);

	/* Verify both nodes reject invalid input */
	p1 = ec_parse(node1, "invalid");
	p2 = ec_parse(node2, "invalid");
	testres |= EC_TEST_CHECK(
		p1 == NULL || !ec_pnode_matches(p1), "node1 should not match 'invalid'"
	);
	testres |= EC_TEST_CHECK(
		p2 == NULL || !ec_pnode_matches(p2), "node2 should not match 'invalid'"
	);
	ec_pnode_free(p1);
	ec_pnode_free(p2);

	ec_node_free(node1);
	ec_node_free(node2);
	unlink(tmpfile);

	/* Test 2: export handles NULL arguments */
	testres |= EC_TEST_CHECK(
		ec_yaml_export(NULL, NULL) < 0, "export should fail with NULL arguments"
	);

	return testres;
}
