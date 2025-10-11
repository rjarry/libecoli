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

struct ec_node;

/**
 * Parse a YAML file and build an ec_node tree from it.
 *
 * @param filename
 *   The path to the file to be parsed.
 * @return
 *   The ec_node tree on success, or NULL on error (errno is set).
 *   The returned node must be freed by the caller with ec_node_free().
 */
struct ec_node *ec_yaml_import(const char *filename);

/** @} */
