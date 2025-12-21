#include <stdio.h>
#include <string.h>
#include "ast.h"


/* Helper to strip pipes from variables |n| -> n */
char* strip_pipes(const char* txt) {
    return strndup(txt + 1, strlen(txt) - 2);
}

/* Helper to check if valid entry point is present */
int has_start_function(ast_node *node) {
    while (node) {
        if (node->type == NODE_FUNC &&
            strcmp(node->data.func.name, "start") == 0) {
            // Verify signature: fn start() -> int
            if (node->data.func.return_type != TYPE_SINT) {
                fprintf(stderr, "Error: start() must return int, not %d\n", node->data.func.return_type);
                return 0;
            }
            // Validate input parameters
            if (node->data.func.params != NULL) {
                fprintf(stderr, "Error: start() must have no parameters\n");
                return 0;
            }
            return 1;
        }
        node = node->next;
    }
    return 0;
}
