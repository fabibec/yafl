#include "optim.h"
#include "ast.h"
#include "types.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static bool changed = false;

/* --- Helpers --- */

static bool is_constant(ast_node *node) {
    if (!node) return false;
    return (node->type == NODE_INT || node->type == NODE_FLOAT ||
            node->type == NODE_BOOL || node->type == NODE_STR);
}

/* --- Constant Folding --- */

static ast_node *fold_binary(ast_node *node) {
    if (!is_constant(node->data.binary.left) || !is_constant(node->data.binary.right)) {
        return node;
    }

    ast_node *l = node->data.binary.left;
    ast_node *r = node->data.binary.right;
    ast_node *res = NULL;

    // Fold Arithmetic (INT)
    if (l->type == NODE_INT && r->type == NODE_INT) {
        uint64_t lv = l->data.integer.value;
        uint64_t rv = r->data.integer.value;

        switch (node->data.binary.op) {
            case OP_ADD: res = ast_new_int(lv + rv); break;
            case OP_SUB: res = ast_new_int(lv - rv); break;
            case OP_MUL: res = ast_new_int(lv * rv); break;
            case OP_DIV:
                if (rv != 0) res = ast_new_int(lv / rv);
                else log_error(node->line, "Division by zero in constant folding");
                break;
            case OP_MOD:
                if (rv != 0) res = ast_new_int(lv % rv);
                else log_error(node->line, "Modulo by zero in constant folding");
                break;
            case OP_LT: res = ast_new_bool(lv < rv); break;
            case OP_GT: res = ast_new_bool(lv > rv); break;
            case OP_LE: res = ast_new_bool(lv <= rv); break;
            case OP_GE: res = ast_new_bool(lv >= rv); break;
            case OP_EQ: res = ast_new_bool(lv == rv); break;
            case OP_NE: res = ast_new_bool(lv != rv); break;
            default: break;
        }
    }
    // Fold Arithmetic (FLOAT)
    else if (l->type == NODE_FLOAT && r->type == NODE_FLOAT) {
        double lv = l->data.float_nr.value;
        double rv = r->data.float_nr.value;

        switch (node->data.binary.op) {
            case OP_ADD: res = ast_new_float(lv + rv); break;
            case OP_SUB: res = ast_new_float(lv - rv); break;
            case OP_MUL: res = ast_new_float(lv * rv); break;
            case OP_DIV: res = ast_new_float(lv / rv); break;
            case OP_LT: res = ast_new_bool(lv < rv); break;
            case OP_GT: res = ast_new_bool(lv > rv); break;
            case OP_LE: res = ast_new_bool(lv <= rv); break;
            case OP_GE: res = ast_new_bool(lv >= rv); break;
            case OP_EQ: res = ast_new_bool(lv == rv); break;
            case OP_NE: res = ast_new_bool(lv != rv); break;
            default: break;
        }
    }
    // Fold Arithmetic (BOOL)
    else if (l->type == NODE_BOOL && r->type == NODE_BOOL) {
        bool lv = l->data.boolean.value;
        bool rv = r->data.boolean.value;

        switch (node->data.binary.op) {
            case OP_AND: res = ast_new_bool(lv && rv); break;
            case OP_OR:  res = ast_new_bool(lv || rv); break;
            case OP_EQ:  res = ast_new_bool(lv == rv); break;
            case OP_NE:  res = ast_new_bool(lv != rv); break;
            default: break;
        }
    }

    if (res) {
        changed = true;
        res->line = node->line;
        res->next = node->next;
        ast_free(node);
        return res;
    }
    return node;
}

static ast_node *fold_unary(ast_node *node) {
    if (!is_constant(node->data.unary.operand)) return node;

    ast_node *op = node->data.unary.operand;
    ast_node *res = NULL;

    switch(op->type) {
        case NODE_INT:
            if (node->data.unary.op == OP_NEG) res = ast_new_int(-op->data.integer.value);
            break;
        case NODE_FLOAT:
            if (node->data.unary.op == OP_NEG) res = ast_new_float(-op->data.float_nr.value);
            break;
        case NODE_BOOL:
            if (node->data.unary.op == OP_NOT) res = ast_new_bool(!op->data.boolean.value);
            break;
        default:
    }

    if (res) {
        changed = true;
        res->line = node->line;
        res->next = node->next;
        ast_free(node);
        return res;
    }
    return node;
}

/* --- Traversal --- */

static ast_node *traverse(ast_node *node) {
    if (!node) return NULL;

    // Process children first (bottom-up)
    if (node->next) node->next = traverse(node->next);

    switch (node->type) {
        case NODE_BINARY:
            node->data.binary.left = traverse(node->data.binary.left);
            node->data.binary.right = traverse(node->data.binary.right);
            node = fold_binary(node);
            break;
        case NODE_UNARY:
            node->data.unary.operand = traverse(node->data.unary.operand);
            node = fold_unary(node);
            break;
        case NODE_IF:
            node->data.if_stmt.condition = traverse(node->data.if_stmt.condition);
            node->data.if_stmt.then_block = traverse(node->data.if_stmt.then_block);
            node->data.if_stmt.else_block = traverse(node->data.if_stmt.else_block);

            // DCE: If condition is constant
            if (node->data.if_stmt.condition->type == NODE_BOOL) {
                bool cond = node->data.if_stmt.condition->data.boolean.value;
                ast_node *replacement;
                if (cond) {
                    replacement = node->data.if_stmt.then_block;
                } else {
                    replacement = node->data.if_stmt.else_block;
                }

                ast_node *chain = node->next;
                ast_free(node->data.if_stmt.condition);
                // Free the unused branch
                if (cond) ast_free(node->data.if_stmt.else_block);
                else ast_free(node->data.if_stmt.then_block);
                free(node);

                if (replacement) {
                    ast_append(replacement, chain);
                    changed = true;
                    return replacement;
                } else {
                    changed = true;
                    return chain;
                }
            }
            break;

        case NODE_BLOCK:
            node->data.block.stmts = traverse(node->data.block.stmts);

            // DCE: Remove dead code after Return/Stop/Next
            {
                ast_node *curr = node->data.block.stmts;
                while (curr) {
                    if (curr->type == NODE_RETURN || curr->type == NODE_STOP || curr->type == NODE_NEXT) {
                        if (curr->next) {
                            ast_free(curr->next);
                            curr->next = NULL;
                            changed = true;
                        }
                    }
                    curr = curr->next;
                }
            }
            break;

        case NODE_FUNC:
            node->data.func.body = traverse(node->data.func.body);
            break;

        case NODE_CALL:
            node->data.call.args = traverse(node->data.call.args);
            break;

        case NODE_PRINT:
            node->data.print.arg = traverse(node->data.print.arg);
            break;

        case NODE_RETURN:
            node->data.ret.value = traverse(node->data.ret.value);
            break;

        case NODE_DECL:
            node->data.decl.init = traverse(node->data.decl.init);
            break;

        case NODE_ASSIGN:
            node->data.assign.value = traverse(node->data.assign.value);
            break;

        case NODE_WHILE:
            node->data.while_loop.condition = traverse(node->data.while_loop.condition);
            node->data.while_loop.body = traverse(node->data.while_loop.body);
            break;

        case NODE_FOR:
            node->data.for_loop.iterable = traverse(node->data.for_loop.iterable);
            node->data.for_loop.body = traverse(node->data.for_loop.body);
            break;

        default:
            break;
    }

    return node;
}

void optimize(ast_node *root) {
    int passes = 0;

    do {
        changed = false;
        root = traverse(root);
        passes++;
    } while (changed && passes < 10);

    log_info(0, "Optimization completed in %d passes", passes);
}
