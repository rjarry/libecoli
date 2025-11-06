# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2025, Olivier MATZ <zer0@droids-corp.org>

pool_editline_sources = files(
	'ip_pool.c',
	'main.c',
)

ecoli_pool_editline = executable(
	'ecoli-pool-editline',
	pool_editline_sources,
	include_directories : inc,
	link_with : libecoli)
