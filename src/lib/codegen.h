#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#include "ast.h"
#include "symtab.h"

void codegen(ast_node *root, char *output_filename);
void codegen_expr(ast_node *node);
void codegen_push_func_arguments(ast_node *node, func_sym *sym, int arg_count);
yafl_t* get_expr_type(ast_node *node);

#endif
