#ifndef UTILS_H
#define UTILS_H

#include "ast.h"

char* strip_pipes(const char* txt);
int has_start_function(ast_node *node);
const char *type_to_string(yafl_t type);
int parse_base(const char* s);
int parse_based_int(int base, const char* s, int *out);
double parse_based_float(int base, const char* s);

#endif
