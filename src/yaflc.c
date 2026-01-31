#include "ast.h"
#include "codegen.h"
#include "logger.h"
#include "optim.h"
#include "stringbuf.h"
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern ast_node *root;
extern int yyparse();
extern int yylex_destroy();
extern FILE *yyin;

static void cleanup(){
    // Free everything
    if (root) ast_free(root);
    if (yyin && yyin != stdin) fclose(yyin);
    strb_free();
    yylex_destroy();
    /*
        Ensure cursor is visible,
        as debug mode might print out ANSI escapes in the program dump
    */
    printf("\033[?25h");
    fflush(stdout);
}

static void usage(){
    fprintf(stderr, "\033[1m\033[1;36mThe Yafl compiler by Fabian Becker\033[0m\n");
    fprintf(stderr, "Usage: yaflc [options] <input_file>.yafl\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose    Enable debug output\n");
    fprintf(stderr, "  -q, --quiet      Only show warnings and errors\n");
    fprintf(stderr, "  -o, --output     Specify output filename (default: code.yaflb)\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

int main(int argc, char **argv) {
    log_level level = LOG_INFO;
    char *input_file = NULL;
    char *output_file = "code.yaflb";
    atexit(cleanup);

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
            case 'h': usage(); return EXIT_SUCCESS;
            default: usage(); return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        input_file = argv[optind];
        yyin = fopen(input_file, "r");
        if (!yyin) {
            log_error(NO_LINE, "Cannot open input file: %s", input_file);
        }
    } else {
        // No stdin parsing here ;-)
        usage();
        return EXIT_FAILURE;
    }

    logger_init(input_file);
    logger_set_level(level);

    int result = yyparse();
    int ret_code = EXIT_SUCCESS;

    if (result == 0 && root) {
        log_info(NO_LINE, "Parse successful!");
        optimize(root);
        if(logger_get_level() > LOG_INFO){
            char base_file[256];
            snprintf(base_file, sizeof(base_file), "%s", output_file);
            char *ext = strrchr(base_file, '.');
            if (ext) *ext = '\0';
            ast_print_dot(root, base_file);
        }
        codegen(root, output_file);
    } else {
        // yyerror usually exits
        log_error(NO_LINE, "Parse failed!");
    }
    return ret_code;
}
