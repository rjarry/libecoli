/*
 * Copyright (c) 2016, Olivier MATZ <zer0@droids-corp.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ECOLI_PARSED_
#define ECOLI_PARSED_

#include <sys/queue.h>
#include <sys/types.h>
#include <stdio.h>

struct ec_node;

TAILQ_HEAD(ec_parsed_list, ec_parsed);

/*
  node == NULL + empty children list means "no match"
*/
struct ec_parsed {
	TAILQ_ENTRY(ec_parsed) next;
	struct ec_parsed_list children;
	const struct ec_node *node;
	struct ec_strvec *strvec;
};

struct ec_parsed *ec_parsed(void);
void ec_parsed_free(struct ec_parsed *parsed);
void ec_parsed_free_children(struct ec_parsed *parsed);

const struct ec_strvec *ec_parsed_strvec(
	const struct ec_parsed *parsed);

void ec_parsed_set_match(struct ec_parsed *parsed,
	const struct ec_node *node, struct ec_strvec *strvec);

/* XXX we could use a cache to store possible completions or match: the
 * cache would be per-node, and would be reset for each call to parse()
 * or complete() ? */
/* a NULL return value is an error, with errno set
  ENOTSUP: no ->parse() operation
*/
struct ec_parsed *ec_node_parse(struct ec_node *node, const char *str);

/* mostly internal to nodes */
/* XXX it should not reset cache
 * ... not sure... it is used by tests */
struct ec_parsed *ec_node_parse_strvec(struct ec_node *node,
	const struct ec_strvec *strvec);

void ec_parsed_add_child(struct ec_parsed *parsed,
	struct ec_parsed *child);
void ec_parsed_del_child(struct ec_parsed *parsed,
	struct ec_parsed *child);
void ec_parsed_dump(FILE *out, const struct ec_parsed *parsed);

struct ec_parsed *ec_parsed_find_first(struct ec_parsed *parsed,
	const char *id);

const char *ec_parsed_to_string(const struct ec_parsed *parsed);
size_t ec_parsed_len(const struct ec_parsed *parsed);
size_t ec_parsed_matches(const struct ec_parsed *parsed);

#endif