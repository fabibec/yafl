#ifndef UTILS_H
#define UTILS_H
/* Parsing helper for compiler AND VM3! */

char* strip_pipes(const char* txt);
int parse_base(const char* s);
int parse_based_int(int base, const char* s, int *out);
int parse_based_float(int base, const char* s, double *out);

#endif
