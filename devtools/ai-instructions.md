# libecoli - AI Instructions

## Code Style & Conventions

See @.editorconfig for indentation and line length limits. See @CONTRIBUTING.md
for C coding style.

## Git Commit Guidelines

See @CONTRIBUTING.md for expected commit message format and git trailers.

- **Title:** Use component-based prefix (e.g., `ip:`, `cli:`, `infra:`) followed
  by all lowercase title except for acronyms/symbols.
- **Use imperative mood:** "add feature" not "added feature"
- **Hard wrap body to 72 columns**
- **Sign-off required:** Use `git commit --signoff`
- **One issue per commit**
- **Emojis are forbidden:** Use regular UTF-8 printable characters.
- **Avoid bullet lists unless necessary:** Prefer regular prose
- **Do not paraphrase the diff:** No need to repeat what files were added,
  modified or changed. Only mention added functions or symbols when this is
  relevant in the context of the commit.
- **Avoid shallow/empty statements:** We don't need AI slop. We need actual
  information that is not present in the diff already.
- **Commit body is NOT optional**

Here is a pathological example:

```
🚀 Restructure core components for better scalability

The following modifications were performed to ensure improved scalability and maintainability:

- Refactored code to follow best practices
- Reorganized files to make the structure cleaner
- Simplified logic in some areas for easier understanding
- Prepared the codebase for future features and improvements

This refactor sets the stage for a more robust architecture.

The following files were modified:

- modules/infra/control/gr_port.h
- modules/infra/control/iface.c
- modules/infra/control/nexthop.c
- modules/infra/control/port.c

Assisted-by: Lame AI v2
Signed-off-by: Foo Bar <foo@bar.com>
```

Here is a good example:

```
rx,tx: use one node per port queue

Instead of dealing with a list of queues to poll packets from,
instantiate one port_rx node clone for each configured port RX queue.

Do the same thing for port_tx and each configured port TX queue.

Depending on worker RX queue mapping, include the associated
port_rx-p*q* clones and initialize their context data accordingly.

Always include *all* port_tx-p*q* clones in all graphs, regardless of
worker TX queue mappings. Instead, configure the port_output node
context to steer packets to the proper port_tx-p*q* clone.

All of this requires delicate juggling with multiple node names lists to
ensure that nodes are cloned when needed before they are affected to
a graph. Secondly, we need to call rte_node_free() on those which are
not used in any graph after all of them have been reloaded.

Signed-off-by: Foo Bar <foo@bar.com>
```
