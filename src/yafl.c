#include "prog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void usage(void) {
    fprintf(stderr, "Yafl bytecode executor\n"
        "Usage: yafl <filename>.yaflb\n"
        "    <filename>.yaflb  Bytecode filename\n"
        "\n"
        );
    exit(EXIT_FAILURE);
}

void check_file(char * fname) {
    char * file_extension_sep = strrchr(fname, '.');
    if(file_extension_sep && strcmp(file_extension_sep + 1, "yafl") == 0){
        fprintf(stderr, "This is a raw .yafl file, please use \'yaflc\' to compile first.\n");
        exit(EXIT_FAILURE);
    } else if (strcmp(file_extension_sep + 1, "yaflb") == 0){
        return;
    } else {
        fprintf(stderr, "Invalid file extension \'%s\' received.\n", file_extension_sep + 1);
        exit(EXIT_FAILURE);
    }
}

/* Wrapper for the vm3 executable so it looks more convenient */
int main(int argc, char **argv) {
    char *filename = "code.yaflb";
    int debug_idx = E_ERR;

    if (argc >= 2) {
        filename = argv[1];
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

    // Create executor
    exec_t *e = exec_new(p, argc - 2, (const char **)(argv + 2));
    exec_set_debuglvl(e, debug_idx);

    // // Optional: set debug level from command line
    // if (argc > debug_idx) {
    //     int debug = atoi(argv[debug_idx]);
    //     exec_set_debuglvl(e, debug);
    // }

    // Run
    exec_run(e);

    return 0;
}
