/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <ecoli_malloc.h>
#include <ecoli_string.h>
#include <ecoli_strvec.h>
#include <ecoli_keyval.h>
#include <ecoli_log.h>
#include <ecoli_test.h>
#include <ecoli_node.h>
#include <ecoli_parse.h>
#include <ecoli_node_sh_lex.h>
#include <ecoli_node_str.h>
#include <ecoli_node_or.h>
#include <ecoli_complete.h>

EC_LOG_TYPE_REGISTER(comp);

struct ec_comp_item {
	TAILQ_ENTRY(ec_comp_item) next;
	enum ec_comp_type type;
	struct ec_comp_group *grp;
	char *start;      /* the initial token */
	char *full;       /* the full token after completion */
	char *completion; /* chars that are added, NULL if not applicable */
	char *display;    /* what should be displayed by help/completers */
	struct ec_keyval *attrs;
};

struct ec_comp *ec_comp(struct ec_parse *state)
{
	struct ec_comp *comp = NULL;

	comp = ec_calloc(1, sizeof(*comp));
	if (comp == NULL)
		goto fail;

	comp->attrs = ec_keyval();
	if (comp->attrs == NULL)
		goto fail;

	TAILQ_INIT(&comp->groups);

	comp->cur_state = state;

	return comp;

 fail:
	if (comp != NULL)
		ec_keyval_free(comp->attrs);
	ec_free(comp);

	return NULL;
}

struct ec_parse *ec_comp_get_state(struct ec_comp *comp)
{
	return comp->cur_state;
}

int
ec_node_complete_child(const struct ec_node *node,
		struct ec_comp *comp,
		const struct ec_strvec *strvec)
{
	struct ec_parse *child_state, *cur_state;
	struct ec_comp_group *cur_group;
	int ret;

	if (ec_node_type(node)->complete == NULL) {
		errno = ENOTSUP;
		return -1;
	}

	/* save previous parse state, prepare child state */
	cur_state = comp->cur_state;
	child_state = ec_parse(node);
	if (child_state == NULL)
		return -1;

	if (cur_state != NULL)
		ec_parse_link_child(cur_state, child_state);
	comp->cur_state = child_state;
	cur_group = comp->cur_group;
	comp->cur_group = NULL;

	/* fill the comp struct with items */
	ret = ec_node_type(node)->complete(node, comp, strvec);

	/* restore parent parse state */
	if (cur_state != NULL) {
		ec_parse_unlink_child(cur_state, child_state);
		assert(!ec_parse_has_child(child_state));
	}
	ec_parse_free(child_state);
	comp->cur_state = cur_state;
	comp->cur_group = cur_group;

	if (ret < 0)
		return -1;

	return 0;
}

struct ec_comp *ec_node_complete_strvec(const struct ec_node *node,
	const struct ec_strvec *strvec)
{
	struct ec_comp *comp = NULL;
	int ret;

	comp = ec_comp(NULL);
	if (comp == NULL)
		goto fail;

	ret = ec_node_complete_child(node, comp, strvec);
	if (ret < 0)
		goto fail;

	return comp;

fail:
	ec_comp_free(comp);
	return NULL;
}

struct ec_comp *ec_node_complete(const struct ec_node *node,
	const char *str)
{
	struct ec_strvec *strvec = NULL;
	struct ec_comp *comp;

	errno = ENOMEM;
	strvec = ec_strvec();
	if (strvec == NULL)
		goto fail;

	if (ec_strvec_add(strvec, str) < 0)
		goto fail;

	comp = ec_node_complete_strvec(node, strvec);
	if (comp == NULL)
		goto fail;

	ec_strvec_free(strvec);
	return comp;

 fail:
	ec_strvec_free(strvec);
	return NULL;
}

static struct ec_comp_group *
ec_comp_group(const struct ec_node *node, struct ec_parse *parse)
{
	struct ec_comp_group *grp = NULL;

	grp = ec_calloc(1, sizeof(*grp));
	if (grp == NULL)
		return NULL;

	grp->attrs = ec_keyval();
	if (grp->attrs == NULL)
		goto fail;

	grp->state = ec_parse_dup(parse);
	if (grp->state == NULL)
		goto fail;

	grp->node = node;
	TAILQ_INIT(&grp->items);

	return grp;

fail:
	if (grp != NULL) {
		ec_parse_free(grp->state);
		ec_keyval_free(grp->attrs);
	}
	ec_free(grp);
	return NULL;
}

static struct ec_comp_item *
ec_comp_item(enum ec_comp_type type,
	const char *start, const char *full)
{
	struct ec_comp_item *item = NULL;
	struct ec_keyval *attrs = NULL;
	char *comp_cp = NULL, *start_cp = NULL;
	char *full_cp = NULL, *display_cp = NULL;

	if (type == EC_COMP_UNKNOWN && full != NULL) {
		errno = EINVAL;
		return NULL;
	}
	if (type != EC_COMP_UNKNOWN && full == NULL) {
		errno = EINVAL;
		return NULL;
	}

	item = ec_calloc(1, sizeof(*item));
	if (item == NULL)
		goto fail;

	attrs = ec_keyval();
	if (attrs == NULL)
		goto fail;

	if (start != NULL) {
		start_cp = ec_strdup(start);
		if (start_cp == NULL)
			goto fail;

		if (ec_str_startswith(full, start)) {
			comp_cp = ec_strdup(&full[strlen(start)]);
			if (comp_cp == NULL)
				goto fail;
		}
	}
	if (full != NULL) {
		full_cp = ec_strdup(full);
		if (full_cp == NULL)
			goto fail;
		display_cp = ec_strdup(full);
		if (display_cp == NULL)
			goto fail;
	}

	item->type = type;
	item->start = start_cp;
	item->full = full_cp;
	item->completion = comp_cp;
	item->display = display_cp;
	item->attrs = attrs;

	return item;

fail:
	ec_keyval_free(attrs);
	ec_free(comp_cp);
	ec_free(start_cp);
	ec_free(full_cp);
	ec_free(display_cp);
	ec_free(item);

	return NULL;
}

int ec_comp_item_set_display(struct ec_comp_item *item,
				const char *display)
{
	char *display_copy = NULL;

	if (item == NULL || display == NULL ||
			item->type == EC_COMP_UNKNOWN) {
		errno = EINVAL;
		return -1;
	}

	display_copy = ec_strdup(display);
	if (display_copy == NULL)
		goto fail;

	ec_free(item->display);
	item->display = display_copy;

	return 0;

fail:
	ec_free(display_copy);
	return -1;
}

int
ec_comp_item_set_completion(struct ec_comp_item *item,
				const char *completion)
{
	char *completion_copy = NULL;

	if (item == NULL || completion == NULL ||
			item->type == EC_COMP_UNKNOWN) {
		errno = EINVAL;
		return -1;
	}

	completion_copy = ec_strdup(completion);
	if (completion_copy == NULL)
		goto fail;

	ec_free(item->completion);
	item->completion = completion_copy;

	return 0;

fail:
	ec_free(completion_copy);
	return -1;
}

int
ec_comp_item_set_str(struct ec_comp_item *item,
			const char *str)
{
	char *str_copy = NULL;

	if (item == NULL || str == NULL ||
			item->type == EC_COMP_UNKNOWN) {
		errno = EINVAL;
		return -1;
	}

	str_copy = ec_strdup(str);
	if (str_copy == NULL)
		goto fail;

	ec_free(item->full);
	item->full = str_copy;

	return 0;

fail:
	ec_free(str_copy);
	return -1;
}

static int
ec_comp_item_add(struct ec_comp *comp, const struct ec_node *node,
		struct ec_comp_item *item)
{
	if (comp == NULL || item == NULL) {
		errno = EINVAL;
		return -1;
	}

	switch (item->type) {
	case EC_COMP_UNKNOWN:
		comp->count_unknown++;
		break;
	case EC_COMP_FULL:
		comp->count_full++;
		break;
	case EC_COMP_PARTIAL:
		comp->count_partial++;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (comp->cur_group == NULL) {
		struct ec_comp_group *grp;

		grp = ec_comp_group(node, comp->cur_state);
		if (grp == NULL)
			return -1;
		TAILQ_INSERT_TAIL(&comp->groups, grp, next);
		comp->cur_group = grp;
	}

	comp->count++;
	TAILQ_INSERT_TAIL(&comp->cur_group->items, item, next);
	item->grp = comp->cur_group;

	return 0;
}

const char *
ec_comp_item_get_str(const struct ec_comp_item *item)
{
	return item->full;
}

const char *
ec_comp_item_get_display(const struct ec_comp_item *item)
{
	return item->display;
}

const char *
ec_comp_item_get_completion(const struct ec_comp_item *item)
{
	return item->completion;
}

enum ec_comp_type
ec_comp_item_get_type(const struct ec_comp_item *item)
{
	return item->type;
}

const struct ec_comp_group *
ec_comp_item_get_grp(const struct ec_comp_item *item)
{
	return item->grp;
}

const struct ec_node *
ec_comp_item_get_node(const struct ec_comp_item *item)
{
	return ec_comp_item_get_grp(item)->node;
}

static void
ec_comp_item_free(struct ec_comp_item *item)
{
	if (item == NULL)
		return;

	ec_free(item->full);
	ec_free(item->start);
	ec_free(item->completion);
	ec_free(item->display);
	ec_keyval_free(item->attrs);
	ec_free(item);
}

int ec_comp_add_item(struct ec_comp *comp,
			const struct ec_node *node,
			struct ec_comp_item **p_item,
			enum ec_comp_type type,
			const char *start, const char *full)
{
	struct ec_comp_item *item = NULL;
	int ret;

	item = ec_comp_item(type, start, full);
	if (item == NULL)
		return -1;

	ret = ec_comp_item_add(comp, node, item);
	if (ret < 0)
		goto fail;

	if (p_item != NULL)
		*p_item = item;

	return 0;

fail:
	ec_comp_item_free(item);

	return -1;
}

/* return a completion item of type "unknown" */
int
ec_node_complete_unknown(const struct ec_node *gen_node,
			struct ec_comp *comp,
			const struct ec_strvec *strvec)
{
	int ret;

	if (ec_strvec_len(strvec) != 1)
		return 0;

	ret = ec_comp_add_item(comp, gen_node, NULL,
				EC_COMP_UNKNOWN, NULL, NULL);
	if (ret < 0)
		return ret;

	return 0;
}

static void ec_comp_group_free(struct ec_comp_group *grp)
{
	struct ec_comp_item *item;

	if (grp == NULL)
		return;

	while (!TAILQ_EMPTY(&grp->items)) {
		item = TAILQ_FIRST(&grp->items);
		TAILQ_REMOVE(&grp->items, item, next);
		ec_comp_item_free(item);
	}
	ec_parse_free(ec_parse_get_root(grp->state));
	ec_keyval_free(grp->attrs);
	ec_free(grp);
}

void ec_comp_free(struct ec_comp *comp)
{
	struct ec_comp_group *grp;

	if (comp == NULL)
		return;

	while (!TAILQ_EMPTY(&comp->groups)) {
		grp = TAILQ_FIRST(&comp->groups);
		TAILQ_REMOVE(&comp->groups, grp, next);
		ec_comp_group_free(grp);
	}
	ec_keyval_free(comp->attrs);
	ec_free(comp);
}

void ec_comp_dump(FILE *out, const struct ec_comp *comp)
{
	struct ec_comp_group *grp;
	struct ec_comp_item *item;

	if (comp == NULL || comp->count == 0) {
		fprintf(out, "no completion\n");
		return;
	}

	fprintf(out, "completion: count=%u full=%u partial=%u unknown=%u\n",
		comp->count, comp->count_full,
		comp->count_partial,  comp->count_unknown);

	TAILQ_FOREACH(grp, &comp->groups, next) {
		fprintf(out, "node=%p, node_type=%s\n",
			grp->node, ec_node_type(grp->node)->name);
		TAILQ_FOREACH(item, &grp->items, next) {
			const char *typestr;

			switch (item->type) {
			case EC_COMP_UNKNOWN: typestr = "unknown"; break;
			case EC_COMP_FULL: typestr = "full"; break;
			case EC_COMP_PARTIAL: typestr = "partial"; break;
			default: typestr = "unknown"; break;
			}

			fprintf(out, "  type=%s str=<%s> comp=<%s> disp=<%s>\n",
				typestr, item->full, item->completion,
				item->display);
		}
	}
}

int ec_comp_merge(struct ec_comp *to,
		struct ec_comp *from)
{
	struct ec_comp_group *grp;

	while (!TAILQ_EMPTY(&from->groups)) {
		grp = TAILQ_FIRST(&from->groups);
		TAILQ_REMOVE(&from->groups, grp, next);
		TAILQ_INSERT_TAIL(&to->groups, grp, next);
	}
	to->count += from->count;
	to->count_full += from->count_full;
	to->count_partial += from->count_partial;
	to->count_unknown += from->count_unknown;

	ec_comp_free(from);
	return 0;
}

unsigned int ec_comp_count(
	const struct ec_comp *comp,
	enum ec_comp_type type)
{
	unsigned int count = 0;

	if (comp == NULL)
		return count;

	if (type & EC_COMP_FULL)
		count += comp->count_full;
	if (type & EC_COMP_PARTIAL)
		count += comp->count_partial;
	if (type & EC_COMP_UNKNOWN)
		count += comp->count_unknown;

	return count;
}

struct ec_comp_iter *
ec_comp_iter(const struct ec_comp *comp,
	enum ec_comp_type type)
{
	struct ec_comp_iter *iter;

	iter = ec_calloc(1, sizeof(*iter));
	if (iter == NULL)
		return NULL;

	iter->comp = comp;
	iter->type = type;
	iter->cur_node = NULL;
	iter->cur_match = NULL;

	return iter;
}

struct ec_comp_item *ec_comp_iter_next(
	struct ec_comp_iter *iter)
{
	const struct ec_comp *comp;
	struct ec_comp_group *cur_node;
	struct ec_comp_item *cur_match;

	if (iter == NULL)
		return NULL;
	comp = iter->comp;
	if (comp == NULL)
		return NULL;

	cur_node = iter->cur_node;
	cur_match = iter->cur_match;

	/* first call */
	if (cur_node == NULL) {
		TAILQ_FOREACH(cur_node, &comp->groups, next) {
			TAILQ_FOREACH(cur_match, &cur_node->items, next) {
				if (cur_match != NULL &&
						cur_match->type & iter->type)
					goto found;
			}
		}
		return NULL;
	} else {
		cur_match = TAILQ_NEXT(cur_match, next);
		if (cur_match != NULL &&
				cur_match->type & iter->type)
			goto found;
		cur_node = TAILQ_NEXT(cur_node, next);
		while (cur_node != NULL) {
			cur_match = TAILQ_FIRST(&cur_node->items);
			if (cur_match != NULL &&
					cur_match->type & iter->type)
				goto found;
			cur_node = TAILQ_NEXT(cur_node, next);
		}
		return NULL;
	}

found:
	iter->cur_node = cur_node;
	iter->cur_match = cur_match;

	return iter->cur_match;
}

void ec_comp_iter_free(struct ec_comp_iter *iter)
{
	ec_free(iter);
}

/* LCOV_EXCL_START */
static int ec_comp_testcase(void)
{
	struct ec_node *node = NULL;
	struct ec_comp *c = NULL;
	struct ec_comp_iter *iter = NULL;
	struct ec_comp_item *item;
	FILE *f = NULL;
	char *buf = NULL;
	size_t buflen = 0;
	int testres = 0;

	node = ec_node_sh_lex(EC_NO_ID,
			EC_NODE_OR(EC_NO_ID,
				ec_node_str("id_x", "xx"),
				ec_node_str("id_y", "yy")));
	if (node == NULL)
		goto fail;

	c = ec_node_complete(node, "xcdscds");
	testres |= EC_TEST_CHECK(
		c != NULL && ec_comp_count(c, EC_COMP_ALL) == 0,
		"complete count should is not 0\n");
	ec_comp_free(c);

	c = ec_node_complete(node, "x");
	testres |= EC_TEST_CHECK(
		c != NULL && ec_comp_count(c, EC_COMP_ALL) == 1,
		"complete count should is not 1\n");
	ec_comp_free(c);

	c = ec_node_complete(node, "");
	testres |= EC_TEST_CHECK(
		c != NULL && ec_comp_count(c, EC_COMP_ALL) == 2,
		"complete count should is not 2\n");

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_comp_dump(f, NULL);
	fclose(f);
	f = NULL;

	testres |= EC_TEST_CHECK(
		strstr(buf, "no completion"), "bad dump\n");
	free(buf);
	buf = NULL;

	f = open_memstream(&buf, &buflen);
	if (f == NULL)
		goto fail;
	ec_comp_dump(f, c);
	fclose(f);
	f = NULL;

	testres |= EC_TEST_CHECK(
		strstr(buf, "comp=<xx>"), "bad dump\n");
	testres |= EC_TEST_CHECK(
		strstr(buf, "comp=<yy>"), "bad dump\n");
	free(buf);
	buf = NULL;

	iter = ec_comp_iter(c, EC_COMP_ALL);
	item = ec_comp_iter_next(iter);
	if (item == NULL)
		goto fail;

	testres |= EC_TEST_CHECK(
		!strcmp(ec_comp_item_get_display(item), "xx"),
		"bad item display\n");
	testres |= EC_TEST_CHECK(
		ec_comp_item_get_type(item) == EC_COMP_FULL,
		"bad item type\n");
	testres |= EC_TEST_CHECK(
		!strcmp(ec_node_id(ec_comp_item_get_node(item)), "id_x"),
		"bad item node\n");

	item = ec_comp_iter_next(iter);
	if (item == NULL)
		goto fail;

	testres |= EC_TEST_CHECK(
		!strcmp(ec_comp_item_get_display(item), "yy"),
		"bad item display\n");
	testres |= EC_TEST_CHECK(
		ec_comp_item_get_type(item) == EC_COMP_FULL,
		"bad item type\n");
	testres |= EC_TEST_CHECK(
		!strcmp(ec_node_id(ec_comp_item_get_node(item)), "id_y"),
		"bad item node\n");

	item = ec_comp_iter_next(iter);
	testres |= EC_TEST_CHECK(item == NULL, "should be the last item\n");

	ec_comp_iter_free(iter);
	ec_comp_free(c);
	ec_node_free(node);

	return testres;

fail:
	ec_comp_iter_free(iter);
	ec_comp_free(c);
	ec_node_free(node);
	if (f != NULL)
		fclose(f);
	free(buf);

	return -1;
}
/* LCOV_EXCL_STOP */

static struct ec_test ec_comp_test = {
	.name = "comp",
	.test = ec_comp_testcase,
};

EC_TEST_REGISTER(ec_comp_test);