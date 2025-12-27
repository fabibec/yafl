#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "utils.h"


/* Helper to strip pipes from variables |n| -> n */
char* strip_pipes(const char* txt) {
    return strndup(txt + 1, strlen(txt) - 2);
}

/* Helper to check if valid entry point is present */
int has_start_function(ast_node *node) {
    while (node) {
        if (node->type == NODE_FUNC &&
            strcmp(node->data.func.name, "start") == 0) {
            // Verify signature: fn start() -> int
            if (!node->data.func.return_type || node->data.func.return_type->base_t != TYPE_SINT) {
                char buf[128];
                type_to_str(node->data.func.return_type, buf, sizeof(buf));
                fprintf(stderr, "Error: start() must return int, not %s\n", buf);
                return 0;
            }
            // Validate input parameters
            if (node->data.func.params != NULL) {
                fprintf(stderr, "Error: start() must have no parameters\n");
                return 0;
            }
            return 1;
        }
        node = node->next;
    }
    return 0;
}

int digit_val(char c) {
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

int parse_base(const char *s) {
    int base = 0;
    // Read base
    while(isdigit((unsigned char)*s))
        base = base * 10 + (*s++ - '0');

    if(base < 2 || base > 16)
        return 0;

    return base;
}

int parse_based_int(int base, const char *s, int* out) {
    int value = 0;

    for (; *s; s++){
        if (*s == '_') { s++; continue; }

        int d = digit_val(*s);
        if (d < 0 || d >= base) return 0;
        value = value * base + d;
    }
    *out = value;
    return 1;
}

double parse_based_float(int base, const char* s) {
    int exp = 0;
    double value = 0.0;
    double frac = 1.0;

    // Integer part
    while(*s && *s != '.' && *s != '^') {
        if (*s == '_') { s++; continue; }

        int d = digit_val(*s++);
        if (d < 0 || d >= base) return NAN;
        value = value * base + d;
    }

    // fraction
    if (*s == '.') {
        s++;
        while (*s && *s != '^') {
            if (*s == '_') { s++; continue; }

            int d = digit_val(*s++);
            if (d < 0 || d >= base) return NAN;
            frac /= base;
            value += d * frac;
        }
    }

    // exponent (opt.)
    if (*s == '^') {
        s++;
        exp = strtol(s, NULL, 16);
    }

    return value * pow(base, exp);
}