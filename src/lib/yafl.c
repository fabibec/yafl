#include "prog.h"
#include <stdio.h>
#include <stdlib.h>

/* Wrapper for the vm3 executable so it looks more convenient */
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: yafl <bytecode.yaflb>\n");
        return 1;
    }

    char *filename = argv[1];

    // Load bytecode
    prog_t *p = prog_read(filename);
    if (!p) {
        fprintf(stderr, "Failed to read bytecode from %s\n", filename);
        return 1;
    }

    // Create executor
    exec_t *e = exec_new(p);

    // Optional: set debug level from command line
    if (argc > 2) {
        int debug = atoi(argv[2]);
        exec_set_debuglvl(e, debug);
    }

    // Run
    exec_run(e);

    return 0;
}
