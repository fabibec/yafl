#include "type_checking.h"
#include "logger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern symtab *prog_symtab;

void type_check_compatibility(yafl_t *expected, yafl_t *actual, int line, const char *context) {
    if (!expected || !actual) return;
    if (!type_equals(expected, actual)) {
        char s_exp[128], s_act[128];
        type_to_str(expected, s_exp, sizeof(s_exp));
        type_to_str(actual, s_act, sizeof(s_act));
        log_error(line, "Type mismatch in %s: expected '%s', got '%s'", context, s_exp, s_act);
    }
}

void type_check_func_signature(ast_node *node) {
    if (!node || node->type != NODE_FUNC) return;

    ast_node *param = node->data.func.params;
    bool seen_default = false;

    while (param) {
        if (param->data.param.default_value) {
            seen_default = true;
            yafl_t *def_t = type_check_expr(param->data.param.default_value);
            type_check_compatibility(param->data.param.type, def_t, param->line, "default argument");
            type_free(def_t);
        } else if (seen_default) {
            log_error(param->line, "No default argument provided for '%s' (it follows one that has a default)",
                        param->data.param.name);
        }
        param = param->next;
    }
}

yafl_t* type_check_expr(ast_node *node) {
    if (!node) return NULL;

    switch (node->type) {
        case NODE_INT:
            return type_new_simple(TYPE_SINT);
        case NODE_FLOAT:
            return type_new_simple(TYPE_FLOAT);
        case NODE_BOOL:
            return type_new_simple(TYPE_BOOL);
        case NODE_STR:
            return type_new_simple(TYPE_STR);
        case NODE_VAR: {
            var_sym *var = symtab_lookup_var(prog_symtab, node->data.var.name);
            if (!var) {
                log_error(node->line, "Variable '%s' not declared", node->data.var.name);
            }
            return type_clone(var->type);
        }

        case NODE_BINARY: {
            yafl_t *left = type_check_expr(node->data.binary.left);
            yafl_t *right = type_check_expr(node->data.binary.right);
            yafl_t *res = NULL;

            if (!left || !right) {
                if (left) type_free(left);
                if (right) type_free(right);
                return NULL;
            }

            bool types_match = type_is_identical(left, right);
            bool is_mul_str_int = (node->data.binary.op == OP_MUL) && (left->base_t == TYPE_STR && right->base_t == TYPE_SINT);
            bool is_mul_arr_int = (node->data.binary.op == OP_MUL) && (left->base_t == TYPE_ARR && right->base_t == TYPE_SINT);

            if (!types_match && !is_mul_str_int && !is_mul_arr_int) {
                char l_str[64], r_str[64];
                type_to_str(left, l_str, 64);
                type_to_str(right, r_str, 64);
                log_error(node->line, "Binary operands must be of the same type (got %s and %s)", l_str, r_str);
            }

            switch (node->data.binary.op){
                case OP_ADD:
                    if (left->base_t == TYPE_STR || left->base_t == TYPE_FLOAT ||
                        left->base_t == TYPE_SINT || left->base_t == TYPE_ARR) {
                        res = type_clone(left);
                    } else {
                        log_error(node->line, "Invalid operand type for +");
                    }
                    break;
                case OP_MUL:
                    if (is_mul_str_int) {
                        res = type_new_simple(TYPE_STR);
                    } else if (is_mul_arr_int) {
                        res = type_clone(left);
                    } else if (left->base_t == TYPE_FLOAT || left->base_t == TYPE_SINT) {
                        res = type_clone(left);
                    } else {
                        log_error(node->line, "Invalid operand type for arithmetic operator");
                    }
                    break;
                case OP_SUB:
                case OP_DIV:
                case OP_MOD:
                    if (left->base_t == TYPE_FLOAT || left->base_t == TYPE_SINT) {
                        res = type_clone(left);
                    } else {
                        log_error(node->line, "Invalid operand type for arithmetic operator");
                    }
                    break;
                case OP_LT:
                case OP_GT:
                case OP_LE:
                case OP_GE:
                case OP_EQ:
                case OP_NE:
                    if (left->base_t == TYPE_SINT ||
                         left->base_t == TYPE_FLOAT || left->base_t == TYPE_STR || left->base_t == TYPE_BOOL) {
                         res = type_new_simple(TYPE_BOOL);
                     } else {
                         log_error(node->line, "Invalid operand type for comparison");
                     }
                    break;
                case OP_AND:
                case OP_OR:
                    if (left->base_t == TYPE_BOOL) {
                        res = type_new_simple(TYPE_BOOL);
                    } else {
                        log_error(node->line, "Logical operators require boolean operands");
                    }
                    break;
                default:
                    log_error(node->line, "Unknown binary operator");
            }
            type_free(left);
            type_free(right);
            return res;
        }

        case NODE_CALL: {
            // Need to evaluate arg types to find correct overload
            int arg_count = 0;
            ast_node *args = node->data.call.args;
            for (ast_node *p = args; p; p = p->next) arg_count++;

            yafl_t **arg_types = NULL;
            if (arg_count > 0) {
                arg_types = malloc(arg_count * sizeof(yafl_t*));
                int i = 0;
                for (ast_node *p = args; p; p = p->next) {
                    arg_types[i++] = type_check_expr(p);
                }
            }

            func_sym *fn = symtab_lookup_func(prog_symtab, node->data.call.name, arg_types, arg_count);

            if (!fn) {
                char arg_desc[512] = "";
                if (arg_count > 0 && arg_types) {
                    char type_buf[128];
                    for(int k=0; k<arg_count; k++) {
                        type_to_str(arg_types[k], type_buf, sizeof(type_buf));
                        strncat(arg_desc, type_buf, sizeof(arg_desc) - strlen(arg_desc) - 1);
                        if (k < arg_count - 1) strncat(arg_desc, ", ", sizeof(arg_desc) - strlen(arg_desc) - 1);
                    }
                } else {
                    strcpy(arg_desc, "none");
                }
                log_error(node->line, "Function '%s' not declared or no matching overload found (provided args: %s)",
                            node->data.call.name, arg_desc);
            }

            yafl_t *ret_t = type_clone(fn->ret_type);

            // Type inference for generic builtins
            if (arg_count > 0) {
                yafl_t *first_arg_t = arg_types[0];
                if (ret_t->base_t == TYPE_GENERIC) {
                    type_free(ret_t);
                    ret_t = type_clone(first_arg_t);
                } else if (ret_t->base_t == TYPE_ARR && ret_t->comp_t->base_t == TYPE_GENERIC) {
                    if (first_arg_t->base_t == TYPE_ARR) {
                        type_free(ret_t);
                        ret_t = type_clone(first_arg_t);
                    }
                }
            }

            type_list_free(arg_types, arg_count);
            return ret_t;
        }

        case NODE_UNARY:
            if (node->data.unary.op == OP_NOT) {
                yafl_t *t = type_check_expr(node->data.unary.operand);
                if (t && t->base_t != TYPE_BOOL) {
                    log_error(node->line, "NOT operator requires boolean operand");
                }
                type_free(t);
                return type_new_simple(TYPE_BOOL);
            }
            return type_check_expr(node->data.unary.operand);

        case NODE_ARR_IDX: {
            yafl_t *base_type = type_check_expr(node->data.arr_idx.base);
            yafl_t *idx_type = type_check_expr(node->data.arr_idx.idx);

            if (idx_type && idx_type->base_t != TYPE_SINT) {
                log_error(node->line, "Array index must be integer");
            }
            type_free(idx_type);

            if (base_type && base_type->base_t == TYPE_ARR) {
                yafl_t *ret = type_clone(base_type->comp_t);
                type_free(base_type);
                return ret;
            } else if (base_type && base_type->base_t == TYPE_STR) {
                type_free(base_type);
                return type_new_simple(TYPE_STR);
            }
            type_free(base_type);
            log_error(node->line, "Indexing non-array/string type");
            return NULL;
        }

        case NODE_ARR_LIT: {
            ast_node *el = node->data.arr_lit.elements;

            yafl_t *first_type = type_check_expr(el);
            if (!first_type) return NULL;

            el = el->next;
            while (el) {
                yafl_t *next_type = type_check_expr(el);
                if (!type_equals(first_type, next_type)) {
                    log_error(node->line, "Array literal elements must have same type");
                }
                type_free(next_type);
                el = el->next;
            }
            return type_new_composite(first_type);
        }

        case NODE_DEFAULT_VAL:
            return type_clone(node->data.default_val.type);
        case NODE_CAST:
            return type_clone(node->data.cast.type);
        default:
            return NULL;
    }
}

bool type_check_return_paths(ast_node *node) {
    if (!node) return false;

    switch (node->type){
        case NODE_RETURN:
            return true;

        case NODE_BLOCK:
            for (ast_node *s = node->data.block.stmts; s; s = s->next) {
                // stop and next make later statements unreachable
                if (s->type == NODE_STOP || s->type == NODE_NEXT) return false;
                if (type_check_return_paths(s)) return true;
            }
            return false;

        case NODE_IF: {
            bool then_ret = type_check_return_paths(node->data.if_stmt.then_block);
            bool else_ret = node->data.if_stmt.else_block ? type_check_return_paths(node->data.if_stmt.else_block) : false;
            return then_ret && else_ret;
        }

        case NODE_MATCH: {
            bool all_cases_ret = true;
            ast_node *case_node = node->data.match_stmt.cases;
            while(case_node) {
                if (case_node->data.case_stmt.body && !type_check_return_paths(case_node->data.case_stmt.body)) {
                    // Every block should contain ret statement
                    all_cases_ret = false;
                } else if (!case_node->next && !case_node->data.case_stmt.body){
                    // Last statement is fallthrough
                    all_cases_ret = false;
                }
                case_node = case_node->next;
            }
            return all_cases_ret;
        }

        case NODE_WHILE:
            return type_check_return_paths(node->data.while_loop.body);

        default:
            return false;
    }
}
