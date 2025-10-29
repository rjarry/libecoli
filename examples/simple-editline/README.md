This is the most simple example that demonstrates a common use-case for
libecoli. It shows how to build a custom CLI based on libedit, with completion
and contextual help.

The top-level helpers of `ec_editline` API are used (`ec_interact()`), making
the code as compact as possible.

The grammar is defined in one function `create_commands()`. The callbacks are
registered using `ec_editline_set_callback()`, and the contextual helps using
`ec_editline_set_help()`.
