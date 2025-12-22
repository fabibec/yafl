#ifndef _CODEGEN_H_
#define _CODEGEN_H_

#include "ast.h"
#include "symtab.h"

void codegen(ast_node *root, char *filename);

#endif
