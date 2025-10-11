/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_editline Editline
 *
 * @brief Helpers for editline
 *
 * Helpers that can be used to associate an editline instance with
 * an ecoli node tree.
 *
 * XXX support multiline edition
 * XXX set prompt
 */

#pragma once

#include <stdbool.h>

#include <histedit.h>

struct ec_editline;
struct ec_node;
struct ec_pnode;
struct ec_comp;

/**
 * 
 */
struct ec_editline_help {
	char *desc;
	char *help;
};

/**
 * Default history size.
 */
#define EC_EDITLINE_HISTORY_SIZE 128

/**
 * Ask the terminal to not send signals (STOP, SUSPEND, XXX). The
 * `ctrl-c`, `ctrl-z` will be interpreted as standard characters. An
 * action can be associated to these characters with:
 *
 *	static int cb(EditLine *editline, int c) {
 *	{
 *		see editline documentation for details
 *	}
 *
 *	if (el_set(el, EL_ADDFN, "ed-foobar", "Help string about foobar", cb))
 *		handle_error;
 *	if (el_set(el, EL_BIND, "^C", "ed-break", NULL))
 *		handle_error;
 *
 * The default behavior (without this flag) is to let the signal pass: ctrl-c
 * will stop program and ctrl-z will suspend it.
 */
#define EC_EDITLINE_DISABLE_SIGNALS 0x01

/**
 * Disable history. The default behavior creates an history with
 * EC_EDITLINE_HISTORY_SIZE entries. To change this value, use
 * ec_editline_set_history().
 */
#define EC_EDITLINE_DISABLE_HISTORY 0x02

/**
 * Disable completion. The default behavior is to complete when
 * `?` or `<tab>` is hit. You can register your own callback with:
 *
 *	if (el_set(el, EL_ADDFN, "ed-complete", "Complete buffer", callback))
 *		handle_error;
 *	if (el_set(el, EL_BIND, "^I", "ed-complete", NULL))
 *		handle_error;
 *
 * The default used callback is ec_editline_complete().
 */
#define EC_EDITLINE_DISABLE_COMPLETION 0x04

/**
 * Use editline own signal handler for the following signals when reading
 * command input: SIGCONT, SIGHUP, SIGINT, SIGQUIT, SIGSTOP, SIGTERM, SIGTSTP,
 * and SIGWINCH. Otherwise, the current signal handlers will be used.
 */
#define EC_EDITLINE_DEFAULT_SIGHANDLER 0x08

typedef int (*ec_editline_cmpl_t)(struct ec_editline *editline, int c);

/**
 * Create an editline instance with default behavior.
 *
 * XXX Wrapper to editline's el_init() 
 *
 * It 
 */
struct ec_editline *
ec_editline(const char *name, FILE *f_in, FILE *f_out, FILE *f_err,
	unsigned int flags);

/**
 * Free an editline instance allocated with ec_editline().
 */
void ec_editline_free(struct ec_editline *editline);

/**
 * Return the editline instance attached to the ec_editline object.
 */
EditLine *ec_editline_get_el(struct ec_editline *editline);

// XXX public?
const struct ec_node *ec_editline_get_node(struct ec_editline *editline);
void ec_editline_set_node(struct ec_editline *editline,
			const struct ec_node *node);

//XXX get history, get_...

/**
 * Change the history size.
 *
 * The default behavior is to have an history whose size
 * is EC_EDITLINE_HISTORY_SIZE. This can be changed with this
 * function.
 *
 * @param editline
 *   The pointer to the ec_editline structure.
 * @param hist_size
 *   The desired size of the history.
 * @param hist_file
 *   Optional file path to load and write the history to.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_editline_set_history(struct ec_editline *editline,
	size_t hist_size, const char *hist_file);

int
ec_editline_print_cols(struct ec_editline *editline,
		char const * const *matches, size_t n);

void ec_editline_free_completions(char **matches, size_t len);
ssize_t
ec_editline_get_completions(const struct ec_comp *cmpl, char ***matches_out);
char *
ec_editline_append_chars(const struct ec_comp *cmpl);

ssize_t
ec_editline_get_helps(const struct ec_editline *editline, const char *line,
	const char *full_line, struct ec_editline_help **helps_out);
int
ec_editline_print_helps(struct ec_editline *editline,
			const struct ec_editline_help *helps, size_t n);
void
ec_editline_free_helps(struct ec_editline_help *helps, size_t len);

ssize_t
ec_editline_get_suggestions(const struct ec_editline *editline,
			    struct ec_editline_help **suggestions,
			    char **full_line, int *pos);

int
ec_editline_set_prompt(struct ec_editline *editline, const char *prompt);

int
ec_editline_set_prompt_esc(struct ec_editline *editline, const char *prompt,
			   char delim);

char *ec_editline_curline(const struct ec_editline *editline,
			  bool trim_after_cursor);

/**
 * Get a line.
 *
 * The returned line must be freed by the caller using ec_free().
 */
char *ec_editline_gets(struct ec_editline *editline);

/**
 * Get a line (managing completion) and parse it with passed node
 * XXX find a better name?
 */
struct ec_pnode *
ec_editline_parse(struct ec_editline *editline, const struct ec_node *node);

int
ec_editline_complete(EditLine *el, int c);
