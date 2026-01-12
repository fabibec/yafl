#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *g_input_filename = NULL;
static log_level g_log_level = LOG_INFO;

void logger_init(const char *filename) {
    g_input_filename = filename;
    // Skip ./ if present at start
    if (g_input_filename && strncmp(g_input_filename, "./", 2) == 0) {
        g_input_filename += 2;
    }
}

void logger_set_level(log_level level) {
    g_log_level = level;
}

log_level logger_get_level() {
    return g_log_level;
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
            break;
        }
        current_line++;
    }
    fclose(f);
}

static void print_log(log_level lvl, int line, const char *fmt, va_list args){
    if (g_log_level < lvl) return;
    char *txt;
    int color_code;

    switch (lvl){
        case LOG_DEBUG:
            txt = "debug";
            color_code = 34;
            break;
        case LOG_INFO:
            txt = "info";
            color_code = 36;
            break;
        case LOG_WARN:
            txt = "warning";
            color_code = 35;
            break;
        case LOG_ERROR:
        default:
            txt = "error";
            color_code = 31;
            break;
    }

    char file_str[128];
    snprintf(file_str, 128, "%s:%d: ", g_input_filename ? g_input_filename : "unknown", line);
    fprintf(stderr, "\033[1m%s\033[1;%dm%s: \033[0m", (line > 0) ? file_str : "", color_code, txt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    if (line > 0) print_context(line);
}

void log_error(int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_log(LOG_ERROR, line, fmt, args);
    va_end(args);
    exit(1);
}

void log_warn(int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_log(LOG_WARN, line, fmt, args);
    va_end(args);
}

void log_info(int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_log(LOG_INFO, line, fmt, args);
    va_end(args);
}

void log_debug(int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_log(LOG_DEBUG, line, fmt, args);
    va_end(args);
}
