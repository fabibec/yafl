#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#include "ast.h"
#include "symtab.h"
#include "prog.h"
#include <stdbool.h>

/* Code generation */
extern prog_t *prog;

void codegen(ast_node *root, char *output_filename);
void codegen_expr(ast_node *node);
void codegen_push_func_arguments(ast_node *node, func_sym *sym, int arg_count);

#endif // _CODEGEN_H_
