# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Olivier MATZ <zer0@droids-corp.org>

if not get_option('examples').require(yaml_dep.found() and edit_dep.found(),
		error_message: 'examples require libyaml and libedit').allowed()
	subdir_done()
endif

parse_yaml_sources = files(
	'parse-yaml.c',
)
doc_sources += parse_yaml_sources

ecoli_parse_yaml = executable(
	'ecoli-parse-yaml',
	parse_yaml_sources,
	include_directories : inc,
	link_with : libecoli,
	dependencies: [yaml_dep, edit_dep])

install_data(
	'ecoli-parse-yaml.sh',
	install_dir : get_option('bindir'))
