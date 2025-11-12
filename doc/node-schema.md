# Node schemas

This page lists all grammar node types supported by libecoli. For each node,
it shows its generic configuration schema, if supported.

## subset

No generic schema.

## str

```
schema str {
    string string  {
        description "The string to match.";
    }
}
```

## space

No generic schema.

## sh_lex

No generic schema.

## seq

```
schema seq {
    list children  {
        description "The list of children nodes, to be parsed in sequence.";

        node  {
            description "A child node which is part of the sequence.";
        }
    }
}
```

## re_lex

```
schema re_lex {
    list patterns  {
        description "The list of patterns elements.";

        dict  {
            description "A pattern element.";

            string pattern  {
                description "The pattern to match.";
            }
            bool keep  {
                description "Whether to keep or drop the string matching the regular expression.";
            }
            string attr  {
                description "The optional attribute name to attach.";
            }
        }
    }
    node child  {
        description "The child node.";
    }
}
```

## re

```
schema re {
    string pattern  {
        description "The pattern to match.";
    }
}
```

## or

```
schema or {
    list children  {
        description "The list of children nodes defining the choice elements.";

        node  {
            description "A child node which is part of the choice.";
        }
    }
}
```

## option

```
schema option {
    node child  {
        description "The child node.";
    }
}
```

## once

```
schema once {
    node child  {
        description "The child node.";
    }
}
```

## none

No generic schema.

## many

```
schema many {
    node child  {
        description "The child node.";
    }
    uint64 min  {
        description "The minimum number of matches (default = 0).";
    }
    uint64 max  {
        description "The maximum number of matches. If 0, there is no maximum (default = 0).";
    }
}
```

## uint

```
schema uint {
    uint64 min  {
        description "The minimum valid value (included).";
    }
    uint64 max  {
        description "The maximum valid value (included).";
    }
    uint64 base  {
        description "The base to use. If unset or 0, try to guess.";
    }
}
```

## int

```
schema int {
    int64 min  {
        description "The minimum valid value (included).";
    }
    int64 max  {
        description "The maximum valid value (included).";
    }
    uint64 base  {
        description "The base to use. If unset or 0, try to guess.";
    }
}
```

## file

No generic schema.

## expr

No generic schema.

## empty

No generic schema.

## dynlist

No generic schema.

## dynamic

No generic schema.

## cond

```
schema cond {
    string expr  {
        description "XXX";
    }
    node child  {
        description "The child node.";
    }
}
```

## cmd

```
schema cmd {
    string expr  {
        description "The expression to match. Supported operators are or '|', list ',', many '+',
                    many-or-zero '*', option '[]', group '()'. An identifier (alphanumeric) can
                    reference a node whose node_id matches. Else it is interpreted as ec_node_str()
                    matching this string. Example: command [option] (subset1, subset2) x|y";
    }
    list children  {
        description "The list of children nodes.";

        node  {
            description "A child node whose id is referenced in the expression.";
        }
    }
}
```

## bypass

```
schema bypass {
    node child  {
        description "The child node.";
    }
}
```

## any

```
schema any {
    string attr  {
        description "The optional attribute name to attach.";
    }
}
```
