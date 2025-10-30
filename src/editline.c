/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <histedit.h>

#include <ecoli/complete.h>
#include <ecoli/dict.h>
#include <ecoli/editline.h>
#include <ecoli/node.h>
#include <ecoli/parse.h>
#include <ecoli/string.h>
#include <ecoli/utils.h>

struct ec_editline {
	EditLine *el;
	History *history;
	char *hist_file;
	HistEvent histev;
	const struct ec_node *node;
	char *prompt;
};

/* used by qsort below */
static int
strcasecmp_cb(const void *p1, const void *p2)
{
	return strcasecmp(*(char * const *)p1, *(char * const *)p2);
}

/* Show the matches as a multi-columns list */
int
ec_editline_print_cols(struct ec_editline *editline,
		char const * const *matches, size_t n)
{
	size_t max_strlen = 0, len, i, j, ncols;
	int width, height;
	const char *space;
	char **matches_copy = NULL;
	FILE *f;

	if (el_get(editline->el, EL_GETFP, 1, &f))
		return -1;

	fprintf(f, "\n");
	if (n == 0)
		return 0;

	if (el_get(editline->el, EL_GETTC, "li", &height, (void *)0))
		return -1;
	if (el_get(editline->el, EL_GETTC, "co", &width, (void *)0))
		return -1;

	/* duplicate the matches table, and sort it */
	matches_copy = calloc(n, sizeof(const char *));
	if (matches_copy == NULL)
		return -1;
	memcpy(matches_copy, matches, sizeof(const char *) * n);
	qsort(matches_copy, n, sizeof(char *), strcasecmp_cb);

	/* get max string length */
	for (i = 0; i < n; i++) {
		len = strlen(matches_copy[i]);
		if (len > max_strlen)
			max_strlen = len;
	}

	/* write the columns */
	ncols = width / (max_strlen + 4);
	if (ncols == 0)
		ncols = 1;
	for (i = 0; i < n; i+= ncols) {
		for (j = 0; j < ncols; j++) {
			if (i + j >= n)
				break;
			if (j == 0)
				space = "";
			else
				space = "    ";
			fprintf(f, "%s%-*s", space,
				(int)max_strlen, matches[i+j]);
		}
		fprintf(f, "\n");
	}

	free(matches_copy);
	return 0;
}

/* Show the helps on editline output */
int
ec_editline_print_helps(const struct ec_editline *editline,
			const struct ec_editline_help *helps, size_t len)
{
	size_t i;
	FILE *out;

	if (el_get(editline->el, EL_GETFP, 1, &out))
		return -1;

	for (i = 0; i < len; i++) {
		if (fprintf(out, "%-20s %s\n",
				helps[i].desc, helps[i].help) < 0)
			return -1;
	}

	return 0;
}

void
ec_editline_free_helps(struct ec_editline_help *helps, size_t n)
{
	size_t i;

	if (helps == NULL)
		return;
	for (i = 0; i < n; i++) {
		free(helps[i].desc);
		free(helps[i].help);
	}
	free(helps);
}

static char *
prompt_cb(EditLine *el)
{
	struct ec_editline *editline;
	void *clientdata;

	if (el_get(el, EL_CLIENTDATA, &clientdata))
		return "> ";
	editline = clientdata;

	if (editline == NULL)
		return "> ";

	return editline->prompt;
}

int
ec_editline_set_prompt(struct ec_editline *editline, const char *prompt)
{
	char *copy = NULL;

	if (prompt != NULL) {
		copy = strdup(prompt);
		if (copy == NULL)
			return -1;
	}

	if (el_set(editline->el, EL_PROMPT, prompt_cb))
		goto fail;

	free(editline->prompt);
	editline->prompt = copy;

	return 0;

fail:
	free(copy);
	return -1;
}

int
ec_editline_set_prompt_esc(struct ec_editline *editline, const char *prompt,
			   char delim)
{
	char *copy = NULL;

	if (prompt != NULL) {
		copy = strdup(prompt);
		if (copy == NULL)
			return -1;
	}

	if (el_set(editline->el, EL_PROMPT_ESC, prompt_cb, delim) < 0)
		goto fail;

	free(editline->prompt);
	editline->prompt = copy;

	return 0;

fail:
	free(copy);
	return -1;
}

struct ec_editline *
ec_editline(const char *prog, FILE *f_in, FILE *f_out, FILE *f_err,
	    enum ec_editline_init_flags flags)
{
	struct ec_editline *editline = NULL;
	EditLine *el;

	if (f_in == NULL || f_out == NULL || f_err == NULL) {
		errno = EINVAL;
		goto fail;
	}

	editline = calloc(1, sizeof(*editline));
	if (editline == NULL)
		goto fail;

	el = el_init(prog, f_in, f_out, f_err);
	if (el == NULL)
		goto fail;
	editline->el = el;

	/* save editline pointer as user data */
	if (el_set(el, EL_CLIENTDATA, editline))
		goto fail;

	/* install default editline signals */
	if (el_set(el, EL_SIGNAL, 1))
		goto fail;

	if (el_set(el, EL_PREP_TERM, 0))
		goto fail;

	/* use emacs bindings */
	if (el_set(el, EL_EDITOR, "emacs"))
		goto fail;
	if (el_set(el, EL_BIND, "^W", "ed-delete-prev-word", NULL))
		goto fail;

	/* ask terminal to not send signals */
	if (flags & EC_EDITLINE_DISABLE_SIGNALS) {
		if (el_set(el, EL_SETTY, "-d", "-isig", NULL))
			goto fail;
	} else if (flags & EC_EDITLINE_DEFAULT_SIGHANDLER) {
		if (el_set(el, EL_SIGNAL, 1))
			goto fail;
	}

	/* set prompt */
	editline->prompt = strdup("> ");
	if (editline->prompt == NULL)
		goto fail;
	if (el_set(el, EL_PROMPT, prompt_cb))
		goto fail;

	/* set up history */
	if ((flags & EC_EDITLINE_DISABLE_HISTORY) == 0) {
		if (ec_editline_set_history(
				editline, EC_EDITLINE_HISTORY_SIZE, NULL) < 0)
			goto fail;
	}

	/* register completion callback */
	if ((flags & EC_EDITLINE_DISABLE_COMPLETION) == 0) {
		if (el_set(el, EL_ADDFN, "ed-complete", "Complete buffer",
				ec_editline_complete))
			goto fail;
		if (el_set(el, EL_BIND, "^I", "ed-complete", NULL))
			goto fail;
		if (el_set(el, EL_BIND, "?", "ed-complete", NULL))
			goto fail;
	}

	return editline;

fail:
	ec_editline_free(editline);
	return NULL;
}

void ec_editline_free(struct ec_editline *editline)
{
	if (editline == NULL)
		return;
	if (editline->el != NULL)
		el_end(editline->el);
	if (editline->history != NULL)
		history_end(editline->history);
	free(editline->hist_file);
	free(editline->prompt);
	free(editline);
}

EditLine *ec_editline_get_el(struct ec_editline *editline)
{
	return editline->el;
}

const struct ec_node *
ec_editline_get_node(const struct ec_editline *editline)
{
	return editline->node;
}

int
ec_editline_set_node(struct ec_editline *editline, const struct ec_node *node)
{
	if (strcmp(ec_node_get_type_name(node), "sh_lex")) {
		errno = EINVAL;
		return -1;
	}

	editline->node = node;
	return 0;
}

int ec_editline_set_history(struct ec_editline *editline,
	size_t hist_size, const char *hist_file)
{
	EditLine *el = editline->el;

	if (editline->history != NULL)
		history_end(editline->history);
	if (editline->hist_file != NULL) {
		free(editline->hist_file);
		editline->hist_file = NULL;
	}

	if (hist_size == 0)
		return 0;

	editline->history = history_init();
	if (editline->history == NULL)
		goto fail;
	if (history(editline->history, &editline->histev, H_SETSIZE,
			hist_size) < 0)
		goto fail;
	if (history(editline->history, &editline->histev, H_SETUNIQUE, 1))
		goto fail;
	if (hist_file != NULL) {
		editline->hist_file = strdup(hist_file);
		if (editline->hist_file == NULL)
			goto fail;
		// ignore errors
		history(editline->history, &editline->histev,
					H_LOAD, editline->hist_file);
	}
	if (el_set(el, EL_HIST, history, editline->history))
		goto fail;

	return 0;

fail:
	//XXX errno
	if (editline->history != NULL) {
		history_end(editline->history);
		editline->history = NULL;
	}
	return -1;
}

void ec_editline_free_completions(char **matches, size_t len)
{
	size_t i;

	// XXX use malloc/free() instead for consistency
	if (matches == NULL)
		return;
	for (i = 0; i < len; i++)
		free(matches[i]);
	free(matches);
}

ssize_t
ec_editline_get_completions(const struct ec_comp *cmpl, char ***matches_out)
{
	struct ec_comp_item *item;
	char **matches = NULL;
	size_t count = 0;

	EC_COMP_FOREACH(item, cmpl, EC_COMP_FULL | EC_COMP_PARTIAL) {
		char **tmp;

		tmp = realloc(matches, (count + 1) * sizeof(char *));
		if (tmp == NULL)
			goto fail;
		matches = tmp;
		matches[count] = strdup(ec_comp_item_get_display(item));
		if (matches[count] == NULL)
			goto fail;
		count++;
	}

	*matches_out = matches;
	return count;

fail:
	ec_editline_free_completions(matches, count);
	*matches_out = NULL;
	return -1;
}

char *
ec_editline_append_chars(const struct ec_comp *cmpl)
{
	struct ec_comp_item *item;
	const char *append;
	char *ret = NULL;
	size_t n;

	EC_COMP_FOREACH(item, cmpl, EC_COMP_FULL | EC_COMP_PARTIAL) {
		append = ec_comp_item_get_completion(item);
		if (ret == NULL) {
			ret = strdup(append);
			if (ret == NULL)
				goto fail;
		} else {
			n = ec_strcmp_count(ret, append);
			ret[n] = '\0';
		}
	}

	return ret;

fail:
	free(ret);

	return NULL;
}

/* this function builds the help string */
static int get_node_help(const struct ec_comp_item *item,
			struct ec_editline_help *help)
{
	const struct ec_comp_group *grp;
	const struct ec_pnode *pstate;
	const struct ec_node *node;
	const char *node_help = NULL;
	char *node_desc = NULL;

	help->desc = NULL;
	help->help = NULL;

	grp = ec_comp_item_get_grp(item);

	for (pstate = ec_comp_group_get_pstate(grp); pstate != NULL;
	     pstate = ec_pnode_get_parent(pstate)) {
		node = ec_pnode_get_node(pstate);
		if (node_help == NULL)
			node_help = ec_dict_get(ec_node_attrs(node), "help");
		if (node_desc == NULL) {
			node_desc = ec_node_desc(node);
			if (node_desc == NULL)
				goto fail;
		}
	}

	if (node_desc == NULL)
		goto fail;
	if (node_help == NULL)
		node_help = "";

	help->desc = node_desc;
	help->help = strdup(node_help);
	if (help->help == NULL)
		goto fail;

	return 0;

fail:
	free(node_desc);
	free(help->help);
	help->desc = NULL;
	help->help = NULL;
	return -1;
}

ssize_t
ec_editline_get_helps(const struct ec_editline *editline, const char *line,
		      struct ec_editline_help **helps_out)
{
	const struct ec_comp_group *grp, *prev_grp = NULL;
	struct ec_comp_item *item;
	struct ec_comp *cmpl = NULL;
	struct ec_pnode *parse = NULL;
	unsigned int count = 0;
	struct ec_editline_help *helps = NULL;
	char *curline = NULL;

	*helps_out = NULL;

	if (line == NULL) {
		curline = ec_editline_curline(editline, true);
		if (curline == NULL)
			goto fail;
		line = curline;
	}

	/* check if the current line matches */
	parse = ec_parse(editline->node, line);
	if (ec_pnode_matches(parse))
		count = 1;
	ec_pnode_free(parse);
	parse = NULL;

	/* complete at current cursor position */
	cmpl = ec_complete(editline->node, line);
	if (cmpl == NULL) //XXX log error
		goto fail;

	helps = calloc(1, sizeof(*helps));
	if (helps == NULL)
		goto fail;
	if (count == 1) {
		helps[0].desc = strdup("<return>");
		if (helps[0].desc == NULL)
			goto fail;
		helps[0].help = strdup("Validate command.");
		if (helps[0].help == NULL)
			goto fail;
	}

	/* let's display one contextual help per node */
	EC_COMP_FOREACH(item, cmpl,
			EC_COMP_UNKNOWN | EC_COMP_FULL | EC_COMP_PARTIAL) {
		struct ec_editline_help *tmp = NULL;

		/* keep one help per group, skip other items  */
		grp = ec_comp_item_get_grp(item);
		if (grp == prev_grp)
			continue;

		prev_grp = grp;

		tmp = realloc(helps, (count + 1) * sizeof(*helps));
		if (tmp == NULL)
			goto fail;
		helps = tmp;
		if (get_node_help(item, &helps[count]) < 0)
			goto fail;
		count++;
	}

	free(curline);
	ec_comp_free(cmpl);
	*helps_out = helps;

	return count;

fail:
	free(curline);
	ec_pnode_free(parse);
	ec_comp_free(cmpl);
	if (helps != NULL) {
		while (count--) {
			free(helps[count].desc);
			free(helps[count].help);
		}
		free(helps);
	}

	return -1;
}

char *
ec_editline_curline(const struct ec_editline *editline, bool trim_after_cursor)
{
	const LineInfo *line_info = NULL;
	char *line = NULL;
	int pos;

	if ((line_info = el_line(editline->el)) == NULL) {
		errno = ENOENT;
		goto fail;
	}

	if (trim_after_cursor)
		pos = line_info->cursor - line_info->buffer;
	else
		pos = line_info->lastchar - line_info->buffer;

	if (asprintf(&line, "%.*s", pos, line_info->buffer) < 0) {
		errno = ENOMEM;
		goto fail;
	}

	return line;

fail:
	free(line);
	return NULL;
}

int
ec_editline_complete(EditLine *el, int c)
{
	struct ec_editline *editline;
	int ret = CC_REFRESH;
	struct ec_comp *cmpl = NULL;
	char *append = NULL;
	size_t comp_count;
	void *clientdata;
	char *line = NULL;
	FILE *out, *err;

	if (el_get(el, EL_GETFP, 1, &out))
		return -1;
	if (el_get(el, EL_GETFP, 1, &err))
		return -1;

	(void)c;

	if (el_get(el, EL_CLIENTDATA, &clientdata)) {
		fprintf(err, "completion failure: no client data\n");
		goto fail;
	}
	editline = clientdata;
	(void)editline;

	line = ec_editline_curline(editline, true);
	if (line == NULL) {
		fprintf(err, "completion failure: %s\n", strerror(errno));
		goto fail;
	}

	if (editline->node == NULL) {
		fprintf(err, "completion failure: no ec_node\n");
		goto fail;
	}

	cmpl = ec_complete(editline->node, line);
	if (cmpl == NULL)
		goto fail;

	append = ec_editline_append_chars(cmpl);
	comp_count = ec_comp_count(cmpl, EC_COMP_FULL) +
		ec_comp_count(cmpl, EC_COMP_PARTIAL);

	if (c == '?') {
		struct ec_editline_help *helps = NULL;
		ssize_t count = 0;

		count = ec_editline_get_helps(editline, line, &helps);

		fprintf(out, "\n");
		if (ec_editline_print_helps(editline, helps, count) < 0) {
			fprintf(err, "completion failure: cannot show help\n");
			ec_editline_free_helps(helps, count);
			goto fail;
		}

		ec_editline_free_helps(helps, count);
		ret = CC_REDISPLAY;
	} else if (append == NULL || (strcmp(append, "") == 0 && comp_count != 1)) {
		char **matches = NULL;
		ssize_t count = 0;

		count = ec_editline_get_completions(cmpl, &matches);
		if (count < 0) {
			fprintf(err, "completion failure: cannot get completions\n");
			goto fail;
		}

		if (ec_editline_print_cols(
				editline,
				EC_CAST(matches, char **,
					char const * const *),
				count) < 0) {
			fprintf(err, "completion failure: cannot print\n");
			ec_editline_free_completions(matches, count);
			goto fail;
		}

		ec_editline_free_completions(matches, count);
		ret = CC_REDISPLAY;
	} else {
		if (strcmp(append, "") != 0 && el_insertstr(el, append) < 0) {
			fprintf(err, "completion failure: cannot insert\n");
			goto fail;
		}
		if (comp_count == 1) {
			if (el_insertstr(el, " ") < 0) {
				fprintf(err, "completion failure: cannot insert space\n");
				goto fail;
			}
		}
	}

	ec_comp_free(cmpl);
	free(line);
	free(append);

	return ret;

fail:
	ec_comp_free(cmpl);
	free(line);
	free(append);

	return CC_ERROR;
}

ssize_t
ec_editline_get_suggestions(const struct ec_editline *editline,
			    struct ec_editline_help **helps_out,
			    char **full_line, int *char_idx) {
	struct ec_pnode *p = NULL;
	struct ec_comp *c = NULL;
	ssize_t count = -1;
	char *line = NULL;
	size_t len, i;

	if (editline == NULL || helps_out == NULL) {
		errno = EINVAL;
		goto out;
	}

	*helps_out = NULL;

	if ((line = ec_editline_curline(editline, false)) == NULL)
		goto out;

	if (full_line != NULL)
		*full_line = strdup(line);

	len = strlen(line);
	for (i = 0; i <= len + 1; i++) {
		/* XXX: we could do better here be tokenizing properly.
		 * for this, we would need to update the ecoli sh_lex node to
		 * return the token offsets in the parsed struct */
		if (i != (len + 1) && !isspace(line[len - i]))
			continue;
		line[len - i + 1] = '\0';

		if ((p = ec_parse(editline->node, line)) == NULL)
			goto out;
		if ((c = ec_complete(editline->node, line)) == NULL)
			goto out;
		if (ec_pnode_matches(p) || ec_comp_count(c, EC_COMP_ALL) > 0) {
			if (char_idx != NULL)
				*char_idx = strlen(line);
			count = ec_editline_get_helps(
				editline, line, helps_out);
			goto out;
		}
		ec_pnode_free(p);
		p = NULL;
		ec_comp_free(c);
		c = NULL;
	}

	count = 0;

out:
	free(line);
	ec_pnode_free(p);
	ec_comp_free(c);
	return count;
}

char *
ec_editline_gets(struct ec_editline *editline)
{
	EditLine *el = editline->el;
	char *line_copy = NULL;
	const char *line;
	int count;

	line = el_gets(el, &count);
	if (line == NULL)
		return NULL;

	line_copy = strdup(line);
	if (line_copy == NULL)
		goto fail;

	line_copy[strlen(line_copy) - 1] = '\0'; //XXX needed because of sh_lex bug?

	if (editline->history != NULL && !ec_str_is_space(line_copy)) {
		history(editline->history, &editline->histev,
			H_ENTER, line_copy);
		if (editline->hist_file != NULL)
			history(editline->history, &editline->histev,
				H_SAVE, editline->hist_file);
	}

	return line_copy;

fail:
	free(line_copy);
	return NULL;
}

struct ec_pnode *
ec_editline_parse(struct ec_editline *editline)
{
	struct ec_pnode *parse = NULL;
	const struct ec_node *node;
	char *line = NULL;

	node = ec_editline_get_node(editline);
	if (node == NULL)
		goto fail;

	line = ec_editline_gets(editline);
	if (line == NULL)
		goto fail;

	parse = ec_parse(node, line);
	if (parse == NULL)
		goto fail;

	free(line);
	return parse;

fail:
	free(line);
	ec_pnode_free(parse);

	return NULL;
}
