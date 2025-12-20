#include <string.h>

/* Helper to strip pipes from variables |n| -> n */
char* strip_pipes(const char* txt) {
    return strndup(txt + 1, strlen(txt) - 2);
}
