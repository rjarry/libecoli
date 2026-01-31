/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2025, Olivier MATZ <zer0@droids-corp.org>
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli.h>

/* Map node type names to doxygen group names when they differ. */
static const char *type_to_group(const char *name)
{
	if (strcmp(name, "uint") == 0)
		return "int";
	return name;
}

static void dump_schema(FILE *f, const struct ec_node_type *type)
{
	fprintf(f, "@addtogroup ecoli_node_%s\n", type_to_group(type->name));
	fprintf(f, "@{\n\n");
	fprintf(f, "<b>Configuration Schema</b>\n\n");

	if (type->schema == NULL) {
		fprintf(f, "No configuration schema.\n");
	} else {
		fprintf(f, "```c\n");
		ec_config_schema_dump(f, type->schema, type->name);
		fprintf(f, "```\n");
	}

	fprintf(f, "\n@}\n");
}

int main(int argc, char **argv)
{
	struct ec_node_type *type;
	const char *dir, *stamp;
	char fname[PATH_MAX];
	FILE *f;

	if (argc != 3)
		errx(EXIT_FAILURE, "invalid arguments. usage: %s DIR STAMP_FILE", argv[0]);

	dir = argv[1];
	stamp = argv[2];

	TAILQ_FOREACH (type, &node_type_list, next) {
		snprintf(fname, sizeof(fname), "%s/node-%s-schema.md", dir, type->name);
		printf("generating %s ...\n", fname);
		f = fopen(fname, "w");
		if (f == NULL)
			errx(EXIT_FAILURE, "failed to create file: %s", fname);
		dump_schema(f, type);
		fclose(f);
	}

	f = fopen(stamp, "w");
	if (f == NULL)
		errx(EXIT_FAILURE, "failed to created stamp file: %s", stamp);
	fclose(f);

	return 0;
}
