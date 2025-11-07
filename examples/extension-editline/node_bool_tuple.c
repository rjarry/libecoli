/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016, Olivier MATZ <zer0@droids-corp.org>
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ecoli.h>

EC_LOG_TYPE_REGISTER(node_bool_tuple);

enum bool_tuple_state {
	BT_EMPTY,
	BT_OPEN,
	BT_BOOL,
	BT_COMMA,
	BT_END,
	BT_FAIL,
};

/* Check if input matches a bool tuple like this: "(true,false,true)". On success, *state is set to
 * BT_END. If parsing is incomplete or fails, set the *state to something else, and return the
 * pointer to the incomplete or failing token.
 */
static const char *parse_bool_tuple(const char *input, enum bool_tuple_state *state)
{
	*state = BT_EMPTY;
	if (*input == '\0')
		return input;

	if (*input != '(') {
		*state = BT_FAIL;
		return input;
	}
	input++;
	*state = BT_OPEN;

	if (*input == ')') {
		*state = BT_END;
		input++;
		if (*input != '\0') {
			*state = BT_FAIL;
			return input;
		}

		return input;
	}

	while (1) {
		if (ec_str_startswith(input, "true")) {
			input += 4;
		} else if (ec_str_startswith(input, "false")) {
			input += 5;
		} else {
			if (!ec_str_startswith("true", input) && !ec_str_startswith("false", input))
				*state = BT_FAIL;
			return input;
		}

		*state = BT_BOOL;
		if (*input == '\0')
			return input;

		if (*input == ')') {
			*state = BT_END;
			input++;
			if (*input != '\0') {
				*state = BT_FAIL;
				return input;
			}
			return input;
		}

		if (*input != ',') {
			*state = BT_FAIL;
			return input;
		}

		input++;
		*state = BT_COMMA;
	}
}

static int ec_node_bool_tuple_parse(
	const struct ec_node *node,
	struct ec_pnode *pstate,
	const struct ec_strvec *strvec
)
{
	enum bool_tuple_state state;
	const char *input;

	(void)node;
	(void)pstate;

	if (ec_strvec_len(strvec) == 0)
		return EC_PARSE_NOMATCH;

	input = ec_strvec_val(strvec, 0);
	parse_bool_tuple(input, &state);

	if (state != BT_END)
		return EC_PARSE_NOMATCH;

	return 1;
}

static int ec_node_bool_tuple_complete(
	const struct ec_node *node,
	struct ec_comp *comp,
	const struct ec_strvec *strvec
)
{
	struct ec_comp_item *item = NULL;
	const char *false_str = "false";
	const char *true_str = "true";
	enum bool_tuple_state state;
	char *comp_str = NULL;
	char *disp_str = NULL;
	const char *incomplete;
	const char *input;
	int ret;

	if (ec_strvec_len(strvec) != 1)
		return 0;

	input = ec_strvec_val(strvec, 0);
	incomplete = parse_bool_tuple(input, &state);

	switch (state) {
	case BT_EMPTY:
		if (asprintf(&comp_str, "%s(", input) < 0)
			goto fail;
		if (asprintf(&disp_str, "(") < 0)
			goto fail;
		item = ec_comp_add_item(comp, node, EC_COMP_PARTIAL, input, comp_str);
		if (item == NULL)
			goto fail;
		if (ec_comp_item_set_display(item, disp_str) < 0)
			goto fail;
		break;
	case BT_OPEN:
		if (incomplete[0] == '\0') {
			if (asprintf(&comp_str, "%s)", input) < 0)
				goto fail;
			if (asprintf(&disp_str, ")") < 0)
				goto fail;
			item = ec_comp_add_item(comp, node, EC_COMP_FULL, input, comp_str);
			if (item == NULL)
				goto fail;
			if (ec_comp_item_set_display(item, disp_str) < 0)
				goto fail;
			free(comp_str);
			comp_str = NULL;
			free(disp_str);
			disp_str = NULL;
		}
		/* fallthrough */
	case BT_COMMA:
		if (incomplete[0] == 't' || incomplete[0] == '\0') {
			if (asprintf(&comp_str, "%s%s", input, &true_str[strlen(incomplete)]) < 0)
				goto fail;
			if (asprintf(&disp_str, "true") < 0)
				goto fail;
			item = ec_comp_add_item(comp, node, EC_COMP_PARTIAL, input, comp_str);
			if (item == NULL)
				goto fail;
			if (ec_comp_item_set_display(item, disp_str) < 0)
				goto fail;
			free(comp_str);
			comp_str = NULL;
			free(disp_str);
			disp_str = NULL;
		}
		if (incomplete[0] == 'f' || incomplete[0] == '\0') {
			if (asprintf(&comp_str, "%s%s", input, &false_str[strlen(incomplete)]) < 0)
				goto fail;
			if (asprintf(&disp_str, "false") < 0)
				goto fail;
			item = ec_comp_add_item(comp, node, EC_COMP_PARTIAL, input, comp_str);
			if (item == NULL)
				goto fail;
			if (ec_comp_item_set_display(item, disp_str) < 0)
				goto fail;
			free(comp_str);
			comp_str = NULL;
			free(disp_str);
			disp_str = NULL;
		}
		break;
	case BT_BOOL:
		if (asprintf(&comp_str, "%s,", input) < 0)
			goto fail;
		if (asprintf(&disp_str, ",") < 0)
			goto fail;
		item = ec_comp_add_item(comp, node, EC_COMP_PARTIAL, input, comp_str);
		if (item == NULL)
			goto fail;
		if (ec_comp_item_set_display(item, disp_str) < 0)
			goto fail;
		if (asprintf(&comp_str, "%s)", input) < 0)
			goto fail;
		if (asprintf(&disp_str, ")") < 0)
			goto fail;
		item = ec_comp_add_item(comp, node, EC_COMP_PARTIAL, input, comp_str);
		if (item == NULL)
			goto fail;
		if (ec_comp_item_set_display(item, disp_str) < 0)
			goto fail;
		break;
	case BT_END:
		item = ec_comp_add_item(comp, node, EC_COMP_FULL, input, input);
		if (item == NULL)
			goto fail;
		break;
	default:
		break;
	}

	ret = 0;
out:
	free(comp_str);
	free(disp_str);

	return ret;

fail:
	ret = -1;
	goto out;
}

static struct ec_node_type ec_node_bool_tuple_type = {
	.name = "bool_tuple",
	.parse = ec_node_bool_tuple_parse,
	.complete = ec_node_bool_tuple_complete,
};

EC_NODE_TYPE_REGISTER(ec_node_bool_tuple_type);
