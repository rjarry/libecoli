# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025, Olivier MATZ <zer0@droids-corp.org>

if not get_option('examples').require(edit_dep.found(),
		error_message: 'examples require libedit').allowed()
	subdir_done()
endif

pool_editline_sources = files(
	'ip_pool.c',
	'main.c',
)
doc_sources += pool_editline_sources + files('ip_pool.h')

ecoli_pool_editline = executable(
	'ecoli-pool-editline',
	pool_editline_sources,
	include_directories : inc,
	link_with : libecoli,
	dependencies: [edit_dep])
