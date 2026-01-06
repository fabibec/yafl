#ifndef _TYPE_CHECKING_H_
#define _TYPE_CHECKING_H_

#include "ast.h"
#include "symtab.h"
#include "types.h"
#include <stdbool.h>

/* Get type of expression (and check validity recursively) */
yafl_t* type_check_expr(ast_node *node);

/* Check if types are compatible (log error if not) */
void type_check_compatibility(yafl_t *expected, yafl_t *actual, int line, const char *context);

/* Check if all paths return */
bool type_check_return_paths(ast_node *node);

#endif
