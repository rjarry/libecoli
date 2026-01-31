/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2018, Olivier MATZ <zer0@droids-corp.org>
 */

/**
 * @defgroup ecoli_editline Editline
 * @{
 *
 * @brief Helpers for editline
 *
 * Helpers that can be used to associate an editline instance with
 * an ecoli node tree.
 */

#pragma once

#include <stdbool.h>
#include <stdio.h>

struct ec_node;
struct ec_pnode;
struct ec_comp;
struct editline;

/** Editline instance for interactive command line editing. */
struct ec_editline;

/**
 * A structure describing a contextual help.
 */
struct ec_editline_help {
	char *desc; /**< The short description of the item. */
	char *help; /**< The associated help. */
};

/**
 * Default history size.
 */
#define EC_EDITLINE_HISTORY_SIZE 128

/**
 * Flags passed at ec_editline initialization.
 */
enum ec_editline_init_flags {
	/**
	 * Ask the terminal to not send signals (STOP, SUSPEND, ...). The
	 * `ctrl-c`, `ctrl-z` will be interpreted as standard characters. An
	 * action can be associated with these characters with:
	 *
	 * @code{.c}
	 *	static int cb(EditLine *editline, int c) {
	 *	{
	 *		see editline documentation for details
	 *	}
	 *
	 *	if (el_set(el, EL_ADDFN, "ed-foobar", "Help string about foobar", cb))
	 *		handle_error;
	 *	if (el_set(el, EL_BIND, "^C", "ed-break", NULL))
	 *		handle_error;
	 * @endcode
	 *
	 * The default behavior (without this flag) is to let the signal pass: ctrl-c
	 * will stop program and ctrl-z will suspend it.
	 */
	EC_EDITLINE_DISABLE_SIGNALS = 1 << 0,

	/**
	 * Disable history. The default behavior creates a history with
	 * ::EC_EDITLINE_HISTORY_SIZE entries. To change this value, use
	 * ec_editline_set_history().
	 */
	EC_EDITLINE_DISABLE_HISTORY = 1 << 1,

	/**
	 * Disable completion. The default behavior is to complete when
	 * `?` or `<tab>` is hit. You can register your own callback with:
	 *
	 * @code{.c}
	 *	if (el_set(el, EL_ADDFN, "ed-complete", "Complete buffer", callback))
	 *		handle_error;
	 *	if (el_set(el, EL_BIND, "^I", "ed-complete", NULL))
	 *		handle_error;
	 * @endcode
	 *
	 * The default used callback is ec_editline_complete().
	 */
	EC_EDITLINE_DISABLE_COMPLETION = 1 << 2,

	/**
	 * Use editline own signal handler for the following signals when reading
	 * command input: SIGCONT, SIGHUP, SIGINT, SIGQUIT, SIGSTOP, SIGTERM, SIGTSTP,
	 * and SIGWINCH. Otherwise, the current signal handlers will be used.
	 */
	EC_EDITLINE_DEFAULT_SIGHANDLER = 1 << 3,
};

/**
 * Type of callback invoked by ec_editline_interact() to check if loop should
 * exit (when the function return true), or continue (when the function return
 * false).
 */
typedef int (*ec_editline_check_exit_cb_t)(void *opaque);

/**
 * Create an editline instance with default behavior.
 *
 * It allocates and initializes an ::ec_editline structure, calls editline's
 * el_init(), and does the editline configuration according to given flags.
 *
 * After that, the user must call ec_editline_set_node() to attach the grammar
 * to the editline.
 *
 * @param prog
 *   The name of the invoking program, used when reading the editrc(5) file
 *   to determine which settings to use.
 * @param f_in
 *   The input stream to use.
 * @param f_out
 *   The output stream to use.
 * @param f_err
 *   The error stream to use.
 * @param flags
 *   Flags to customize initialization. See ::ec_editline_init_flags.
 * @return
 *   The allocated ec_editline structure, or NULL on error.
 */
struct ec_editline *ec_editline(
	const char *prog,
	FILE *f_in,
	FILE *f_out,
	FILE *f_err,
	enum ec_editline_init_flags flags
);

/**
 * Free an editline structure allocated with ec_editline().
 *
 * @param editline
 *   The pointer to the ::ec_editline structure to free.
 */
void ec_editline_free(struct ec_editline *editline);

/**
 * Return the wrapped editline instance attached to the ::ec_editline structure.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 */
struct editline *ec_editline_get_el(struct ec_editline *editline);

/**
 * Get terminal width and height.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @param width
 *   The pointer where the number of columns is stored on success.
 * @param height
 *   The pointer where the number of rows is stored on success.
 * @return
 *   0 on success, or -1 on error (in this case the value pointed by width and
 *   height are not modified).
 */
int ec_editline_term_size(
	const struct ec_editline *editline,
	unsigned int *width,
	unsigned int *height
);

/**
 * Attach an ecoli node to the editline structure.
 *
 * This node must be an sh_lex node, with its grammar subtree. It will be used
 * for completion and contextual help. The contextual help description is
 * attached as a string to nodes using a node attribute EC_EDITLINE_HELP_ATTR.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @param node
 *   The pointer to the sh_lex ::ec_node to use as grammar.
 * @return
 *   0 on success, or -1 on error. errno is set to EINVAL if the node is not
 *   of type sh_lex.
 */
int ec_editline_set_node(struct ec_editline *editline, const struct ec_node *node);

/**
 * Return the ecoli node attached to the editline structure.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @return
 *   The pointer to the ::ec_node.
 */
const struct ec_node *ec_editline_get_node(const struct ec_editline *editline);

/**
 * Change the history size.
 *
 * The default behavior is to have a history whose size
 * is ::EC_EDITLINE_HISTORY_SIZE. This can be changed with this
 * function.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @param hist_size
 *   The desired size of the history.
 * @param hist_file
 *   Optional file path to load and write the history to.
 * @return
 *   0 on success, or -1 on error (errno is set).
 */
int ec_editline_set_history(struct ec_editline *editline, size_t hist_size, const char *hist_file);

/**
 * Set editline prompt.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @param prompt
 *   The prompt string to use.
 * @return
 *   0 on success, or -1 on error.
 */
int ec_editline_set_prompt(struct ec_editline *editline, const char *prompt);

/**
 * Set editline escaped prompt.
 *
 * From el_set(3):
 *
 * If the start/stop delim character is found in the prompt, the character
 * itself is not printed, but characters after it are printed directly to the
 * terminal without affecting the state of the current line. A subsequent second
 * start/stop literal character ends this behavior. This is typically used to
 * embed literal escape sequences that change the color/style of the terminal in
 * the prompt.  Note that the literal escape character cannot be the last
 * character in the prompt, as the escape sequence is attached to the next
 * character in the prompt.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @param prompt
 *   The prompt string to use.
 * @param delim
 *   The start/stop literal prompt character.
 * @return
 *   0 on success, or -1 on error.
 */
int ec_editline_set_prompt_esc(struct ec_editline *editline, const char *prompt, char delim);

/**
 * Get the current edited line.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @param trim_after_cursor
 *   If true, remove all characters starting from cursor (included).
 * @return
 *   An allocated string containing the current line. It must be freed by the
 *   user. Return NULL on error.
 */
char *ec_editline_curline(const struct ec_editline *editline, bool trim_after_cursor);

/**
 * Get a line interactively (with completion).
 *
 * Wrapper to libedit el_gets(). It returns the edited line without its "\n",
 * and adds it to the history.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @return
 *   An allocated string containing the edited line, that must be freed by the
 *   caller using free().
 */
char *ec_editline_gets(struct ec_editline *editline);

/**
 * Get a line interactively (with completion), and parse it.
 *
 * Similar to ec_editline_gets(), except that the string result is parsed using
 * the grammar node on success.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @return
 *   An allocated ::ec_pnode containing the parse result. It must be freed by the
 *   caller using ec_pnode_free(). Return NULL on error.
 */
struct ec_pnode *ec_editline_parse(struct ec_editline *editline);

/**
 * Interact with the user until exit.
 *
 * Run the command line, parsing and completing commands on demand. When a
 * command is parsed successfully, invoke the callback attached to the grammar
 * node as a ::EC_INTERACT_CB_ATTR attribute. The callback type must be
 * ::ec_interact_command_cb_t.
 *
 * On EOF, or if check_exit() function returns true, exit from the interactive
 * loop.
 *
 * @param editline
 *   The pointer to the ::ec_editline structure.
 * @param check_exit_cb
 *   Optional function pointer executed before each loop iteration to check if
 *   loop should be exited (when the function return true).
 * @param opaque
 *   User pointer, passed as-is to check_exit_cb().
 * @return
 *   0 if interactive loop exited gracefully, or -1 on error.
 */
int ec_editline_interact(
	struct ec_editline *editline,
	ec_editline_check_exit_cb_t check_exit_cb,
	void *opaque
);

/**
 * Default completion callback used by editline.
 *
 * It displays the list of completions with TAB, and a contextual help
 * with '?'.
 *
 * @param el
 *   The pointer to libedit Editline structure.
 * @param c
 *   The character used to complete.
 * @return
 *   An editline error code: CC_REFRESH, CC_ERROR, or CC_REDISPLAY.
 */
int ec_editline_complete(struct editline *el, int c);

/** @} */
