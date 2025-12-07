/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <histedit.h>

#include <ecoli/complete.h>
#include <ecoli/dict.h>
#include <ecoli/editline.h>
#include <ecoli/interact.h>
#include <ecoli/node.h>
#include <ecoli/parse.h>
#include <ecoli/string.h>
#include <ecoli/strvec.h>
#include <ecoli/utils.h>

struct ec_editline {
	EditLine *el;
	History *history;
	char *hist_file;
	HistEvent histev;
	const struct ec_node *node;
	char *prompt;
};

int ec_editline_term_size(
	const struct ec_editline *editline,
	unsigned int *width,
	unsigned int *height
)
{
	unsigned int _width, _height;

	if (el_get(editline->el, EL_GETTC, "co", &_width, (void *)0))
		return -1;
	if (el_get(editline->el, EL_GETTC, "li", &_height, (void *)0))
		return -1;

	*width = _width;
	*height = _height;

	return 0;
}

static char *prompt_cb(EditLine *el)
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

int ec_editline_set_prompt(struct ec_editline *editline, const char *prompt)
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

int ec_editline_set_prompt_esc(struct ec_editline *editline, const char *prompt, char delim)
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

struct ec_editline *ec_editline(
	const char *prog,
	FILE *f_in,
	FILE *f_out,
	FILE *f_err,
	enum ec_editline_init_flags flags
)
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
		if (ec_editline_set_history(editline, EC_EDITLINE_HISTORY_SIZE, NULL) < 0)
			goto fail;
	}

	/* register completion callback */
	if ((flags & EC_EDITLINE_DISABLE_COMPLETION) == 0) {
		if (el_set(el, EL_ADDFN, "ed-complete", "Complete buffer", ec_editline_complete))
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

const struct ec_node *ec_editline_get_node(const struct ec_editline *editline)
{
	return editline->node;
}

int ec_editline_set_node(struct ec_editline *editline, const struct ec_node *node)
{
	if (strcmp(ec_node_get_type_name(node), "sh_lex")) {
		errno = EINVAL;
		return -1;
	}

	editline->node = node;
	return 0;
}

int ec_editline_set_history(struct ec_editline *editline, size_t hist_size, const char *hist_file)
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
	if (history(editline->history, &editline->histev, H_SETSIZE, hist_size) < 0)
		goto fail;
	if (history(editline->history, &editline->histev, H_SETUNIQUE, 1))
		goto fail;
	if (hist_file != NULL) {
		editline->hist_file = strdup(hist_file);
		if (editline->hist_file == NULL)
			goto fail;
		/* ignore errors */
		history(editline->history, &editline->histev, H_LOAD, editline->hist_file);
	}
	if (el_set(el, EL_HIST, history, editline->history))
		goto fail;

	return 0;

fail:
	if (editline->history != NULL) {
		history_end(editline->history);
		editline->history = NULL;
	}
	return -1;
}

char *ec_editline_curline(const struct ec_editline *editline, bool trim_after_cursor)
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

int ec_editline_complete(EditLine *el, int c)
{
	struct ec_editline *editline;
	int ret = CC_REFRESH;
	struct ec_comp *cmpl = NULL;
	char *append = NULL;
	unsigned int height;
	unsigned int width;
	size_t comp_count;
	void *clientdata;
	char *line = NULL;
	FILE *out, *err;

	if (el_get(el, EL_GETFP, 1, &out))
		return -1;
	if (el_get(el, EL_GETFP, 2, &err))
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

	ec_editline_term_size(editline, &width, &height);
	if (width > 100)
		width = 100;
	if (width < 50)
		width = 50;

	cmpl = ec_complete(editline->node, line);
	if (cmpl == NULL)
		goto fail;

	append = ec_interact_append_chars(cmpl);
	comp_count = ec_comp_count(cmpl, EC_COMP_FULL) + ec_comp_count(cmpl, EC_COMP_PARTIAL);

	if (c == '?') {
		struct ec_interact_help *helps = NULL;
		ssize_t count = 0;
		size_t char_idx;

		count = ec_interact_get_helps(editline->node, line, &helps);
		fprintf(out, "\n");
		if (count != 0 && ec_interact_print_helps(out, width, helps, count) < 0) {
			fprintf(err, "completion failure: cannot show help\n");
			ec_interact_free_helps(helps, count);
			goto fail;
		}
		ec_interact_free_helps(helps, count);

		if (count == 0) {
			count = ec_interact_get_error_helps(
				editline->node, line, &helps, &char_idx
			);
			if (count != 0
			    && ec_interact_print_error_helps(
				       out, width, line, helps, count, char_idx
			       ) < 0) {
				fprintf(err, "completion failure: cannot show help\n");
				ec_interact_free_helps(helps, count);
				goto fail;
			}
			ec_interact_free_helps(helps, count);
		}
		ret = CC_REDISPLAY;
	} else if (append == NULL || (strcmp(append, "") == 0 && comp_count != 1)) {
		char **matches = NULL;
		ssize_t count = 0;

		count = ec_interact_get_completions(cmpl, &matches, EC_COMP_FULL | EC_COMP_PARTIAL);
		if (count < 0) {
			fprintf(err, "completion failure: cannot get completions\n");
			goto fail;
		}

		if (ec_interact_print_cols(
			    out, width, EC_CAST(matches, char **, char const *const *), count
		    )
		    < 0) {
			fprintf(err, "completion failure: cannot print\n");
			ec_interact_free_completions(matches, count);
			goto fail;
		}

		ec_interact_free_completions(matches, count);
		ret = CC_REDISPLAY;
	} else {
		if (strcmp(append, "") != 0 && el_insertstr(el, append) < 0) {
			fprintf(err, "completion failure: cannot insert\n");
			goto fail;
		}
		if (comp_count == 1 && ec_comp_count(cmpl, EC_COMP_FULL) == 1) {
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

char *ec_editline_gets(struct ec_editline *editline)
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

	line_copy[strlen(line_copy) - 1] = '\0'; /* remove \n */

	if (editline->history != NULL && !ec_str_is_space(line_copy)) {
		history(editline->history, &editline->histev, H_ENTER, line_copy);
		if (editline->hist_file != NULL)
			history(editline->history, &editline->histev, H_SAVE, editline->hist_file);
	}

	return line_copy;

fail:
	free(line_copy);
	return NULL;
}

struct ec_pnode *ec_editline_parse(struct ec_editline *editline)
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

int ec_editline_interact(
	struct ec_editline *editline,
	ec_editline_check_exit_cb_t check_exit_cb,
	void *opaque
)
{
	struct ec_interact_help *helps = NULL;
	struct ec_strvec *line_vec = NULL;
	struct ec_pnode *parse = NULL;
	ec_interact_command_cb_t cb;
	const struct ec_node *node;
	size_t char_idx = 0;
	unsigned int height;
	unsigned int width;
	char *line = NULL;
	ssize_t n;
	FILE *out;
	FILE *err;

	if (el_get(editline->el, EL_GETFP, 1, &out))
		return -1;
	if (el_get(editline->el, EL_GETFP, 2, &err))
		return -1;

	node = ec_editline_get_node(editline);
	if (node == NULL)
		goto fail;

	while (check_exit_cb == NULL || !check_exit_cb(opaque)) {
		line = ec_editline_gets(editline);
		if (line == NULL) {
			fprintf(err, "\nExit using ctrl-d\n");
			goto fail;
		}

		line_vec = ec_strvec_sh_lex_str(line, EC_STRVEC_STRICT, NULL);
		if (line_vec == NULL) {
			if (errno == EBADMSG) {
				fprintf(err, "Unterminated quote\n");
				goto again;
			}
			fprintf(err, "Failed to split line\n");
			goto fail;
		}

		if (ec_strvec_len(line_vec) == 0)
			goto again;

		parse = ec_parse(node, line);
		if (parse == NULL) {
			fprintf(err, "Failed to parse command\n");
			goto fail;
		}

		if (!ec_pnode_matches(parse)) {
			ec_editline_term_size(editline, &width, &height);
			if (width > 100)
				width = 100;
			if (width < 50)
				width = 50;

			n = ec_interact_get_error_helps(editline->node, line, &helps, &char_idx);
			if (n < 0
			    || ec_interact_print_error_helps(out, width, line, helps, n, char_idx)
				    < 0)
				fprintf(err, "Invalid command\n");
			if (n >= 0)
				ec_interact_free_helps(helps, n);
			helps = NULL;
			goto again;
		}

		cb = ec_interact_get_callback(parse);
		if (cb == NULL) {
			fprintf(err, "Callback function missing\n");
			goto fail;
		}

		if (cb(parse) < 0) {
			fprintf(err, "Command function returned an error\n");
			goto again;
		}

again:
		free(line);
		line = NULL;
		ec_pnode_free(parse);
		parse = NULL;
		ec_strvec_free(line_vec);
		line_vec = NULL;
	}

	return 0;

fail:
	free(line);
	ec_pnode_free(parse);
	ec_strvec_free(line_vec);

	return -1;
}
