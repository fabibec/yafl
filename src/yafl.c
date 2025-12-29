#include "prog.h"
#include <stdio.h>
#include <stdlib.h>

/* Wrapper for the vm3 executable so it looks more convenient */
int main(int argc, char **argv) {
    char *filename = "code.yaflb";
    int debug_idx = 1;

    if (argc >= 2) {
        filename = argv[1];
        debug_idx = 2;
    }

    // Load bytecode
    prog_t *p = prog_read(filename);
    if (!p) {
        fprintf(stderr, "Failed to read bytecode from %s\n", filename);
        if (argc < 2) {
            fprintf(stderr, "Usage: yafl <bytecode.yaflb> [debug_level]\n");
        }
        return 1;
    }

    // Create executor
    exec_t *e = exec_new(p);

    // Optional: set debug level from command line
    if (argc > debug_idx) {
        int debug = atoi(argv[debug_idx]);
        exec_set_debuglvl(e, debug);
    }

    //exec_set_debuglvl(e, E_DEBUG3);

    // Run
    exec_run(e);

    return 0;
}
