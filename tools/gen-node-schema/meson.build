# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025, Olivier MATZ <zer0@droids-corp.org>

gen_node_schema_sources = files(
	'main.c',
)

ecoli_gen_node_schema = executable(
	'ecoli-gen-node-schema',
	gen_node_schema_sources,
	include_directories : inc,
	link_with : libecoli,
	dependencies: [edit_dep])

node_schema = custom_target(
	'node-schema.md',
	output: 'node-schema.md',
	capture: true,
	command: ecoli_gen_node_schema,
)

test(
	'node_schema_test',
	find_program('diff'),
	args: ['-u', files('../../doc/node-schema.md'), node_schema],
	suite: 'unit',
)
