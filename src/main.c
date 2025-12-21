#include <stdio.h>
#include "ast.h"

extern ast_node *root;
extern int yyparse();
extern FILE *yyin;

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "Error: Cannot open file %s\n", argv[1]);
            return 1;
        }
    }

    printf("Parsing...\n");
    int result = yyparse();

    if (result == 0 && root) {
        printf("Parse successful!\n");
        ast_print_dot(root, "ast.dot");
        ast_free(root);
        return 0;
    } else {
        fprintf(stderr, "Parse failed!\n");
        return 1;
    }
}
