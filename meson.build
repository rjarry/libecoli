# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Olivier MATZ <zer0@droids-corp.org>

project('libecoli',
	'c',
	version : '0.8.0',
	license : 'BSD-3-clause',
	default_options: [
		'warning_level=2',
		'werror=true',
		'c_std=c99',
	])

edit_dep = dependency('libedit', required: get_option('editline'))
yaml_dep = dependency('yaml-0.1', required: get_option('yaml'))

add_project_arguments('-Wmissing-prototypes', language : 'c')
add_project_arguments('-D_GNU_SOURCE', language : 'c')

inc = include_directories('include')
libecoli_sources = []

subdir('src')
subdir('include')

if meson.is_subproject()
	libecoli = static_library('ecoli',
		libecoli_sources,
		include_directories : inc,
		dependencies : deps,
		install : false
	)
	libecoli_dep = declare_dependency(
		link_whole: libecoli,
		include_directories: inc,
	)
else
	libecoli = shared_library('ecoli',
		libecoli_sources,
		include_directories : inc,
		dependencies : deps,
		version : meson.project_version(),
		install : true
	)
	pkg_mod = import('pkgconfig')
	pkg_mod.generate(libecoli,
		name : 'libecoli',
		filebase : 'libecoli',
		description : 'Extensible COmmand LIne library.')
endif

subdir('test')
subdir('examples')
subdir('doc')
