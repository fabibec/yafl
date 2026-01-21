#include "prog.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
/* Yafl bytecode executor or wrapper for the vm3 executable */

/*
    I unset the cursor in some demos, if the user presses Strg+C (SIGINT)
    the cursor isn't reset, so we will force this here
*/
void restore_cursor(void) {
    // ANSI escape code to show cursor
    printf("\033[?25h");
    fflush(stdout);
}

void sig_handler(int signo) {
    if (signo == SIGINT) {
        restore_cursor();
        exit(0);
    }
}

void usage(){
    fprintf(stderr, "\033[1m\033[1;36mThe Yafl bytecode executor by Fabian Becker\033[0m\n");
    fprintf(stderr, "Usage: yafl [options] <input_file>.yaflb [input arguments]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -v, --verbose    Enable debug output\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
    exit(EXIT_FAILURE);
}

void check_file(char * fname) {
    char * file_extension_sep = strrchr(fname, '.');
    if(file_extension_sep && strcmp(file_extension_sep + 1, "yafl") == 0){
        fprintf(stderr, "\033[1m\033[1;31mError:\033[0m This is a raw .yafl file, please use \'yaflc\' to compile first.\n");
        exit(EXIT_FAILURE);
    } else if (file_extension_sep && strcmp(file_extension_sep + 1, "yaflb") == 0){
        return;
    } else {
        fprintf(stderr, "\033[1m\033[1;31mError:\033[0m Invalid file extension received for \'%s\'. Expected .yaflb\n", fname);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    // Register signal handler and cleanup
    signal(SIGINT, sig_handler);
    atexit(restore_cursor);

    char *filename = NULL;
    int debug_idx = E_ERR;

    static struct option long_options[] = {
        {"verbose", no_argument,       0, 'v'},
        {"help",    no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "vh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'v': debug_idx = E_DEBUG; break;
            case 'h': usage(); return 0;
            default: usage(); return 1;
        }
    }

    // Get filename
    if (optind < argc) {
        filename = argv[optind];
    } else {
        usage();
    }
    check_file(filename);

    // Load bytecode
    prog_t *p = prog_read(filename);
    if (!p) {
        fprintf(stderr, "Failed to read bytecode from %s\n", filename);
        exit(EXIT_FAILURE);
    }

    // Create executor + pass remaining args
    exec_t *e = exec_new(p, argc - optind - 1, (const char **)(argv + optind + 1));
    exec_set_debuglvl(e, debug_idx);

    // Run
    exec_run(e);

    return 0;
}
