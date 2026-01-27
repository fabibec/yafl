#include "prog.h"
#include "logger.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
/* Yafl bytecode executor or wrapper for the vm3 executable */

static exec_t *e;
static prog_t *p;

static void cleanup(){
    // GC Cleanup
    vals_unmark();
    vals_sweep();

    if (e) {
        if (e->vstack) vstack_free(e->vstack);
        if (e->vars) vstack_free(e->vars);
        free(e);
    }
    if (p) {
        free(p);
    }
    // ANSI escape code to show cursor
    printf("\033[?25h");
    fflush(stdout);
}

/*
    I unset the cursor in some demos, if the user presses Strg+C (SIGINT)
    the cursor isn't reset, so we will force this here
*/
void sig_handler(int signo) {
    if (signo == SIGINT) {
        cleanup();
        exit(EXIT_SUCCESS);
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
        log_error(NO_LINE, "This is a raw .yafl file, please use \'yaflc\' to compile first.");
    } else if (file_extension_sep && strcmp(file_extension_sep + 1, "yaflb") == 0){
        return;
    } else {
        log_error(NO_LINE, "Invalid file extension received for \'%s\'. Expected \'.yaflb\'.", fname);
    }
}

int main(int argc, char **argv) {
    // Register signal handler and cleanup
    signal(SIGINT, sig_handler);
    atexit(cleanup);

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
        check_file(filename);
    } else {
        usage();
    }

    // Load bytecode
    p = prog_read(filename);
    if (!p) {
        log_error(NO_LINE, "Failed to read bytecode from %s\n", filename);
    }

    // Create executor + pass remaining args
    e = exec_new(p, argc - optind - 1, (const char **)(argv + optind + 1));
    exec_set_debuglvl(e, debug_idx);

    // Run
    exec_run(e);

    return 0;
}
