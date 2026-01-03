#ifndef UTILS_H
#define UTILS_H

#include "ast.h"

char* strip_pipes(const char* txt);
int parse_base(const char* s);
int parse_based_int(int base, const char* s, int *out);
double parse_based_float(int base, const char* s);

#endif
