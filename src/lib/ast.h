#ifndef AST_H
#define AST_H

#include <stdint.h>
#include "types.h"

typedef enum {
    /* Top Level */
    NODE_INT_LIT,
} NodeKind;

typedef struct ASTNode {
    NodeKind kind;
    /* For linking lists (stmts, params, args) */
    struct ASTNode* next;

    union {
        /* fn name(...) --> type { body } */
        struct {
            char* name;
            /* Points to first param */
            struct ASTNode* params;
            struct ASTNode* body;
            BaseType ret_type;
        } func;

        /* name: type */
        struct {
            char* name;
            BaseType type;
        } param;

        /* { stmts } */
        struct {
            /* Points to first stmt */
            struct ASTNode* stmts;
        } block;

        /* type |x| -> expr */
        struct {
            char* name;
            BaseType type;
            struct ASTNode* expr;
        } var_decl;

        /* |x| -> expr */
        struct {
            char* name;
            struct ASTNode* expr;
        } assign;

        /* if (cond) then_block else else_block */
        struct {
            struct ASTNode* cond;
            struct ASTNode* then_block;
            struct ASTNode* else_block;
        } if_stmt;

        /* while (cond) body */
        struct {
            struct ASTNode* cond;
            struct ASTNode* body;
        } while_stmt;

        /* print("fmt", args...) */
        struct {
            char* fmt;
            /* Points to first arg */
            struct ASTNode* args;
        } print_stmt;

        /* L op R */
        struct {
            int op;
            struct ASTNode* left;
            struct ASTNode* right;
        } bin_op;

        /* op operand */
        struct {
            int op;
            struct ASTNode* operand;
        } unary_op;

        /* call name(args) */
        struct {
            char* name;
            /* Points to first arg */
            struct ASTNode* args;
        } call;

        /* for int |i| in range(start, end, step) { body } */
        struct {
            char* var_name;
            struct ASTNode* start;
            struct ASTNode* end;
            struct ASTNode* step;
            struct ASTNode* body;
        } for_stmt;

        /* ret expression; */
        struct {
            struct ASTNode* expr;   /* The value being returned */
        } ret_stmt;

        uint64_t nr;
        char* str;
    } data;

} ASTNode;

/* Constructors */
ASTNode* ast_new_node(NodeKind kind);
ASTNode* ast_new_int(uint64_t val);
ASTNode* ast_new_var(char* name);
ASTNode* ast_new_str(char* str);
ASTNode* ast_new_binary(int op, ASTNode* left, ASTNode* right);
ASTNode* ast_new_unary(int op, ASTNode* operand);
ASTNode* ast_new_decl(BaseType type, char* name, ASTNode* expr);
ASTNode* ast_new_assign(char* name, ASTNode* expr);
ASTNode* ast_new_if(ASTNode* cond, ASTNode* then_block, ASTNode* else_block);
ASTNode* ast_new_call(char* name, ASTNode* args);
ASTNode* ast_new_func(char* name, ASTNode* params, BaseType ret, ASTNode* body);
ASTNode* ast_new_return(ASTNode* expr);
ASTNode* ast_new_for(char* var, ASTNode* start, ASTNode* end, ASTNode* step, ASTNode* body);
ASTNode* ast_new_while(ASTNode* cond, ASTNode* body);
ASTNode* ast_new_print(char* fmt, ASTNode* args);

/* Helper to link lists */
ASTNode* ast_append(ASTNode* list, ASTNode* new_node);

void ast_print_dot(ASTNode* root, FILE* fp);

#endif
