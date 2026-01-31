# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Olivier MATZ <zer0@droids-corp.org>

readline_dep = dependency('readline', required: get_option('examples'))
if not readline_dep.found()
	subdir_done()
endif

readline_sources = files(
	'main.c',
)
doc_sources += readline_sources

ecoli_readline = executable(
	'ecoli-readline',
	readline_sources,
	include_directories : inc,
	link_with : libecoli,
	dependencies: readline_dep)
