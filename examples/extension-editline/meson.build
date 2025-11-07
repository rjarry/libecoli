# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025, Olivier MATZ <zer0@droids-corp.org>

extension_editline_sources = files(
	'main.c',
	'node_bool_tuple.c',
)

ecoli_extension_editline = executable(
	'ecoli-extension-editline',
	extension_editline_sources,
	include_directories : inc,
	link_with : libecoli)
