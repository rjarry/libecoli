# libecoli

![logo.svg](/doc/logo.svg)

[![Doxygen](https://img.shields.io/badge/doxygen-2C4AA8)](https://rjarry.github.io/libecoli/)

Libecoli is an **E**xtensible **CO**mmand **LI**ne library written in C that
provides a modular, composable framework for building interactive command line
interfaces with dynamic completion, contextual help, and parsing capabilities.

## Use Cases

* Complex interactive command line interfaces (e.g., router or network
  appliance CLIs).
* Application arguments parsing with native bash completion support.
* Generic grammar-based parsers.

## Features

* **Grammar-based parsing**: Define command syntax using composable grammar
  nodes that form a directed graph.
* **Dynamic completion**: Automatic TAB completion based on the grammar, with
  support for runtime-generated suggestions.
* **Contextual help**: Display relevant help messages based on current input
  position.
* **Shell-like tokenization**: Built-in lexers handle quoting, escaping, and
  variable expansion.
* **Interactive editing**: Integration with libedit for line editing, history,
  and key bindings.
* **YAML configuration**: Define grammars in YAML files instead of C code.
* **Extensible**: Write custom node types to provide application-specific
  features.

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

## Documentation

See the [API documentation](https://rjarry.github.io/libecoli/) for details on
grammar nodes, parsing, completion, and the interactive editing API.

## License

libecoli uses the Open Source BSD-3-Clause license. The full license text can
be found in LICENSE.

libecoli makes use of Unique License Identifiers, as defined by the SPDX
project (https://spdx.org/):

- it avoids including large license headers in all files
- it ensures licence consistency between all files
- it improves automated detection of licences

The SPDX tag should be placed in the first line of the file when possible, or
on the second line (e.g.: shell scripts).

## Contributing

Anyone can contribute to libecoli. See [`CONTRIBUTING.md`](/CONTRIBUTING.md).

## Projects that use libecoli

* [6WIND Virtual Service Router](https://doc.6wind.com/new/vsr-3/latest/vsr-guide/user-guide/cli/index.html)
* [grout # a graph router based on DPDK](https://github.com/DPDK/grout)
