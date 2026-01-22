#include "ast.h"
#include "codegen.h"
#include "logger.h"
#include "optim.h"
#include "stringbuf.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern ast_node *root;
extern int yyparse();
extern int yylex_destroy();
extern FILE *yyin;

static void usage(){
    fprintf(stderr, "\033[1m\033[1;36mThe Yafl compiler by Fabian Becker\033[0m\n");
    fprintf(stderr, "Usage: yaflc [options] <input_file>.yafl\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose    Enable debug output\n");
    fprintf(stderr, "  -q, --quiet      Only show warnings and errors\n");
    fprintf(stderr, "  -o, --output     Specify output filename (default: code.yaflb)\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    log_level level = LOG_INFO;
    char *input_file = NULL;
    char *output_file = "code.yaflb";

    static struct option long_options[] = {
        {"verbose", no_argument,       0, 'v'},
        {"quiet",   no_argument,       0, 'q'},
        {"output",  required_argument, 0, 'o'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "vqo:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'v': level = LOG_DEBUG; break;
            case 'q': level = LOG_WARN; break;
            case 'o': output_file = optarg; break;
            case 'h': usage(); return 0;
            default: usage(); return 1;
        }
    }

    if (optind < argc) {
        input_file = argv[optind];
        yyin = fopen(input_file, "r");
        if (!yyin) {
            log_error(0, "Cannot open input file: %s", input_file);
        }
    } else {
        usage();
    }

    logger_init(input_file ? input_file : "<stdin>");
    logger_set_level(level);

    int result = yyparse();
    int ret_code = 0;

    if (result == 0 && root) {
        log_info(-1, "Parse successful!");
        optimize(root);
        if(logger_get_level() > LOG_INFO){
            ast_print_dot(root, "ast.dot");
        }
        codegen(root, output_file);
        ast_free(root);
    } else {
        // yyerror usually exits
        fprintf(stderr, "Parse failed!\n");
        ret_code = 1;
    }

    // Free everything
    if (input_file && yyin) fclose(yyin);
    strb_free();
    yylex_destroy();

    return ret_code;
}
