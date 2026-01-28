#ifndef _OPTIM_H_
#define _OPTIM_H_
#include "ast.h"

/* AST Optimization: Constant Folding & Dead Code Elimination */

void optimize(ast_node *root);

#endif // _OPTIM_H_
