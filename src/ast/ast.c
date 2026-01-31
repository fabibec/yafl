#include "arith.h"
#include "ast.h"
#include "logger.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* From flex */
extern int yylineno;

/* Base utility */
ast_node *ast_new_node(ast_node_t type) {
    ast_node *node = calloc(1, sizeof(ast_node));
    node->type = type;
    node->line = yylineno;
    node->next = NULL;
    return node;
}

/* Node creation */
ast_node* ast_new_func(char* name, ast_node* params, yafl_t *return_type, ast_node* body) {
    ast_node *node = ast_new_node(NODE_FUNC);
    node->data.func.name = name;
    node->data.func.params = params;
    node->data.func.return_type = return_type;
    node->data.func.body = body;
    return node;
}

ast_node* ast_new_param(char * name, yafl_t *type, ast_node *default_val) {
    ast_node *node = ast_new_node(NODE_PARAM);
    node->data.param.name = name;
    node->data.param.type = type;
    node->data.param.default_value = default_val;
    return node;
}

ast_node *ast_new_ret(ast_node *value) {
    ast_node *node = ast_new_node(NODE_RETURN);
    node->data.ret.value = value;
    return node;
}

ast_node *ast_new_call(char *name, ast_node *args) {
    ast_node *node = ast_new_node(NODE_CALL);
    node->data.call.name = name;
    node->data.call.args = args;
    return node;
}

ast_node *ast_new_decl(yafl_t *type, char *name, ast_node *init) {
    ast_node *node = ast_new_node(NODE_DECL);
    node->data.decl.type = type;
    node->data.decl.name = name;
    node->data.decl.init = init;
    return node;
}

ast_node *ast_new_assign(char *name, ast_node *value) {
    ast_node *node = ast_new_node(NODE_ASSIGN);
    node->data.assign.name = name;
    node->data.assign.value = value;
    return node;
}


ast_node *ast_new_if(ast_node *cond, ast_node *then_b, ast_node *else_b) {
    ast_node *node = ast_new_node(NODE_IF);
    node->data.if_stmt.condition = cond;
    node->data.if_stmt.then_block = then_b;
    node->data.if_stmt.else_block = else_b;
    return node;
}


ast_node *ast_new_for(ast_node* var, ast_node* iterable, ast_node* body) {
    /* Check for shadowing: for int |i| in |i| */
    char *var_name = NULL;
    if (var->type == NODE_FOR_DECL) {
        var_name = var->data.for_decl.name;
    }

    if (var_name && iterable->type == NODE_VAR) {
        if (strcmp(var_name, iterable->data.var.name) == 0) {
             log_error(var->line, "Loop variable '%s' cannot be the same as the iterable variable.", var_name);
        }
    }

    ast_node *node = ast_new_node(NODE_FOR);
    node->data.for_loop.var = var;
    node->data.for_loop.iterable = iterable;
    node->data.for_loop.body = body;
    return node;
}
ast_node *ast_new_while(ast_node *cond, ast_node *body) {
    ast_node *node = ast_new_node(NODE_WHILE);
    node->data.while_loop.condition = cond;
    node->data.while_loop.body = body;
    return node;
}

ast_node *ast_new_next(void) {
    return ast_new_node(NODE_NEXT);
}

ast_node *ast_new_stop(void) {
    return ast_new_node(NODE_STOP);
}


ast_node *ast_new_print(ast_node *arg) {
    ast_node *node = ast_new_node(NODE_PRINT);
    node->data.print.arg = arg;
    return node;
}


ast_node *ast_new_binary(bin_op_t op, ast_node *left, ast_node *right) {
    ast_node *node = ast_new_node(NODE_BINARY);
    node->data.binary.op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;
    return node;
}
ast_node *ast_new_unary(un_op_t op, ast_node *operand) {
    ast_node *node = ast_new_node(NODE_UNARY);
    node->data.unary.op = op;
    node->data.unary.operand = operand;
    return node;
}


ast_node *ast_new_arr_lit(ast_node *elements) {
    ast_node *node = ast_new_node(NODE_ARR_LIT);
    node->data.arr_lit.elements = elements;
    return node;
}
ast_node *ast_new_arr_idx(ast_node *base, ast_node *idx) {
    ast_node *node = ast_new_node(NODE_ARR_IDX);
    node->data.arr_idx.base = base;
    node->data.arr_idx.idx = idx;
    return node;
}
ast_node *ast_new_arr_assign(ast_node *base, ast_node *idx, ast_node *value) {
    ast_node *node = ast_new_node(NODE_ARR_ASSIGN);
    node->data.arr_assign.base = base;
    node->data.arr_assign.idx = idx;
    node->data.arr_assign.value = value;
    return node;
}


ast_node *ast_new_int(int value) {
    ast_node *node = ast_new_node(NODE_INT);
    node->data.integer.value = value;
    return node;
}
ast_node *ast_new_float(double value) {
    ast_node *node = ast_new_node(NODE_FLOAT);
    node->data.float_nr.value = value;
    return node;
}
ast_node *ast_new_str(char *value) {
    ast_node *node = ast_new_node(NODE_STR);
    node->data.string.value = value;
    return node;
}
ast_node *ast_new_bool(bool value) {
    ast_node *node = ast_new_node(NODE_BOOL);
    node->data.boolean.value = value;
    return node;
}
ast_node *ast_new_var(char *name) {
    ast_node *node = ast_new_node(NODE_VAR);
    node->data.var.name = name;
    return node;
}

ast_node *ast_new_default(yafl_t *type) {
    ast_node *node = ast_new_node(NODE_DEFAULT_VAL);
    node->data.default_val.type = type;
    return node;
}


ast_node *ast_new_match(ast_node *expr, ast_node *cases) {
    ast_node *node = ast_new_node(NODE_MATCH);
    node->data.match_stmt.expr = expr;
    node->data.match_stmt.cases = cases;
    return node;
}

ast_node *ast_new_case(ast_node *expr, ast_node *body) {
    ast_node *node = ast_new_node(NODE_CASE);
    node->data.case_stmt.expr = expr;
    node->data.case_stmt.body = body;
    return node;
}

/* Append node to end of linked list */
ast_node *ast_append(ast_node *list, ast_node *node) {
    if (!list) return node;
    if (!node) return list;

    ast_node *curr = list;
    while (curr->next) {
        curr = curr->next;
    }
    curr->next = node;
    return list;
}

/* String lookups for AST printing */
const char *node_type_str(ast_node_t type) {
    switch (type) {
        case NODE_FUNC: return "FUNC";
        case NODE_PARAM: return "PARAM";
        case NODE_BLOCK: return "BLOCK";
        case NODE_DECL: return "DECL";
        case NODE_ASSIGN: return "ASSIGN";
        case NODE_RETURN: return "RETURN";
        case NODE_IF: return "IF";
        case NODE_FOR: return "FOR";
        case NODE_FOR_DECL: return "FOR_DECL";
        case NODE_FOR_VAR: return "FOR_VAR";
        case NODE_WHILE: return "WHILE";
        case NODE_NEXT: return "NEXT";
        case NODE_STOP: return "STOP";
        case NODE_PRINT: return "PRINT";
        case NODE_BINARY: return "BINARY";
        case NODE_ARR_ASSIGN: return "ARR_ASSIGN";
        case NODE_ARR_IDX: return "ARR_INDEX";
        case NODE_ARR_LIT: return "ARR_LITERAL";
        case NODE_UNARY: return "UNARY";
        case NODE_INT: return "INT";
        case NODE_FLOAT:return "FLOAT";
        case NODE_STR: return "STR";
        case NODE_BOOL: return "BOOL";
        case NODE_VAR: return "VAR";
        case NODE_CALL: return "CALL";
        case NODE_MATCH: return "MATCH";
        case NODE_CASE: return "CASE";
        default: return "UNKNOWN";
    }
}

static const char *bin_op_str(bin_op_t op) {
    switch (op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_LT: return "<";
        case OP_GT: return ">";
        case OP_LE: return "<=";
        case OP_GE: return ">=";
        case OP_EQ: return "==";
        case OP_NE: return "!=";
        case OP_AND: return "&&";
        case OP_OR: return "||";
        default: return "?";
    }
}

static const char *un_op_str(un_op_t op) {
    switch(op){
        case OP_NEG: return "-";
        case OP_NOT: return "!";
        case OP_INC: return "++";
        case OP_DEC: return "--";
        default: return "?";
    }
}

// Monotonic counter for unique id's
static int dot_node_id = 0;

/* Remove characters that can't be visualized and escape quotes */
char *ast_dot_str_escape(const char *s) {
    if (!s) return NULL;

    // Calculate required length
    size_t len = 0;
    const char *p = s;
    while (*p) {
        if (*p == '"' || *p == '\'' || *p == '\\') {
            len += 2;
        } else {
            len++;
        }
        p++;
    }

    char *ret = malloc(len + 1);
    if (!ret) return NULL;

    // Escape
    char *d = ret;
    p = s;
    while (*p) {
        if (*p == '"' || *p == '\'' || *p == '\\') {
            *d++ = '\\';
            *d++ = *p;
        } else if (*p < 32 || *p > 126) {
            *d++ = '?';
        } else {
            *d++ = *p;
        }
        p++;
    }
    *d = '\0';
    return ret;
}

/* Recursive helper to print AST to dot file */
static void ast_print_dot_node(FILE *fp, ast_node *node, int parent_id) {
    if (!node) return;

    int my_id = dot_node_id++;

    char label[4096]; // Node label based on type
    char type_buf[128]; // Buffer for type string conversion

    switch (node->type) {
        case NODE_BLOCK:
            snprintf(label, sizeof(label), "BLOCK");
            break;


        case NODE_FUNC:
            type_to_str(node->data.func.return_type, type_buf, sizeof(type_buf));
            snprintf(label, sizeof(label), "FUNC\\n%s\\n→ %s",
                     node->data.func.name,
                     type_buf);
            break;
        case NODE_PARAM:
            type_to_str(node->data.param.type, type_buf, sizeof(type_buf));
            snprintf(label, sizeof(label), "PARAM\\n%s : %s",
                     node->data.param.name,
                     type_buf);
            break;
        case NODE_RETURN:
            snprintf(label, sizeof(label), "RETURN");
            break;
        case NODE_CALL:
            snprintf(label, sizeof(label), "CALL\\n%s()", node->data.call.name);
            break;


        case NODE_DECL:
            type_to_str(node->data.decl.type, type_buf, sizeof(type_buf));
            snprintf(label, sizeof(label), "DECL\\n%s : %s",
                     node->data.decl.name,
                     type_buf);
            break;
        case NODE_ASSIGN:
            snprintf(label, sizeof(label), "ASSIGN\\n%s", node->data.assign.name);
            break;


        case NODE_IF:
            snprintf(label, sizeof(label), "IF");
            break;
        case NODE_MATCH:
            snprintf(label, sizeof(label), "MATCH");
            break;
        case NODE_CASE:
            snprintf(label, sizeof(label), (!node->data.case_stmt.expr) ? "OTHERWISE" : "CASE");
            break;


        case NODE_FOR:
            snprintf(label, sizeof(label), "FOR");
            break;
        case NODE_FOR_DECL:
            type_to_str(node->data.for_decl.type, type_buf, sizeof(type_buf));
            snprintf(label, sizeof(label), "FOR_DECL\\n%s : %s",
                     node->data.for_decl.name,
                     type_buf);
            break;
        case NODE_FOR_VAR:
            snprintf(label, sizeof(label), "FOR_VAR\\n%s",
                     node->data.for_var.name);
            break;
        case NODE_WHILE:
            snprintf(label, sizeof(label), "WHILE");
            break;
        case NODE_NEXT:
            snprintf(label, sizeof(label), "NEXT");
            break;
        case NODE_STOP:
            snprintf(label, sizeof(label), "STOP");
            break;


        case NODE_PRINT:
            snprintf(label, sizeof(label), "PRINT");
            break;


        case NODE_BINARY:
            snprintf(label, sizeof(label), "BINOP\\n%s", bin_op_str(node->data.binary.op));
            break;
        case NODE_UNARY:
            snprintf(label, sizeof(label), "UNOP\\n%s", un_op_str(node->data.unary.op));
            break;


        case NODE_ARR_ASSIGN:
            snprintf(label, sizeof(label), "ARR_ASSIGN");
            break;
        case NODE_ARR_IDX:
            snprintf(label, sizeof(label), "ARR_INDEX");
            break;
        case NODE_ARR_LIT:
            snprintf(label, sizeof(label), "ARR_LITERAL\\n");
            break;



        case NODE_INT:
            snprintf(label, sizeof(label), "INT\\n%d", node->data.integer.value);
            break;
        case NODE_FLOAT:
            snprintf(label, sizeof(label), "FLOAT\\n%f", node->data.float_nr.value);
            break;
        case NODE_STR:
            // Escape special chars in str
            char *sanitized = ast_dot_str_escape(node->data.string.value);
            snprintf(label, sizeof(label), "STR\\n%s", sanitized);
            free(sanitized);
            break;
        case NODE_BOOL:
            snprintf(label, sizeof(label), "BOOL\\n%s", node->data.boolean.value ? "Yes" : "No");
            break;
        case NODE_VAR:
            snprintf(label, sizeof(label), "VAR\\n%s", node->data.var.name);
            break;
        case NODE_DEFAULT_VAL:
            type_to_str(node->data.default_val.type, type_buf, sizeof(type_buf));
            snprintf(label, sizeof(label), "OTHERWISE\\n%s", type_buf);
            break;
        default:
            snprintf(label, sizeof(label), "%s", node_type_str(node->type));
            break;
    }

    // Print node
    fprintf(fp, "  node%d [label=\"%s\"];\n", my_id, label);

    // Connect to parent
    if (parent_id >= 0) {
        fprintf(fp, "  node%d -> node%d;\n", parent_id, my_id);
    }

    // Process children if needed
    switch (node->type) {
        case NODE_BLOCK:
            if (node->data.block.stmts) {
                ast_print_dot_node(fp, node->data.block.stmts, my_id);
            }
            break;


        case NODE_FUNC:
            if (node->data.func.params) {
                ast_print_dot_node(fp, node->data.func.params, my_id);
            }
            if (node->data.func.body) {
                ast_print_dot_node(fp, node->data.func.body, my_id);
            }
            break;
        case NODE_RETURN:
            if (node->data.ret.value) {
                ast_print_dot_node(fp, node->data.ret.value, my_id);
            }
            break;
        case NODE_CALL:
            if(node->data.call.args) {
                ast_print_dot_node(fp, node->data.call.args, my_id);
            }
            break;


        case NODE_DECL:
            if (node->data.decl.init) {
                ast_print_dot_node(fp, node->data.decl.init, my_id);
            }
            break;
        case NODE_ASSIGN:
            if (node->data.assign.value) {
                ast_print_dot_node(fp, node->data.assign.value, my_id);
            }
            break;


        case NODE_IF:
            if (node->data.if_stmt.condition) {
                ast_print_dot_node(fp, node->data.if_stmt.condition, my_id);
            }
            if (node->data.if_stmt.then_block) {
                ast_print_dot_node(fp, node->data.if_stmt.then_block, my_id);
            }
            if (node->data.if_stmt.else_block) {
                ast_print_dot_node(fp, node->data.if_stmt.else_block, my_id);
            }
            break;
        case NODE_MATCH:
            if (node->data.match_stmt.expr) {
                ast_print_dot_node(fp, node->data.match_stmt.expr, my_id);
            }
            if (node->data.match_stmt.cases) {
                ast_print_dot_node(fp, node->data.match_stmt.cases, my_id);
            }
            break;
        case NODE_CASE:
            if (node->data.case_stmt.expr) {
                ast_print_dot_node(fp, node->data.case_stmt.expr, my_id);
            }
            if (node->data.case_stmt.body) {
                ast_print_dot_node(fp, node->data.case_stmt.body, my_id);
            }
            break;


        case NODE_FOR:
            if (node->data.for_loop.var) {
                ast_print_dot_node(fp, node->data.for_loop.var, my_id);
            }
            if (node->data.for_loop.iterable) {
                ast_print_dot_node(fp, node->data.for_loop.iterable, my_id);
            }
            if (node->data.for_loop.body) {
                ast_print_dot_node(fp, node->data.for_loop.body, my_id);
            }
            break;
        case NODE_WHILE:
            if (node->data.while_loop.condition) {
                ast_print_dot_node(fp, node->data.while_loop.condition, my_id);
            }
            if (node->data.while_loop.body) {
                ast_print_dot_node(fp, node->data.while_loop.body, my_id);
            }
            break;


        case NODE_PRINT:
            if (node->data.print.arg) {
                ast_print_dot_node(fp, node->data.print.arg, my_id);
            }
            break;


        case NODE_BINARY:
            if (node->data.binary.left) {
                ast_print_dot_node(fp, node->data.binary.left, my_id);
            }
            if (node->data.binary.right) {
                ast_print_dot_node(fp, node->data.binary.right, my_id);
            }
            break;
        case NODE_UNARY:
            if (node->data.unary.operand) {
                ast_print_dot_node(fp, node->data.unary.operand, my_id);
            }
            break;

        case NODE_ARR_ASSIGN:
            if (node->data.arr_assign.base) {
                ast_print_dot_node(fp, node->data.arr_assign.base, my_id);
            }
            if (node->data.arr_assign.idx) {
                ast_print_dot_node(fp, node->data.arr_assign.idx, my_id);
            }
            if (node->data.arr_assign.value) {
                ast_print_dot_node(fp, node->data.arr_assign.value, my_id);
            }
            break;
        case NODE_ARR_IDX:
            if (node->data.arr_idx.base) {
                ast_print_dot_node(fp, node->data.arr_idx.base, my_id);
            }
            if (node->data.arr_idx.idx) {
                ast_print_dot_node(fp, node->data.arr_idx.idx, my_id);
            }
            break;
        case NODE_ARR_LIT:
            if (node->data.arr_lit.elements) {
                ast_print_dot_node(fp, node->data.arr_lit.elements, my_id);
            }
            break;

        default:
            break;
    }

    // Process siblings (linked list)
    if (node->next) {
        ast_print_dot_node(fp, node->next, parent_id);
    }
}

/* Print the ast as a dot file */
void ast_print_dot(ast_node *node, const char *basename) {
    char dot_filename[256];
    snprintf(dot_filename, sizeof(dot_filename), "%s.dot", basename);

    FILE *fp = fopen(dot_filename, "w");
    if (!fp) {
        log_error(NO_LINE, "Error: Could not open \'%s\' for writing\n", dot_filename);
    }

    fprintf(fp, "digraph AST {\n");
    fprintf(fp, "  node [shape=box, style=rounded];\n");
    fprintf(fp, "  rankdir=TB;\n");

    dot_node_id = 0;
    ast_print_dot_node(fp, node, -1);

    fprintf(fp, "}\n");
    fclose(fp);

    log_debug(NO_LINE, "AST written to %s", dot_filename);

    // Convert to svg automatically
    char cmd[1024];
    char svg_filename[256];
    snprintf(svg_filename, sizeof(svg_filename), "%s.svg", basename);

    snprintf(cmd, sizeof(cmd), "dot -Tsvg %s -o %s", dot_filename, svg_filename);
    log_debug(NO_LINE, "Generating SVG: %s", cmd);
    if (system(cmd) != 0) {
        log_error(NO_LINE, "Error generating SVG. Is 'graphviz' installed?\n");
    } else {
        log_debug(NO_LINE, "SVG generated at %s", svg_filename);
    }
}

/* Frees the AST recursively so valgrind doesn't complain ;-) */
void ast_free(ast_node *node) {
    if (!node) return;

    ast_node *next = node->next;

    // Free node-specific data (strings)
    switch (node->type) {
        case NODE_BLOCK:
            ast_free(node->data.block.stmts);
            break;

        case NODE_FUNC:
            free(node->data.func.name);
            type_free(node->data.func.return_type);
            ast_free(node->data.func.params);
            ast_free(node->data.func.body);
            break;
        case NODE_PARAM:
            free(node->data.param.name);
            type_free(node->data.param.type);
            ast_free(node->data.param.default_value);
            break;
        case NODE_RETURN:
            ast_free(node->data.ret.value);
            break;
                case NODE_CALL:
                    free(node->data.call.name);
                    ast_free(node->data.call.args);
                    break;
        
                case NODE_DEFAULT_VAL:
                    type_free(node->data.default_val.type);
                    break;

        case NODE_DECL:
            free(node->data.decl.name);
            type_free(node->data.decl.type);
            ast_free(node->data.decl.init);
            break;
        case NODE_ASSIGN:
            free(node->data.assign.name);
            ast_free(node->data.assign.value);
            break;


        case NODE_IF:
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_block);
            ast_free(node->data.if_stmt.else_block);
            break;
        case NODE_MATCH:
            ast_free(node->data.match_stmt.expr);
            ast_free(node->data.match_stmt.cases);
            break;
        case NODE_CASE:
            ast_free(node->data.case_stmt.expr);
            ast_free(node->data.case_stmt.body);
            break;

        case NODE_FOR:
            ast_free(node->data.for_loop.var);
            ast_free(node->data.for_loop.iterable);
            ast_free(node->data.for_loop.body);
            break;
        case NODE_FOR_DECL:
            free(node->data.for_decl.name);
            type_free(node->data.for_decl.type);
            break;
        case NODE_FOR_VAR:
            free(node->data.for_var.name);
            break;
        case NODE_WHILE:
            ast_free(node->data.while_loop.condition);
            ast_free(node->data.while_loop.body);
            break;

        case NODE_PRINT:
            ast_free(node->data.print.arg);
            break;


        case NODE_BINARY:
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;
        case NODE_UNARY:
            ast_free(node->data.unary.operand);
            break;


        case NODE_ARR_ASSIGN:
            ast_free(node->data.arr_assign.base);
            ast_free(node->data.arr_assign.idx);
            ast_free(node->data.arr_assign.value);
            break;
        case NODE_ARR_IDX:
            ast_free(node->data.arr_idx.base);
            ast_free(node->data.arr_idx.idx);
            break;
        case NODE_ARR_LIT:
            ast_free(node->data.arr_lit.elements);
            break;

        case NODE_STR:
            free(node->data.string.value);
            break;
        case NODE_VAR:
            free(node->data.var.name);
            break;

        // No dynamic memory
        case NODE_NEXT:
        case NODE_STOP:
        case NODE_INT:
        case NODE_FLOAT:
        case NODE_BOOL:
        default:
            break;
    }

    free(node);
    // Free siblings, if present
    ast_free(next);
}
