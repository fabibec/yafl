#ifndef _LOGGER_H_
#define _LOGGER_H_

typedef enum {
    LOG_ERROR = 0,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
} LogLevel;

void logger_init(const char *filename);
void logger_set_level(LogLevel level);

void log_error(int line, const char *fmt, ...);
void log_warn(int line, const char *fmt, ...);
void log_info(int line, const char *fmt, ...);
void log_debug(int line, const char *fmt, ...);

#endif
