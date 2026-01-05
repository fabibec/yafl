#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static const char *g_input_filename = NULL;
static LogLevel g_log_level = LOG_INFO;

void logger_init(const char *filename) {
    g_input_filename = filename;
    // Skip ./ if present at start
    if (g_input_filename && strncmp(g_input_filename, "./", 2) == 0) {
        g_input_filename += 2;
    }
}

void logger_set_level(LogLevel level) {
    g_log_level = level;
}

static void print_context(int line) {
    if (!g_input_filename || strcmp(g_input_filename, "<stdin>") == 0) return;
    if (line <= 0) return;

    FILE *f = fopen(g_input_filename, "r");
    if (!f) return;

    char buf[1024];
    int current_line = 1;
    while (fgets(buf, sizeof(buf), f)) {
        if (current_line == line) {
            char *p = strchr(buf, '\n');
            if (p) *p = 0;

            fprintf(stderr, "   %d | %s\n", line, buf);
            fprintf(stderr, "       \033[1;31m^\033[0m\n");
            break;
        }
        current_line++;
    }
    fclose(f);
}

void log_error(int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\033[1m%s:%d: \033[1;31merror: \033[0m", g_input_filename ? g_input_filename : "unknown", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);

    print_context(line);
    exit(1);
}

void log_warn(int line, const char *fmt, ...) {
    if (g_log_level < LOG_WARN) return;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\033[1m%s:%d: \033[1;35mwarning: \033[0m", g_input_filename ? g_input_filename : "unknown", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);

    print_context(line);
}

void log_info(int line, const char *fmt, ...) {
    if (g_log_level < LOG_INFO) return;
    va_list args;
    va_start(args, fmt);
    if(line >= 0){
        fprintf(stderr, "\033[1m%s:%d: \033[1;36m", g_input_filename ? g_input_filename : "unknown", line);
    }
    fprintf(stderr, "info: \033[0m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    if (line > 0) print_context(line);
}

void log_debug(int line, const char *fmt, ...) {
    if (g_log_level < LOG_DEBUG) return;
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\033[1m%s:%d: \033[1;34mdebug: \033[0m", g_input_filename ? g_input_filename : "unknown", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    if (line > 0) print_context(line);
}
