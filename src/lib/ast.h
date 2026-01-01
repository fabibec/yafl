#ifndef _AST_H_
#define _AST_H_
/* Abstract Syntax Tree */

#include <stdint.h>
#include <stdio.h>
#include "arith.h"
#include "types.h"
#include "stdbool.h"

/* Node types */
typedef enum {
    NODE_PROGRAM,
    NODE_BLOCK,

    NODE_FUNC,
    NODE_PARAM,
    NODE_RETURN,
    NODE_CALL,

    NODE_DECL,
    NODE_ASSIGN,

    NODE_IF,

    NODE_FOR,
    NODE_FOR_DECL,
    NODE_FOR_VAR,
    NODE_WHILE,

    NODE_PRINT,

    NODE_BINARY,
    NODE_UNARY,

    NODE_ARR_LIT,
    NODE_ARR_FILL,
    NODE_ARR_ASSIGN,
    NODE_ARR_IDX,

    NODE_INT,
    NODE_FLOAT,
    NODE_STR,
    NODE_BOOL,
    NODE_VAR,
    NODE_CAST,
    NODE_DEFAULT
} ast_node_t;

typedef struct ast_node {
    ast_node_t type;
    /* Line for errors (e.g. types)*/
    int line;
    /* For linked lists */
    struct ast_node *next;

    /* Data container */
    union {
        struct {
            // linked list of statements
            struct ast_node *stmts;
        } block;


        struct {
            char *name;
            // linked list of parameters decls
            struct ast_node *params;
            yafl_t *return_type;
            // linked list of statements
            struct ast_node *body;
        } func;
        struct {
            yafl_t *type;
            char *name;
            struct ast_node *default_value;
        } param;
        struct {
            struct ast_node *value;
        } ret;
        struct {
            char *name;
            // linked list of args
            struct ast_node *args;
        } call;

        struct {
            yafl_t *type;
            char *name;
            struct ast_node *init;
        } decl;
        struct {
            char *name;
            struct ast_node *value;
        } assign;


        struct {
            struct ast_node *condition;
            struct ast_node *then_block;
            struct ast_node *else_block;
        } if_stmt;
        struct {
            // loop variable
            struct ast_node *var;
            // iterable expression (range(...), array, string)
            struct ast_node *iterable;

            struct ast_node *body;
        } for_loop;
        struct{
            yafl_t *type;
            char *name;
        } for_decl;
        struct {
            char *name;
        } for_var;
        struct {
            struct ast_node *condition;
            struct ast_node *body;
        } while_loop;

        struct {
            // Currently only print one string
            struct ast_node *arg;
        } print;

        struct {
            bin_op_t op;
            struct ast_node *left;
            struct ast_node *right;
        } binary;
        struct {
            un_op_t op;
            struct ast_node *operand;
        } unary;


        struct {
            struct ast_node * elements;
        } arr_lit;
        struct {
            struct ast_node *elements;
            struct ast_node *count;
        } arr_fill;
        struct {
            struct ast_node *base;
            struct ast_node *idx;
        } arr_idx;
        struct {
            struct ast_node *base;
            struct ast_node *idx;
            struct ast_node * value;
        } arr_assign;


        struct {
            uint64_t value;
        } integer;
        struct {
            double value;
        } float_nr;
        struct {
            char *value;
        } string;
        struct {
            bool value;
        } boolean;
        struct {
            char *name;
        } var;
        struct {
            yafl_t *type;
            struct ast_node *expr;
        } cast;
        struct {
            yafl_t *type;
        } default_val;
    } data;

} ast_node;

/* AST Node Constructors */
ast_node* ast_new_node(ast_node_t kind);

ast_node* ast_new_func(char* name, ast_node* params, yafl_t *return_type, ast_node* body);
ast_node* ast_new_param(char * name, yafl_t *type, ast_node *default_val);
ast_node* ast_new_ret(ast_node* value);
ast_node* ast_new_call(char* name, ast_node* args);

ast_node *ast_new_decl(yafl_t *type, char* name, ast_node* init);
ast_node *ast_new_assign(char* name, ast_node* value);

ast_node *ast_new_if(ast_node* condition, ast_node* then_block, ast_node* else_block);

ast_node *ast_new_for(ast_node* var, ast_node* iterable, ast_node* body);
ast_node *ast_new_while(ast_node* cond, ast_node* body);

ast_node *ast_new_print(ast_node* arg);

ast_node *ast_new_binary(bin_op_t op, ast_node* left, ast_node* right);
ast_node *ast_new_unary(un_op_t op, ast_node* operand);


ast_node *ast_new_arr_lit(ast_node *elements);
ast_node *ast_new_arr_fill(ast_node *elements, ast_node *count);
ast_node *ast_new_arr_idx(ast_node *base, ast_node *idx);
ast_node *ast_new_arr_assign(ast_node *base, ast_node *idx, ast_node *value);


ast_node *ast_new_int(uint64_t value);
ast_node *ast_new_float(double value);
ast_node *ast_new_str(char* value);
ast_node *ast_new_bool(bool value);
ast_node *ast_new_var(char* name);
ast_node *ast_new_cast(yafl_t *type, ast_node *expr);
ast_node *ast_new_default(yafl_t *type);

/* Helper to link lists */
ast_node *ast_append(ast_node* list, ast_node* new_node);
/* List operations */
ast_node *ast_append(ast_node *list, ast_node *node);

/* Utilities */
void ast_print_dot(ast_node *node, const char *filename);
void ast_free(ast_node *node);

#endif // _AST_H_
