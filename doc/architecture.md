@page architecture Architecture

Libecoli is written in C and provides an API for building interactive command
line interfaces. The library consists of several components:

- **Core**: The main API for parsing and completing input strings.
- **Nodes**: The modular component of libecoli. Each node type implements
  specific parsing behavior (integers, strings, regular expressions,
  sequences, etc.).
- **YAML parser**: Loads grammar trees from YAML configuration files.
- **Editline**: Integration helpers for the editline library.
- **Utilities**: Logging, string manipulation, string vectors, hash tables,
  and other support functions.

## The Grammar Graph

Nodes are organized into a directed graph that defines the grammar. Although
this structure is typically a tree, loops are permitted in certain cases. The
grammar graph describes how input is parsed and completed.

Consider the following example:

![Simple grammar tree](simple-tree.svg)

The same structure can be represented textually as:

```
sh_lex(
  seq(
    str(foo),
    option(
      str(bar)
    )
  )
)
```

This grammar matches:

- `"foo"`
- `"foo bar"`

It does not match:

- `"bar"`
- `"foobar"`
- `""` (empty input)

## Parsing

When libecoli parses input, it traverses the grammar graph using depth-first
search and constructs a parse tree. The following example illustrates the
process when @ref ec_parse_strvec() is called:

1. The input is a string vector containing `["foo bar"]`.
2. The `sh_lex` node tokenizes the input using shell lexing rules, producing
   `["foo", "bar"]`, and passes this to its child.
3. The `seq` node forwards the input to its first child.
4. The `str` node checks whether the first token equals `foo`. Since it does,
   the node returns 1 (the number of consumed tokens) to its parent.
5. The `seq` node passes the remaining tokens `["bar"]` to its next child.
6. The `option` node forwards the input to its child, which matches and
   returns 1.
7. The `seq` node has processed all children and returns 2 (total consumed
   tokens).
8. The `sh_lex` node compares the return value against its token count and
   returns 1 (success) to the caller.

When a node fails to match, it returns @ref EC_PARSE_NOMATCH to its parent. This
value propagates up the tree until a node handles the failure. For example, the
`or` node tries each child in sequence until one matches.

## The Parse Tree

Consider another example grammar:

![Grammar with or node](simple-tree2.svg)

Without a lexer node, the input must already be tokenized. This grammar matches:

- `["foo"]`
- `["bar", "1"]`
- `["bar", "100"]`

It does not match:

- `["bar 1"]` (not tokenized)
- `["bar"]` (missing required argument)
- `[]` (empty input)

During parsing, a parse tree is constructed. When parsing succeeds, the tree
describes which grammar nodes matched and what input each node consumed.

Parsing `["bar", "1"]` produces the following parse tree:

![Parse tree structure](parse-tree.svg)

Each parse tree node references the grammar node that matched and stores the
corresponding input tokens:

![Parse tree with token references](parse-tree2.svg)

Not all grammar nodes appear in the parse tree. In this example, `str("foo")`
was not matched and therefore does not appear. Conversely, a single grammar node
may appear multiple times in the parse tree. Consider this grammar that matches
zero or more occurrences of `foo`:

![Grammar with many node](simple-tree3.svg)

Parsing `[foo, foo, foo]` produces:

![Parse tree with repeated matches](parse-tree3.svg)

## Node Identifiers

Each grammar node may have an optional string identifier. This identifier
enables locating specific nodes in the parse tree after parsing completes. For
example, a node created with `ec_node_int("COUNT", 0, 100, 10)` can be found
using `ec_pnode_find(parse_tree, "COUNT")`.

Identifiers need not be unique within the grammar graph. When multiple nodes
share the same identifier, @ref ec_pnode_find() returns the first match. Use
@ref ec_pnode_find_next() to iterate through additional matches.

## Completion

The completion mechanism operates similarly to parsing but collects possible
continuations instead of validating input. When the user requests completion,
libecoli traverses the grammar graph and queries each node for tokens that could
follow the current partial input.

Completions are grouped by the grammar node that produced them. Each
completion item has one of three types:

- @ref EC_COMP_FULL - A complete token (e.g., a command keyword).
- @ref EC_COMP_PARTIAL - A partial token requiring further input (e.g.,
  a directory path).
- @ref EC_COMP_UNKNOWN - Valid input that cannot be completed (e.g., an
  arbitrary string matching a regular expression).

## Node Attributes

Grammar nodes support arbitrary key-value attributes. The interactive layer
uses these attributes to store:

- Help text displayed during completion or on errors
- Callbacks invoked when parsing succeeds
- Short descriptions for contextual help

Use @ref ec_node_attrs() to access the attribute dictionary and manipulate it with
@ref ec_dict_set() and @ref ec_dict_get(). The @ref ecoli_interact API provides
convenience functions such as @ref ec_interact_set_help() and
@ref ec_interact_set_callback().

## Node Configuration

Nodes support a generic configuration system for setting parameters after
creation. Each node type defines a schema describing its configuration options.
The YAML parser uses this system to instantiate nodes from configuration files.

See @ref ecoli_config for details.
