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

/**
 * API for generating completions item on a node.
 *
 * This file provide helpers to list and manipulate the possible
 * completions for a given input.
 *
 * XXX completed vs item
 */

#ifndef ECOLI_COMPLETED_
#define ECOLI_COMPLETED_

#include <sys/queue.h>
#include <sys/types.h>
#include <stdio.h>

struct ec_node;

enum ec_completed_type {
	EC_COMP_UNKNOWN = 0x1,
	EC_COMP_FULL = 0x2,
	EC_COMP_PARTIAL = 0x4,
	EC_COMP_ALL = 0x7,
};

struct ec_completed_item;

TAILQ_HEAD(ec_completed_item_list, ec_completed_item);

struct ec_completed_group {
	TAILQ_ENTRY(ec_completed_group) next;
	const struct ec_node *node;
	struct ec_completed_item_list items;
	struct ec_parsed *state;
	struct ec_keyval *attrs;
};

TAILQ_HEAD(ec_completed_group_list, ec_completed_group);

struct ec_completed {
	unsigned count;
	unsigned count_match;
	struct ec_parsed *cur_state;
	struct ec_completed_group *cur_group;
	struct ec_completed_group_list groups;
	struct ec_keyval *attrs;
};

/*
 * return a completed object filled with items
 * return NULL on error (nomem?)
 */
struct ec_completed *ec_node_complete(struct ec_node *node,
	const char *str);
struct ec_completed *ec_node_complete_strvec(struct ec_node *node,
	const struct ec_strvec *strvec);

/* internal: used by nodes */
int ec_node_complete_child(struct ec_node *node,
			struct ec_completed *completed,
			const struct ec_strvec *strvec);

/**
 * Create a completion object (list of completion items).
 *
 *
 */
struct ec_completed *ec_completed(struct ec_parsed *state);

/**
 * Free a completion object and all its items.
 *
 *
 */
void ec_completed_free(struct ec_completed *completed);

/**
 *
 *
 *
 */
void ec_completed_dump(FILE *out,
	const struct ec_completed *completed);

/**
 * Merge items contained in 'from' into 'to'
 *
 * The 'from' completed struct is freed.
 */
int ec_completed_merge(struct ec_completed *to,
		struct ec_completed *from);

struct ec_parsed *ec_completed_get_state(struct ec_completed *completed);

/* shortcut for ec_completed_item() + ec_completed_item_add() */
int ec_completed_add_item(struct ec_completed *completed,
			const struct ec_node *node,
			struct ec_completed_item **p_item,
			enum ec_completed_type type,
			const char *start, const char *full);

/**
 *
 */
int ec_completed_item_set_str(struct ec_completed_item *item,
			const char *str);

/**
 * Get the string value of a completion item.
 *
 *
 */
const char *
ec_completed_item_get_str(const struct ec_completed_item *item);

/**
 * Get the display string value of a completion item.
 *
 *
 */
const char *
ec_completed_item_get_display(const struct ec_completed_item *item);

/**
 * Get the completion string value of a completion item.
 *
 *
 */
const char *
ec_completed_item_get_completion(const struct ec_completed_item *item);

/**
 * Get the group of a completion item.
 *
 *
 */
const struct ec_completed_group *
ec_completed_item_get_grp(const struct ec_completed_item *item);

/**
 * Get the type of a completion item.
 *
 *
 */
enum ec_completed_type
ec_completed_item_get_type(const struct ec_completed_item *item);

/**
 * Get the node associated to a completion item.
 *
 *
 */
const struct ec_node *
ec_completed_item_get_node(const struct ec_completed_item *item);

/**
 * Set the display value of an item.
 *
 *
 */
int ec_completed_item_set_display(struct ec_completed_item *item,
				const char *display);

/**
 * Set the completion value of an item.
 *
 *
 */
int ec_completed_item_set_completion(struct ec_completed_item *item,
				const char *completion);

/**
 *
 *
 *
 */
int
ec_node_default_complete(const struct ec_node *gen_node,
			struct ec_completed *completed,
			const struct ec_strvec *strvec);

/**
 *
 *
 *
 */
unsigned int ec_completed_count(
	const struct ec_completed *completed,
	enum ec_completed_type flags);

/**
 *
 *
 *
 */
struct ec_completed_iter {
	enum ec_completed_type type;
	struct ec_completed *completed;
	struct ec_completed_group *cur_node;
	struct ec_completed_item *cur_match;
};

/**
 *
 *
 *
 */
struct ec_completed_iter *
ec_completed_iter(struct ec_completed *completed,
	enum ec_completed_type type);

/**
 *
 *
 *
 */
struct ec_completed_item *ec_completed_iter_next(
	struct ec_completed_iter *iter);

/**
 *
 *
 *
 */
void ec_completed_iter_free(struct ec_completed_iter *iter);


#endif
