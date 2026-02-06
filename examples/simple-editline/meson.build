# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025, Olivier MATZ <zer0@droids-corp.org>

if not get_option('examples').require(edit_dep.found(),
		error_message: 'examples require libedit').allowed()
	subdir_done()
endif

simple_editline_sources = files(
	'main.c',
)
doc_sources += simple_editline_sources

ecoli_simple_editline = executable(
	'ecoli-simple-editline',
	simple_editline_sources,
	include_directories : inc,
	link_with : libecoli,
	dependencies: [edit_dep])
