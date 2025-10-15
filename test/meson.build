# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 Robin Jarry

if not get_option('tests').allowed()
	subdir_done()
endif

libecoli_tests = files(
	'complete.c',
	'config.c',
	'dict.c',
	'htable.c',
	'log.c',
	'node.c',
	'node_any.c',
	'node_bypass.c',
	'node_cmd.c',
	'node_cond.c',
	'node_dynamic.c',
	'node_empty.c',
	'node_expr.c',
	'node_file.c',
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
	'malloc.c',
	'parse.c',
	'strvec.c',
	'vec.c',
)

fs = import('fs')
foreach t : libecoli_tests
	test(
		fs.stem(t) + '_test',
		executable(
			fs.stem(t),
			sources: [t] + files('test.c'),
			link_with: libecoli,
			include_directories: inc,
		),
		env: {
			'ASAN_OPTIONS': 'handle_abort=0:halt_on_error=1:handle_segv=2',
			'CMOCKA_TEST_ABORT': '1',
		},
		suite: 'unit',
	)
endforeach
