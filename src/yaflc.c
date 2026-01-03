#include <stdio.h>
#include "ast.h"
#include "codegen.h"
#include "stringbuf.h"

extern ast_node *root;
extern int yyparse();
extern int yylex_destroy();
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
    int ret_code = 0;

    if (result == 0 && root) {
        printf("Parse successful!\n");
        ast_print_dot(root, "ast.dot");
        codegen(root, "code.yaflb");
        ast_free(root);
    } else {
        fprintf(stderr, "Parse failed!\n");
        ret_code = 1;
    }

    // Free everything
    fclose(yyin);
    strb_free();
    yylex_destroy();

    return ret_code;
}
