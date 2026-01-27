#include "ast.h"
#include "logger.h"
#include "optim.h"
#include "types.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool changed = false;

/* --- Helpers --- */

static bool is_constant(ast_node *node) {
    if (!node) return false;
    return (node->type == NODE_INT || node->type == NODE_FLOAT ||
            node->type == NODE_BOOL || node->type == NODE_STR);
}

static bool nodes_are_equal(ast_node *a, ast_node *b) {
    if (!a || !b) return false;
    if (a->type != b->type) return false;

    switch (a->type) {
        case NODE_INT: return a->data.integer.value == b->data.integer.value;
        case NODE_FLOAT: return a->data.float_nr.value == b->data.float_nr.value;
        case NODE_BOOL: return a->data.boolean.value == b->data.boolean.value;
        case NODE_STR: return strcmp(a->data.string.value, b->data.string.value) == 0;
        default: return false;
    }
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
        int lv = l->data.integer.value;
        int rv = r->data.integer.value;

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
    // Fold String Concatenation
    else if (l->type == NODE_STR && r->type == NODE_STR && node->data.binary.op == OP_ADD) {
        char *lv = l->data.string.value;
        char *rv = r->data.string.value;
        size_t len = strlen(lv) + strlen(rv) + 1;
        char *new_str = malloc(len);
        strcpy(new_str, lv);
        strcat(new_str, rv);
        res = ast_new_str(new_str);
    }
    // Fold String Repetition (Str * Int)
    else if (l->type == NODE_STR && r->type == NODE_INT && node->data.binary.op == OP_MUL) {
        char *lv = l->data.string.value;
        int count = (int)r->data.integer.value;
        if (count < 0) count = 0;
        size_t slen = strlen(lv);
        size_t len = (size_t)slen * count + 1;
        char *new_str = calloc(1, len);
        for (int i = 0; i < count; i++) {
            strcat(new_str, lv);
        }
        res = ast_new_str(new_str);
    }

    if (res) {
        changed = true;
        res->line = node->line;
        res->next = node->next;
        node->next = NULL;
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
        node->next = NULL;
        ast_free(node);
        return res;
    }
    return node;
}

/* --- Traversal --- */

static ast_node *traverse(ast_node *node) {
    if (!node) return NULL;

    // IMPORTANT: Process the NEXT node in the chain FIRST.
    // This ensures we have a stable pointer to the rest of the list
    // before we potentially free or replace the CURRENT node.
    if (node->next) {
        node->next = traverse(node->next);
    }

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
                    node->data.if_stmt.then_block = NULL;
                } else {
                    replacement = node->data.if_stmt.else_block;
                    node->data.if_stmt.else_block = NULL;
                }

                ast_node *chain = node->next;
                node->next = NULL;
                ast_free(node);

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

        case NODE_MATCH:
            node->data.match_stmt.expr = traverse(node->data.match_stmt.expr);
            node->data.match_stmt.cases = traverse(node->data.match_stmt.cases);

            // DCE: If expr is constant, pick the right case
            if (is_constant(node->data.match_stmt.expr)) {
                ast_node *match_val = node->data.match_stmt.expr;
                ast_node *curr_case = node->data.match_stmt.cases;
                ast_node *selected_body = NULL;
                bool found = false;
                ast_node *default_body = NULL;

                while (curr_case) {
                    if (curr_case->type == NODE_CASE) {
                        ast_node *case_expr = curr_case->data.case_stmt.expr;
                        // Default case
                        if (!case_expr) {
                            default_body = curr_case;
                        }
                        // Constant Match
                        else if (nodes_are_equal(match_val, case_expr)) {
                            // Search for next body if fallthrough
                            while(curr_case && !curr_case->data.case_stmt.body)
                                curr_case = curr_case->next;
                            if(curr_case) {
                                selected_body = curr_case->data.case_stmt.body;
                                curr_case->data.case_stmt.body = NULL;
                                found = true;
                            }
                            break;
                        }
                    }
                    curr_case = curr_case->next;
                }

                if (!found && default_body) {
                    selected_body = default_body->data.case_stmt.body;
                    default_body->data.case_stmt.body = NULL;
                    found = true;
                }

                ast_node *chain = node->next;
                node->next = NULL;
                ast_free(node);

                if (found && selected_body) {
                    ast_append(selected_body, chain);
                    changed = true;
                    return selected_body;
                } else {
                    // Case found but no body
                    // No match, no default -> remove entire match
                    changed = true;
                    return chain;
                }
            }
            break;

        case NODE_CASE:
            // expr may not yet be a literal
            if (node->data.case_stmt.expr)
                node->data.case_stmt.expr = traverse(node->data.case_stmt.expr);
            if (node->data.case_stmt.body)
                node->data.case_stmt.body = traverse(node->data.case_stmt.body);
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

            // DCE: Constant false condition
            if (node->data.while_loop.condition->type == NODE_BOOL) {
                if (node->data.while_loop.condition->data.boolean.value == false) {
                    ast_node *chain = node->next;
                    node->next = NULL;
                    ast_free(node);
                    changed = true;
                    return chain;
                }
            }
            break;

        case NODE_FOR:
            node->data.for_loop.iterable = traverse(node->data.for_loop.iterable);
            node->data.for_loop.body = traverse(node->data.for_loop.body);
            break;

        case NODE_FOR_DECL:
        case NODE_FOR_VAR:
        case NODE_PARAM:
            // Nothing to optimize here
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

    log_info(NO_LINE, "Optimization completed in %d passes", passes);
}
