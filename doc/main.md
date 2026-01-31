@mainpage About Libecoli

Libecoli is an <b>E</b>xtensible <b>CO</b>mmand <b>LI</b>ne library written in
C that provides a modular, composable framework for building interactive command
line interfaces with dynamic completion, contextual help, and parsing
capabilities.

Project page: https://github.com/rjarry/libecoli

## Key Features

- **Grammar-based parsing**: Define command syntax using composable grammar
  nodes that form a directed graph.

- **Dynamic completion**: Automatic TAB completion based on the grammar, with
  support for runtime-generated suggestions.

- **Contextual help**: Display relevant help messages based on current input
  position.

- **Shell-like tokenization**: Built-in lexers handle quoting, escaping, and
  variable expansion.

- **Interactive editing**: Integration with libedit for line editing, history,
  and key bindings.

- **YAML configuration**: Define grammars in YAML files instead of C code.

- **Expression evaluation**: Parse and evaluate arithmetic or custom
  expressions with user-defined operators.

## Core Concepts

### Grammar Graph

A grammar is built by composing @ref ecoli_nodes into a directed graph. Each
node type matches specific input patterns:

- **Terminal nodes** match actual input tokens: @ref ec_node_str() matches
  a literal string, @ref ec_node_int() matches integers, @ref ec_node_re()
  matches regular expressions, the file node matches filesystem paths.

- **Composite nodes** combine other nodes: @ref ec_node_seq() matches children
  in sequence, @ref ec_node_or() matches any one child, @ref ec_node_many()
  matches a child repeatedly, @ref ec_node_option() makes a child optional.

- **Lexer nodes** tokenize raw input before passing to children:
  @ref ec_node_sh_lex() provides shell-like tokenization with quote handling.

### Parsing

The @ref ecoli_parse API validates input against a grammar and produces a parse
tree. Each node in the tree references the grammar node that matched it and the
tokens it consumed. Use @ref ec_pnode_find() to locate specific nodes by their
identifier and extract matched values.

### Completion

The @ref ecoli_complete API generates completion suggestions for partial input.
Completions are grouped by the grammar node that produced them and can be
filtered by type (full match, partial match, or unknown).

### Interactive Mode

The @ref ecoli_editline API provides a complete interactive command line with:

- Line editing with libedit
- TAB and `?` key completion
- Command history with file persistence
- Callback-based command execution
- Contextual help display on errors

## Quick Example

```c
#include <ecoli.h>

static bool done;

static int hello_cb(const struct ec_pnode *p) {
	const char *name = ec_strvec_val(
		ec_pnode_get_strvec(ec_pnode_find(p, "NAME")), 0);
	printf("Hello, %s!\n", name);
	return 0;
}

static int exit_cb(const struct ec_pnode *p) {
	(void)p;
	done = true;
	return 0;
}

static bool check_exit_cb(void *priv) {
	(void)priv;
	return done;
}

int main(void) {
	struct ec_node *cmd, *root;
	struct ec_editline *edit;

	ec_init();

	// Build grammar: "hello <name>" or "exit"
	root = ec_node("or", EC_NO_ID);

	cmd = EC_NODE_SEQ(EC_NO_ID,
		ec_node_str(EC_NO_ID, "hello"),
		ec_node_any("NAME", NULL));
	ec_interact_set_callback(cmd, hello_cb);
	ec_interact_set_help(cmd, "Say hello to someone.");
	ec_node_or_add(root, cmd);

	cmd = ec_node_str(EC_NO_ID, "exit");
	ec_interact_set_callback(cmd, exit_cb);
	ec_interact_set_help(cmd, "Exit the program.");
	ec_node_or_add(root, cmd);

	// Add shell lexer for quote/escape handling
	root = ec_node_sh_lex(EC_NO_ID, root);

	// Create interactive session
	edit = ec_editline("example", stdin, stdout, stderr, 0);
	ec_editline_set_prompt(edit, "> ");
	ec_editline_set_node(edit, root);
	ec_editline_interact(edit, check_exit_cb, NULL);

	ec_editline_free(edit);
	ec_node_free(root);
	ec_exit();

	return 0;
}
```

See the [Examples](examples.html) page for complete working examples
demonstrating grammar construction, completion, readline integration, custom
node types, and dynamic completion from runtime data.

## Available Node Types

### Terminal Nodes

| Node Type | Description |
|-----------|-------------|
| @ref ecoli_node_str | Matches a specific literal string |
| @ref ecoli_node_int | Matches signed integers with range/base constraints |
| @ref ecoli_node_re | Matches input against a POSIX extended regex |
| @ref ecoli_node_file | Matches and completes filesystem paths |
| @ref ecoli_node_any | Matches any single token |
| @ref ecoli_node_empty | Matches zero tokens (always succeeds) |
| @ref ecoli_node_none | Matches nothing (always fails) |
| @ref ecoli_node_space | Matches whitespace |

### Composite Nodes

| Node Type | Description |
|-----------|-------------|
| @ref ecoli_node_seq | Matches children in strict order |
| @ref ecoli_node_or | Matches any one of its children |
| @ref ecoli_node_many | Matches a child repeatedly (with min/max) |
| @ref ecoli_node_option | Makes a child optional |
| @ref ecoli_node_subset | Matches any subset of children in any order |

### Advanced Nodes

| Node Type | Description |
|-----------|-------------|
| @ref ecoli_node_cmd | Parses commands using a format string syntax |
| @ref ecoli_node_expr | Parses expressions with operators and precedence |
| @ref ecoli_node_dynamic | Builds grammar dynamically at parse time |
| @ref ecoli_node_dynlist | Matches names from a runtime-generated list |
| @ref ecoli_node_cond | Conditionally matches based on an expression |
| @ref ecoli_node_once | Prevents a child from matching more than once |
| @ref ecoli_node_bypass | Pass-through node for building graph loops |

### Lexer Nodes

| Node Type | Description |
|-----------|-------------|
| @ref ecoli_node_sh_lex | Shell-like tokenization with quotes and escapes |
| @ref ecoli_node_re_lex | Regex-based tokenization |

## API Reference

- @ref architecture - How libecoli works internally
- @ref ecoli_init - Library initialization
- @ref ecoli_nodes - Grammar node types and operations
- @ref ecoli_parse - Parsing API
- @ref ecoli_complete - Completion API
- @ref ecoli_editline - Interactive editing with libedit
- @ref ecoli_interact - Command callbacks and help system
- @ref ecoli_yaml - YAML grammar import/export
- @ref ecoli_config - Node configuration system
- @ref ecoli_log - Logging facilities

## Real-World Usage

Libecoli powers the CLI of several production systems including the
[grout](https://github.com/DPDK/grout) graph router and 6WIND's Virtual Service
Router. These projects demonstrate patterns such as:

- Modular command registration with constructor functions
- Dynamic completion callbacks that query live system state
- Hierarchical command contexts (e.g., `ip route add ...`)
- Typed argument extraction helpers
- JSON export of command trees for documentation generation
