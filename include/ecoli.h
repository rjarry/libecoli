/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @anchor main_page
 * @mainpage About Libecoli
 *
 * This is the C API documentation of libecoli. This library provides
 * helpers to build interactive command line interfaces.
 *
 * To create a command line parser, one should create a grammar graph
 * which is composed of @ref ecoli_nodes. Then an input can be
 * parsed or completed, respectively using the @ref ecoli_parse and @ref
 * ecoli_complete APIs.
 *
 * The library also provides helpers to create a an interactive command
 * line based on @ref ecoli_editline library, and a @ref ecoli_yaml parser for
 * grammar graphs.
 */

#pragma once

#include <ecoli_assert.h>
#include <ecoli_complete.h>
#include <ecoli_config.h>
#include <ecoli_dict.h>
#include <ecoli_editline.h>
#include <ecoli_htable.h>
#include <ecoli_init.h>
#include <ecoli_log.h>
#include <ecoli_malloc.h>
#include <ecoli_murmurhash.h>
#include <ecoli_node_any.h>
#include <ecoli_node_bypass.h>
#include <ecoli_node_cmd.h>
#include <ecoli_node_cond.h>
#include <ecoli_node_dynamic.h>
#include <ecoli_node_empty.h>
#include <ecoli_node_expr.h>
#include <ecoli_node_file.h>
#include <ecoli_node.h>
#include <ecoli_node_helper.h>
#include <ecoli_node_int.h>
#include <ecoli_node_many.h>
#include <ecoli_node_none.h>
#include <ecoli_node_once.h>
#include <ecoli_node_option.h>
#include <ecoli_node_or.h>
#include <ecoli_node_re.h>
#include <ecoli_node_re_lex.h>
#include <ecoli_node_seq.h>
#include <ecoli_node_sh_lex.h>
#include <ecoli_node_space.h>
#include <ecoli_node_str.h>
#include <ecoli_node_subset.h>
#include <ecoli_parse.h>
#include <ecoli_string.h>
#include <ecoli_strvec.h>
#include <ecoli_utils.h>
#include <ecoli_vec.h>
#include <ecoli_yaml.h>
