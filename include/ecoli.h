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

#include <ecoli/assert.h>
#include <ecoli/complete.h>
#include <ecoli/config.h>
#include <ecoli/dict.h>
#include <ecoli/editline.h>
#include <ecoli/htable.h>
#include <ecoli/init.h>
#include <ecoli/log.h>
#include <ecoli/murmurhash.h>
#include <ecoli/node.h>
#include <ecoli/node_any.h>
#include <ecoli/node_bypass.h>
#include <ecoli/node_cmd.h>
#include <ecoli/node_cond.h>
#include <ecoli/node_dynamic.h>
#include <ecoli/node_empty.h>
#include <ecoli/node_expr.h>
#include <ecoli/node_file.h>
#include <ecoli/node_helper.h>
#include <ecoli/node_int.h>
#include <ecoli/node_many.h>
#include <ecoli/node_none.h>
#include <ecoli/node_once.h>
#include <ecoli/node_option.h>
#include <ecoli/node_or.h>
#include <ecoli/node_re.h>
#include <ecoli/node_re_lex.h>
#include <ecoli/node_seq.h>
#include <ecoli/node_sh_lex.h>
#include <ecoli/node_space.h>
#include <ecoli/node_str.h>
#include <ecoli/node_subset.h>
#include <ecoli/parse.h>
#include <ecoli/string.h>
#include <ecoli/strvec.h>
#include <ecoli/utils.h>
#include <ecoli/vec.h>
#include <ecoli/yaml.h>
