This example shows how to extend libecoli by creating a custom grammar node. We
introduce "node_bool_tuple", which aims to parse and complete a tuple of
booleans, like this:

- ()
- (false)
- (true)
- (false,true)
- (false,true,false,false,true)
...
