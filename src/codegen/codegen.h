#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#include "ast.h"
#include "symtab.h"
#include <stdbool.h>

/* Code generation */

void codegen(ast_node *root, char *output_filename);
void codegen_expr(ast_node *node);

#endif
