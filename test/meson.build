# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Olivier MATZ <zer0@droids-corp.org>

if not get_option('tests').allowed()
  subdir_done()
endif

test_sources = files(
	'test.c',
)

ecoli_test = executable('ecoli-test',
	test_sources,
	include_directories : inc,
	link_with : libecoli)

test('libecoli test', ecoli_test)
