# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Olivier MATZ <zer0@droids-corp.org>

libecoli_sources += files(
	'assert.c',
	'complete.c',
	'config.c',
	'dict.c',
	'htable.c',
	'init.c',
	'log.c',
	'murmurhash.c',
	'node.c',
	'node_any.c',
	'node_bypass.c',
	'node_cmd.c',
	'node_cond.c',
	'node_dynamic.c',
	'node_empty.c',
	'node_expr.c',
	'node_file.c',
	'node_helper.c',
	'node_int.c',
	'node_many.c',
	'node_none.c',
	'node_once.c',
	'node_option.c',
	'node_or.c',
	'node_re.c',
	'node_re_lex.c',
	'node_seq.c',
	'node_sh_lex.c',
	'node_space.c',
	'node_str.c',
	'node_subset.c',
	'parse.c',
	'string.c',
	'strvec.c',
	'vec.c',
)
deps = []
if yaml_dep.found()
	libecoli_sources += files(
		'yaml.c',
	)
	deps += [
		yaml_dep,
	]
endif
if edit_dep.found()
	libecoli_sources += files(
		'editline.c',
	)
	deps += [
		edit_dep,
	]
endif
