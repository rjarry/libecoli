# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019, Olivier MATZ <zer0@droids-corp.org>

doxygen = find_program('doxygen', required: get_option('doc'))
doxygen2man = find_program('doxygen2man', required: get_option('doc'))
xmllint = find_program('xmllint', required: get_option('doc'))
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

if doxygen2man.found() and xmllint.found()
	xml_cdata = configuration_data()
	xml_cdata.merge_from(cdata)
	xml_cdata.set('GENERATE_HTML', 'NO')
	xml_cdata.set('GENERATE_XML', 'YES')
	xml_cdata.set('EXAMPLES_INPUT', '')
	doxygen_xml_conf = configure_file(
		input: 'Doxyfile.in',
		output: 'Doxyfile.xml',
		configuration: xml_cdata,
		install: false)
	doxygen_xml = custom_target(
		'doxygen-xml',
		input: doxygen_xml_conf,
		output: 'xml',
		command: [doxygen, '@INPUT@'],
		build_by_default: true,
		depends: node_schemas,
		depend_files: doc_sources,
		install: false)

	custom_target(
		'man-pages',
		output: 'man-pages.stamp',
		command: [
			files('gen-man-pages.sh'),
			meson.current_build_dir() / 'xml',
			meson.project_source_root() / 'include',
			meson.current_build_dir() / 'man',
			'@OUTPUT@',
		],
		depends: doxygen_xml,
		depend_files: doc_sources,
		build_by_default: true,
		install: false)

	meson.add_install_script(
		files('install-man-pages.sh'),
		meson.current_build_dir() / 'man',
		get_option('mandir'))
endif

html_cdata = configuration_data()
html_cdata.merge_from(cdata)
html_cdata.set('GENERATE_HTML', 'YES')
html_cdata.set('GENERATE_XML', 'NO')
html_cdata.set('EXAMPLES_INPUT', meson.project_source_root() / 'examples')
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
