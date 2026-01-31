/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @example parse-yaml/parse-yaml.c
 * Load grammar from YAML and parse or complete input.
 *
 * Demonstrates loading a grammar definition from a YAML file using
 * ec_yaml_import() and using it to parse or complete command line input.
 *
 * Example session using test.yaml grammar:
 * @code
 * $ ./parse-yaml -i test.yaml -o /dev/stdout
 * parse-yaml> hello john
 * ec_node1_id='sh_lex'
 * ec_node1_type='sh_lex'
 * ec_node1_strvec_len=1
 * ec_node1_str0='hello john'
 * ec_node1_first_child='ec_node2'
 * ec_node2_id='hello'
 * ...
 * $ ./parse-yaml -i test.yaml -o /dev/null -c hello ""
 * john
 * mike
 * @endcode
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli.h>

static char *input_file;
static char *output_file;
static bool complete;

static const char short_options[] = "h" /* help */
				    "i:" /* input-file */
				    "o:" /* output-file */
				    "c" /* complete */
	;

#define OPT_HELP "help"
#define OPT_INPUT_FILE "input-file"
#define OPT_OUTPUT_FILE "output-file"
#define OPT_COMPLETE "complete"

static const struct option long_options[] = {
	{OPT_HELP, 0, NULL, 'h'},
	{OPT_INPUT_FILE, 1, NULL, 'i'},
	{OPT_OUTPUT_FILE, 1, NULL, 'o'},
	{OPT_COMPLETE, 0, NULL, 'c'},
	{NULL, 0, NULL, 0}
};

static void usage(const char *prgname)
{
	fprintf(stderr,
		"%s -o <file.sh> -i <file.yaml>\n"
		"  -h\n"
		"  --" OPT_HELP "\n"
		"      Show this help.\n"
		"  -i <input-file>\n"
		"  --" OPT_INPUT_FILE "=<file>\n"
		"      Set the yaml input file describing the grammar.\n"
		"  -o <output-file>\n"
		"  --" OPT_OUTPUT_FILE "=<file>\n"
		"      Set the output file.\n"
		"  -c\n"
		"  --" OPT_COMPLETE "\n"
		"      Output the completion list.\n",
		prgname);
}

static int parse_args(int argc, char **argv)
{
	int ret, opt;

	while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != EOF) {
		switch (opt) {
		case 'h': /* help */
			usage(argv[0]);
			exit(0);

		case 'i': /* input-file */
			input_file = strdup(optarg);
			break;

		case 'o': /* output-file */
			output_file = strdup(optarg);
			break;

		case 'c': /* complete */
			complete = 1;
			break;

		default:
			usage(argv[0]);
			return -1;
		}
	}

	if (input_file == NULL) {
		fprintf(stderr, "No input file\n");
		usage(argv[0]);
		return -1;
	}
	if (output_file == NULL) {
		fprintf(stderr, "No output file\n");
		usage(argv[0]);
		return -1;
	}

	ret = optind - 1;
	optind = 1;

	return ret;
}

static int __dump_as_shell(FILE *f, const struct ec_pnode *parse, size_t *seq)
{
	const struct ec_node *node = ec_pnode_get_node(parse);
	struct ec_pnode *child;
	size_t cur_seq, i, len;
	char *quoted;

	(*seq)++;
	cur_seq = *seq;

	quoted = ec_str_quote(ec_node_id(node), '\'', true);
	fprintf(f, "ec_node%zu_id=%s\n", cur_seq, quoted);
	free(quoted);

	quoted = ec_str_quote(ec_node_type_name(ec_node_type(node)), '\'', true);
	fprintf(f, "ec_node%zu_type=%s\n", cur_seq, quoted);
	free(quoted);

	len = ec_strvec_len(ec_pnode_get_strvec(parse));
	fprintf(f, "ec_node%zu_strvec_len=%zu\n", cur_seq, len);
	for (i = 0; i < len; i++) {
		quoted = ec_str_quote(ec_strvec_val(ec_pnode_get_strvec(parse), i), '\'', true);
		fprintf(f, "ec_node%zu_str%zu=%s\n", cur_seq, i, quoted);
		free(quoted);
	}

	if (ec_pnode_get_first_child(parse) != NULL) {
		fprintf(f, "ec_node%zu_first_child='ec_node%zu'\n", cur_seq, cur_seq + 1);
	}

	EC_PNODE_FOREACH_CHILD (child, parse) {
		fprintf(f, "ec_node%zu_parent='ec_node%zu'\n", *seq + 1, cur_seq);
		__dump_as_shell(f, child, seq);
	}

	if (ec_pnode_next(parse) != NULL) {
		fprintf(f, "ec_node%zu_next='ec_node%zu'\n", cur_seq, *seq + 1);
	}

	return 0;
}

static int dump_as_shell(const struct ec_pnode *parse)
{
	FILE *f;
	size_t seq = 0;
	int ret;

	f = fopen(output_file, "w");
	if (f == NULL)
		return -1;

	ret = __dump_as_shell(f, parse, &seq);

	fclose(f);

	return ret;
}

static int interact(struct ec_node *node)
{
	struct ec_editline *editline = NULL;
	struct ec_pnode *parse = NULL;
	struct ec_node *shlex = NULL;

	shlex = ec_node_sh_lex(EC_NO_ID, ec_node_clone(node));
	if (shlex == NULL) {
		fprintf(stderr, "Failed to add lexer node\n");
		goto fail;
	}

	editline = ec_editline("parse-yaml", stdin, stdout, stderr, 0);
	if (editline == NULL) {
		fprintf(stderr, "Failed to initialize editline\n");
		goto fail;
	}

	if (ec_editline_set_node(editline, shlex) < 0) {
		fprintf(stderr, "Failed to set editline ec_node\n");
		goto fail;
	}

	parse = ec_editline_parse(editline);
	if (parse == NULL)
		goto fail;

	if (!ec_pnode_matches(parse))
		goto fail;

	if (dump_as_shell(parse) < 0) {
		fprintf(stderr, "Failed to dump the parsed result\n");
		goto fail;
	}

	ec_pnode_free(parse);
	ec_editline_free(editline);
	ec_node_free(shlex);
	return 0;

fail:
	ec_pnode_free(parse);
	ec_editline_free(editline);
	ec_node_free(shlex);
	return -1;
}

static int complete_words(const struct ec_node *node, int argc, char *argv[])
{
	struct ec_comp *comp = NULL;
	struct ec_strvec *strvec = NULL;
	struct ec_comp_item *item = NULL;
	size_t count;

	if (argc <= 1)
		goto fail;
	strvec = ec_strvec_from_array((const char *const *)&argv[1], argc - 1);
	if (strvec == NULL)
		goto fail;

	comp = ec_complete_strvec(node, strvec);
	if (comp == NULL)
		goto fail;

	count = ec_comp_count(comp, EC_COMP_UNKNOWN | EC_COMP_FULL | EC_COMP_PARTIAL);

	EC_COMP_FOREACH (item, comp, EC_COMP_UNKNOWN | EC_COMP_FULL | EC_COMP_PARTIAL) {
		/* only one match, display it fully */
		if (count == 1) {
			printf("%s\n", ec_comp_item_get_str(item));
			break;
		}

		/* else show the 'display' part only */
		printf("%s\n", ec_comp_item_get_display(item));
	}

	ec_comp_free(comp);
	ec_strvec_free(strvec);
	return 0;

fail:
	ec_comp_free(comp);
	ec_strvec_free(strvec);
	return -1;
}

int main(int argc, char *argv[])
{
	struct ec_node *node = NULL;
	int ret;

	ret = parse_args(argc, argv);
	if (ret < 0)
		goto fail;

	argc -= ret;
	argv += ret;

	if (ec_init() < 0) {
		fprintf(stderr, "cannot init ecoli: %s\n", strerror(errno));
		return 1;
	}

	node = ec_yaml_import(input_file);
	if (node == NULL) {
		fprintf(stderr, "Failed to parse file\n");
		goto fail;
	}

	if (complete) {
		if (complete_words(node, argc, argv) < 0)
			goto fail;
	} else {
		if (interact(node) < 0)
			goto fail;
	}

	ec_node_free(node);

	return 0;

fail:
	ec_node_free(node);
	return 1;
}
