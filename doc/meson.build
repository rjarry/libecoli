# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019, Olivier MATZ <zer0@droids-corp.org>

doxygen = find_program('doxygen', required: get_option('doc'))
if not doxygen.found()
  subdir_done()
endif

cdata = configuration_data()
cdata.set('VERSION', meson.project_version())
cdata.set('OUTPUT', meson.current_build_dir())
cdata.set('TOPDIR', meson.project_source_root())

ecoli_gen_node_schema = executable(
	'gen-node-schema',
	files('gen-node-schema.c'),
	include_directories : inc,
	link_with: libecoli,
)

node_schemas = custom_target(
	'node-schemas',
	output: 'node-schemas',
	command: [ecoli_gen_node_schema, meson.current_build_dir(), '@OUTPUT@'],
)

man_cdata = configuration_data()
man_cdata.merge_from(cdata)
man_cdata.set('GENERATE_HTML', 'NO')
man_cdata.set('GENERATE_MAN', 'YES')
doxygen_man_conf = configure_file(
	input: 'Doxyfile.in',
	output: 'Doxyfile.man',
	configuration: man_cdata,
	install: false)
custom_target(
	'doxygen-man',
	input: doxygen_man_conf,
	output: 'man',
	command: [doxygen, '@INPUT@'],
	build_by_default: true,
	depends: node_schemas,
	depend_files: doc_sources,
	install: true,
	install_dir: get_option('datadir'))

html_cdata = configuration_data()
html_cdata.merge_from(cdata)
html_cdata.set('GENERATE_HTML', 'YES')
html_cdata.set('GENERATE_MAN', 'NO')
doxygen_html_conf = configure_file(
	input: 'Doxyfile.in',
	output: 'Doxyfile.html',
	configuration: html_cdata,
	install: false)
custom_target(
	'doxygen-html',
	input: doxygen_html_conf,
	output: 'html',
	command: [doxygen, '@INPUT@'],
	build_by_default: true,
	depends: node_schemas,
	depend_files: doc_sources,
	install: true,
	install_dir: join_paths(get_option('datadir'), 'doc', 'libecoli'))
