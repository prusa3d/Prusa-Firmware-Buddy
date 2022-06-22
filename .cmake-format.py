# If a statement is wrapped to more than one line, than dangle the closing
# parenthesis on it's own line.
dangle_parens = True
dangle_align = 'child'

# If true, the parsers may infer whether or not an argument list is sortable
# (without annotation).
autosort = True

# How wide to allow formatted cmake files
line_width = 100

additional_commands = {
    "target_sources": {
        "kwargs": {
            "PUBLIC": {
                "pargs": {
                    "nargs": "*",
                    "tags": ["file-list"],
                    "sortable": True
                }
            },
            "PRIVATE": {
                "pargs": {
                    "nargs": "*",
                    "tags": ["file-list"],
                    "sortable": True
                }
            },
            "INTERFACE": {
                "pargs": {
                    "nargs": "*",
                    "tags": ["file-list"],
                    "sortable": True
                }
            },
        }
    },
}
