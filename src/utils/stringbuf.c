#include "stringbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *str_buffer = NULL;
static int str_size = 0;
static int str_capacity = 0;

void str_reset() {
    if (str_buffer) {
        str_buffer[0] = '\0';
    }
    str_size = 0;
}

void str_append(const char *str, int len) {
    /* Dynamic allocation */
    if (str_size + len + 1 > str_capacity) {
        while (str_size + len + 1 > str_capacity) {
            str_capacity = (str_capacity == 0) ? 16 : str_capacity * 2;
        }
        str_buffer = realloc(str_buffer, str_capacity);

        if (!str_buffer) {
            fprintf(stderr, "realloc failed in stringbuf\n");
            exit(1);
        }
    }
    memcpy(str_buffer + str_size, str, len);
    str_size += len;
    str_buffer[str_size] = '\0';
}

void chr_append(const char c) {
    str_append(&c, 1);
}

void strb_free() {
    if(str_buffer){
        free(str_buffer);
    }
    str_buffer = NULL;
    str_size = 0;
    str_capacity = 0;
}
