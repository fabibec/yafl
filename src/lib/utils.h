#ifndef UTILS_H
#define UTILS_H

#include "ast.h"

char* strip_pipes(const char* txt);
int has_start_function(ast_node *node);
const char *type_to_string(yafl_t type);

#endif
