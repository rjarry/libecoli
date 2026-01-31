/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_yaml YAML import/export
 * @{
 *
 * Interface to import/export ecoli data structures in YAML.
 */

#pragma once

#include <stdio.h>

struct ec_node;

/**
 * Parse a YAML file and build an ::ec_node tree from it.
 *
 * @param filename
 *   The path to the file to be parsed.
 * @return
 *   The ec_node tree on success, or NULL on error (errno is set).
 *   The returned node must be freed by the caller with ec_node_free().
 */
struct ec_node *ec_yaml_import(const char *filename);

/**
 * Export an ::ec_node tree to a YAML formatted stream.
 *
 * This function traverses the ::ec_node tree and outputs a YAML representation
 * of the grammar structure including node type, id, help, attributes and
 * configuration. The output can be used as a template or documentation
 * for grammar definitions.
 *
 * @param out
 *   The output stream where YAML content will be written.
 * @param node
 *   The root node of the grammar tree to export.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_yaml_export(FILE *out, const struct ec_node *node);

/** @} */
