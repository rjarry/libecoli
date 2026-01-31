/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <ecoli.h>

static void dump_schema(FILE *stdout, const struct ec_node_type *type)
{
	fprintf(stdout,
		"\n"
		"## %s\n"
		"\n",
		type->name);

	if (type->schema == NULL) {
		fprintf(stdout, "No generic schema.\n");
		return;
	}

	fprintf(stdout, "```\n");
	ec_config_schema_dump(stdout, type->schema, type->name);
	fprintf(stdout, "```\n");
}

static bool check_dir(const char *path)
{
	DIR *dir = opendir(path);

	if (dir == NULL)
		return false;

	closedir(dir);

	return true;
}

int main(void)
{
	struct ec_node_type *type;

	if (!check_dir("doc")) {
		fprintf(stderr, "Failed to open doc directory: %s\n", strerror(errno));
		fprintf(stderr, "Ensure that you run this tool from libecoli root directory\n");
		return 1;
	}

	fprintf(stdout,
		"# Node schemas\n"
		"\n"
		"This page lists all grammar node types supported by libecoli. For each node,\n"
		"it shows its generic configuration schema, if supported.\n");

	TAILQ_FOREACH (type, &node_type_list, next)
		dump_schema(stdout, type);

	return 0;
}
