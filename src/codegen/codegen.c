#include "codegen.h"
#include "loop_stack.h"
#include "symtab.h"
#include "prog.h"
#include "utils.h"
#include "vector.h"
#include "logger.h"
#include "type_checking.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* Global state */
prog_t *prog = NULL;
symtab *prog_symtab = NULL;
int temp_var_counter = 0;
yafl_t *current_func_ret_type = NULL;
bool codegen_inline_builtins = true;

/* Forward declarations */
static void codegen_stmt(ast_node *node);
void codegen_expr(ast_node *node);

/* Convert AST param list to type array */
static yafl_t **extract_params(ast_node *params, int *count) {
    int c = 0;
    for (ast_node *p = params; p; p = p->next) c++;

    *count = c;
    if (c == 0) return NULL;

    yafl_t **types = malloc(c * sizeof(yafl_t*));
    int i = 0;
    for (ast_node *p = params; p; p = p->next) {
        types[i++] = type_clone(p->data.param.type);
    }
    return types;
}

static ast_node **extract_defaults(ast_node *params, int count) {
    if (count == 0) return NULL;
    ast_node **defaults = malloc(count * sizeof(ast_node*));
    int i = 0;
    for (ast_node *p = params; p; p = p->next) {
        defaults[i++] = p->data.param.default_value;
    }
    return defaults;
}

/* --- Codegen --- */

/* Helper for automatic zero-init (local + global vars) */
static void codegen_zero_init(yafl_t *type, int line) {
    int const_id;
    if (!type) return;

    switch (type->base_t){
        case TYPE_STR:
            // Empty String for string type
            val_t *str = v_str_create();
            const_id = prog_new_constant(prog, str);
            prog_add_num(prog, const_id);
            prog_add_op(prog, CONSTANT);
            break;

        case TYPE_BOOL:
        case TYPE_SINT:
            prog_add_num(prog, 0);
            break;
        case TYPE_FLOAT:
            val_t *fl = v_real_new_double(0.0);
            const_id = prog_new_constant(prog, fl);
            prog_add_num(prog, const_id);
            prog_add_op(prog, CONSTANT);
            break;
        case TYPE_ARR:
            prog_add_num(prog, 0);
            prog_add_op(prog, MKARRAY);
            break;
        default: {
            char buf[128];
            type_to_str(type, buf, sizeof(buf));
            log_error(line, "Zero-init on type '%s' is not implemented", buf);
            break;
        }
    }
}

static var_sym *codegen_register_var(char *name, yafl_t *type, int line){
    var_sym *sym = symtab_add_var(prog_symtab, name, type);
    if (!sym) {
        log_error(line, "Variable '%s' already declared", name);
    }
    return sym;
}

static var_sym *codegen_lookup_var(char *name, int line){
    var_sym *sym = symtab_lookup_var(prog_symtab, name);
    if (!sym) {
        log_error(line, "Variable '%s' not declared", name);
    }
    return sym;
}

/* recursion helper for array literals */
void codegen_list_reverse(ast_node *elem, int *count) {
    if (!elem) return;

    codegen_list_reverse(elem->next, count);
    codegen_expr(elem);
    if(count) (*count)++;
}

void codegen_push_func_arguments(ast_node *node, func_sym *sym, int arg_count){
    // Generate default values for missing arguments (pushed first -> bottom of stack)
    if (arg_count < sym->num_params) {
        if (!sym->default_values) {
                log_error(node->line, "Missing arguments for function '%s'", node->data.call.name);
        }
        for (int i = sym->num_params - 1; i >= arg_count; i--) {
            if (!sym->default_values[i]) {
                log_error(node->line, "Missing argument %d for function '%s' (no default value)",
                    i+1, node->data.call.name);
            }
            codegen_expr(sym->default_values[i]);
        }
    }

    // Push explicit arguments to stack in reverse order
    codegen_list_reverse(node->data.call.args, NULL);
}

/* Helper to lookup variable id and generate the correct OPCODE (local/global)*/
static void codegen_set_get_var(char *name, int line, enum opcodes op_type){
    var_sym *sym = codegen_lookup_var(name, line);
    prog_add_num(prog, sym->nr);
    if(op_type == GETVAR){
        prog_add_op(prog, (sym->is_global) ? GETGLOBAL : GETVAR);
    } else {
        prog_add_op(prog, (sym->is_global) ? SETGLOBAL : SETVAR);
    }
}



void codegen_expr(ast_node *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_INT:
            prog_add_num(prog, node->data.integer.value);
            break;

        case NODE_FLOAT: {
            val_t *fl = v_real_new_double(node->data.float_nr.value);
            int const_id = prog_new_constant(prog, fl);
            prog_add_num(prog, const_id);
            prog_add_op(prog, CONSTANT);
            break;
        }

        case NODE_BOOL:
            prog_add_num(prog, node->data.boolean.value);
            break;

        case NODE_STR: {
            val_t *str = v_str_new_cstr(node->data.string.value);
            int const_id = prog_new_constant(prog, str);
            prog_add_num(prog, const_id);
            prog_add_op(prog, CONSTANT);
            break;
        }

        case NODE_VAR: {
            codegen_set_get_var(node->data.var.name, node->line, GETVAR);
            break;
        }

        case NODE_BINARY: {
            codegen_expr(node->data.binary.right);
            codegen_expr(node->data.binary.left);

            switch (node->data.binary.op) {
                case OP_ADD: prog_add_op(prog, ADD); break;
                case OP_SUB: prog_add_op(prog, SUB); break;
                case OP_MUL: prog_add_op(prog, MUL); break;
                case OP_DIV: prog_add_op(prog, DIV); break;
                case OP_MOD: prog_add_op(prog, MOD); break;
                case OP_LT:  prog_add_op(prog, LESS); break;
                case OP_GT:  prog_add_op(prog, GREATER); break;
                case OP_LE:  prog_add_op(prog, LESSEQUAL); break;
                case OP_GE:  prog_add_op(prog, GREATEREQUAL); break;
                case OP_EQ:  prog_add_op(prog, EQUAL); break;
                case OP_NE:  prog_add_op(prog, NOTEQUAL); break;
                case OP_AND: prog_add_op(prog, AND); break;
                case OP_OR:  prog_add_op(prog, OR); break;
                default:
                    log_error(node->line, "Unknown binary operator");
            }
            break;
        }

        case NODE_UNARY: {
            codegen_expr(node->data.unary.operand);
            ast_node *operand = node->data.unary.operand;
            switch (node->data.unary.op) {
                case OP_NEG: prog_add_op(prog, NEG); break;
                case OP_NOT: prog_add_op(prog, NOT); break;
                case OP_INC:
                    prog_add_op(prog, INC);
                    // For variables we need to not only increment but also set
                    if (operand->type == NODE_VAR) {
                        prog_add_op(prog, DUP);
                        codegen_set_get_var(operand->data.var.name,
                            operand->line, SETVAR);
                    }
                    break;
                case OP_DEC:
                    prog_add_op(prog, DEC);
                    // For variables we need to not only decrement but also set
                    if (operand->type == NODE_VAR) {
                        prog_add_op(prog, DUP);
                        codegen_set_get_var(operand->data.var.name,
                            operand->line, SETVAR);
                    }
                    break;
                default:
                    log_error(node->line, "Unknown unary operator");
            }
            break;
        }

        case NODE_ARR_IDX: {
            codegen_expr(node->data.arr_idx.idx);
            codegen_expr(node->data.arr_idx.base);
            prog_add_op(prog, INDEX1);
            break;
        }
        case NODE_ARR_LIT: {
            int count = 0;

            // use recursion to generate expressions in reversed order
            codegen_list_reverse(node->data.arr_lit.elements, &count);

            prog_add_num(prog, count);
            prog_add_op(prog, MKARRAY);
            break;
        }

        case NODE_DEFAULT: {
            codegen_zero_init(node->data.default_val.type, node->line);
            break;
        }

        case NODE_ARR_FILL: {
            // new pseudo scope to prevent duplication of count and idx variables
            symtab_enter_scope(prog_symtab);
            codegen_expr(node->data.arr_fill.count);

            yafl_t *cnt_type = type_new_simple(TYPE_SINT);
            var_sym *cnt_sym = codegen_register_var(".count", cnt_type, node->line);
            type_free(cnt_type);
            prog_add_num(prog, cnt_sym->nr);
            prog_add_op(prog, SETVAR);

            yafl_t *idx_type = type_new_simple(TYPE_SINT);
            var_sym *idx_sym = codegen_register_var(".idx", idx_type, node->line);
            type_free(idx_type);
            prog_add_num(prog, 0);
            prog_add_num(prog, idx_sym->nr);
            prog_add_op(prog, SETVAR);

            int loop_start_pc = prog_next_pc(prog);

            // idx < count
            prog_add_num(prog, cnt_sym->nr);
            prog_add_op(prog, GETVAR);
            prog_add_num(prog, idx_sym->nr);
            prog_add_op(prog, GETVAR);
            prog_add_op(prog, LESS);

            int jmp_out_pc = prog_add_num(prog, -1); // Placeholder
            prog_add_op(prog, JUMPF);

            // Push elements in reverse order for MKARRAY
            codegen_list_reverse(node->data.arr_fill.elements, NULL);

            // Increment idx
            prog_add_num(prog, idx_sym->nr);
            prog_add_op(prog, GETVAR);
            prog_add_op(prog, INC);
            prog_add_num(prog, idx_sym->nr);
            prog_add_op(prog, SETVAR);

            // Jump back
            prog_add_num(prog, loop_start_pc);
            prog_add_op(prog, JUMP);

            // Patch exit jump
            prog_set_num(prog, jmp_out_pc, prog_next_pc(prog));

            // Push total count for MKARRAY: count * list_length
            int list_len = 0;
            for (ast_node *e = node->data.arr_fill.elements; e; e = e->next) list_len++;
            prog_add_num(prog, list_len);
            prog_add_num(prog, cnt_sym->nr);
            prog_add_op(prog, GETVAR);
            prog_add_op(prog, MUL);

            prog_add_op(prog, MKARRAY);
            symtab_exit_scope(prog_symtab);
            break;
        }

        case NODE_CALL: {
            // Need to determine types of arguments to resolve overload
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

            func_sym *sym = symtab_lookup_func(prog_symtab, node->data.call.name, arg_types, arg_count);

            if (!sym) {
                log_error(node->line, "Function '%s' not declared or no matching overload found",
                            node->data.call.name);
            }
            type_list_free(arg_types, arg_count);



            if (sym->is_builtin) {
                // Execute the builtin codegen
                sym->impl.codegen(prog, node, sym, arg_count);
            } else {
                codegen_push_func_arguments(node, sym, arg_count);
                prog_add_num(prog, sym->num_params);
                int pc_loc = prog_add_num(prog, sym->impl.pc);

                if (sym->impl.pc == -1) {
                    // Backpatching required
                    symtab_add_fixup(sym, pc_loc);
                }

                prog_add_op(prog, CALL_PC);
            }
            break;
        }

        case NODE_CAST: {
            codegen_expr(node->data.cast.expr);
            break;
        }

        default:
            log_error(node->line, "Invalid expression node type");
    }
}



static void codegen_stmt(ast_node *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_DECL: {
            // Generate initializer
            if (node->data.decl.init) {
                codegen_expr(node->data.decl.init);
                yafl_t *init_t = type_check_expr(node->data.decl.init);
                type_check_compatibility(node->data.decl.type, init_t, node->line, "variable declaration");
                type_free(init_t);
            } else {
                // Auto-init to zero
                codegen_zero_init(node->data.decl.type, node->line);
            }

            var_sym *sym = codegen_register_var(node->data.decl.name,
                node->data.decl.type, node->line);
            prog_add_num(prog, sym->nr);
            prog_add_op(prog, SETVAR);
            break;
        }

        case NODE_ASSIGN: {
            codegen_expr(node->data.assign.value);
            var_sym *sym = codegen_lookup_var(node->data.assign.name, node->line);
            yafl_t *val_t = type_check_expr(node->data.assign.value);
            type_check_compatibility(sym->type, val_t, node->line, "assignment");
            type_free(val_t);

            codegen_set_get_var(node->data.assign.name, node->line, SETVAR);
            break;
        }

        case NODE_RETURN:
            if (node->data.ret.value) {
                codegen_expr(node->data.ret.value);
                yafl_t *ret_t = type_check_expr(node->data.ret.value);
                type_check_compatibility(current_func_ret_type, ret_t, node->line, "return statement");
                type_free(ret_t);
            } else {
                if (current_func_ret_type && current_func_ret_type->base_t != TYPE_VOID) {
                    log_error(node->line, "Return value expected for non-void function");
                }
            }
            prog_add_op(prog, RET);
            break;

        case NODE_BLOCK:
            symtab_enter_scope(prog_symtab);
            for (ast_node *s = node->data.block.stmts; s; s = s->next) {
                codegen_stmt(s);
            }
            symtab_exit_scope(prog_symtab);
            break;

        case NODE_IF: {
            codegen_expr(node->data.if_stmt.condition);
            yafl_t *cond_t = type_check_expr(node->data.if_stmt.condition);
            yafl_t *bool_t = type_new_simple(TYPE_BOOL);
            type_check_compatibility(bool_t, cond_t, node->line, "if condition");
            type_free(cond_t);
            type_free(bool_t);

            int else_jmp_pc = prog_add_num(prog, -1);
            prog_add_op(prog, JUMPF);

            codegen_stmt(node->data.if_stmt.then_block);

            if (node->data.if_stmt.else_block) {
                // else - JUMP to end after then block
                int then_end_jmp_pc = prog_add_num(prog, -1);
                prog_add_op(prog, JUMP);

                // set JUMPF target for else
                int else_jmp_trgt = prog_next_pc(prog);
                prog_set_num(prog, else_jmp_pc, else_jmp_trgt);

                codegen_stmt(node->data.if_stmt.else_block);

                // set JUMP target for then block
                int then_end_jmp_trgt = prog_next_pc(prog);
                prog_set_num(prog, then_end_jmp_pc, then_end_jmp_trgt);
            } else {
                // no else - JUMPF just jumps over then block
                int else_jmp_trgt = prog_next_pc(prog);
                prog_set_num(prog, else_jmp_pc, else_jmp_trgt);
            }
            break;
        }

        case NODE_WHILE: {
            int cond_pc = prog_next_pc(prog);

            loop_push(cond_pc, false);

            codegen_expr(node->data.while_loop.condition);
            yafl_t *cond_t = type_check_expr(node->data.while_loop.condition);
            yafl_t *bool_t = type_new_simple(TYPE_BOOL);
            type_check_compatibility(bool_t, cond_t, node->line, "while condition");
            type_free(cond_t);
            type_free(bool_t);

            int exit_jmp = prog_add_num(prog, -1);
            prog_add_op(prog, JUMPF);

            codegen_stmt(node->data.while_loop.body);

            prog_add_num(prog, cond_pc);
            prog_add_op(prog, JUMP);

            int ext_jmp_trgt = prog_next_pc(prog);
            prog_set_num(prog, exit_jmp, ext_jmp_trgt);

            loop_pop(ext_jmp_trgt);
            break;
        }

        case NODE_MATCH: {
            val_t *switch_map = v_map_create();
            int map_const_id = prog_new_constant(prog, switch_map);
            prog_add_num(prog, map_const_id);
            prog_add_op(prog, CONSTANT); // Map

            int default_pc_loc = prog_add_num(prog, -1); // Default pc
            codegen_expr(node->data.match_stmt.expr); // Match val

            prog_add_num(prog, 3); // Arg count

            val_t *fn_name = v_str_new_cstr("SWITCH_LOOKUP");
            int fn_id = prog_new_constant(prog, fn_name);
            prog_add_num(prog, fn_id);
            prog_add_op(prog, CONSTANT);
            prog_add_op(prog, CALL);
            prog_add_op(prog, JUMP);

            /* Prepare for backpatching breaks */
            vector break_jumps_vec;
            vector *break_jumps = &break_jumps_vec;
            vector_init(break_jumps, NULL);

            /* Pending cases (fallthrough) */
            vector pending_keys_vec;
            vector *pending_keys = &pending_keys_vec;
            vector_init(pending_keys, NULL);

            bool default_pc_set = false;
            bool pending_default = false;

            /* Generate cases */
            ast_node *case_node = node->data.match_stmt.cases;
            while (case_node) {
                ast_node *expr = case_node->data.case_stmt.expr;
                ast_node *body = case_node->data.case_stmt.body;

                if (expr) {
                    // Regular case
                    val_t *key_val = NULL;
                    switch (expr->type){
                        case NODE_BOOL:
                            key_val = v_num_new_int(expr->data.boolean.value);
                            break;
                        case NODE_INT:
                            key_val = v_num_new_int(expr->data.integer.value);
                            break;
                        case NODE_STR:
                            key_val = v_str_new_cstr(expr->data.string.value);
                            break;
                        default:
                            log_error(case_node->line, "Switch case must be a constant literal (int, str, bool)");
                            break;
                    }
                    vector_push(pending_keys, key_val);
                } else {
                    // Default case
                    if (default_pc_set || pending_default) {
                        log_error(case_node->line, "Multiple default cases in switch");
                    }
                    if (!body) {
                        // Default as fallthrough
                        pending_default = true;
                    }
                }

                if (body) {
                    int current_pc = prog_next_pc(prog);

                    // map all pending keys (no body) to this body (fallthrough)
                    while(!vector_is_empty(pending_keys)){
                        map_set(switch_map->u.map, (val_t *)vector_pop(pending_keys), v_num_new_int(current_pc));
                    }

                    if (expr == NULL) {
                        // Default body
                        prog_set_num(prog, default_pc_loc, current_pc);
                        default_pc_set = true;
                    } else if (pending_default) {
                        // Default is pending
                        prog_set_num(prog, default_pc_loc, current_pc);
                        default_pc_set = true;
                        pending_default = false;
                    }

                    codegen_stmt(body);

                    // Implicit break: jump to end
                    int jmp = prog_add_num(prog, -1);
                    prog_add_op(prog, JUMP);
                    int *jmp_ptr = malloc(sizeof(int));
                    *jmp_ptr = jmp;
                    vector_push(break_jumps, jmp_ptr);
                }

                case_node = case_node->next;
            }

            int end_pc = prog_next_pc(prog);

            /* Patch breaks */
            while(!vector_is_empty(break_jumps)){
                int *ptr = (int *)vector_pop(break_jumps);
                prog_set_num(prog, *ptr, end_pc);
                free(ptr);
            }
            vector_free(break_jumps);

            /* If default never set, set to end_pc */
            if (!default_pc_set) {
                prog_set_num(prog, default_pc_loc, end_pc);
            }

            /* Map remaining pending keys to end_pc (empty cases at end) */
            while(!vector_is_empty(pending_keys)){
                map_set(switch_map->u.map, (val_t *)vector_pop(pending_keys), v_num_new_int(end_pc));
            }
            vector_free(pending_keys);
            break;
        }

        case NODE_FOR: {
            // Loop variable lives in extra scope
            symtab_enter_scope(prog_symtab);

            // Create Iterator
            codegen_expr(node->data.for_loop.iterable);
            yafl_t *iter_t = type_check_expr(node->data.for_loop.iterable);
            yafl_t *elem_t = NULL;

            if (iter_t->base_t == TYPE_ARR) {
                 elem_t = type_clone(iter_t->comp_t);
            } else if (iter_t->base_t == TYPE_STR) {
                 elem_t = type_new_simple(TYPE_STR);
            } else if (iter_t->base_t == TYPE_RANGE) {
                 elem_t = type_new_simple(TYPE_SINT);
            } else {
                 log_error(node->line, "Expression is not iterable (must be array, string, or range)");
            }

            prog_add_op(prog, ITER_BEGIN);

            // Register loop variable
            ast_node *var_node = node->data.for_loop.var;
            var_sym *loop_var;
            if (var_node->type == NODE_FOR_DECL){
                type_check_compatibility(var_node->data.for_decl.type, elem_t, node->line, "for loop variable");
                loop_var = codegen_register_var(var_node->data.for_decl.name, var_node->data.for_decl.type, var_node->line);
            } else {
                loop_var = codegen_lookup_var(var_node->data.for_var.name, var_node->line);
                type_check_compatibility(loop_var->type, elem_t, node->line, "for loop variable");
            }

            type_free(iter_t);
            type_free(elem_t);

            int loop_start_pc = prog_next_pc(prog);
            loop_push(loop_start_pc, true);

            // ITER_NEXT -> (iterator, val, 1) OR (0)
            prog_add_op(prog, ITER_NEXT);
            int exit_jmp = prog_add_num(prog, -1);
            prog_add_op(prog, JUMPF);
            prog_add_num(prog, loop_var->nr);
            prog_add_op(prog, SETVAR);

            codegen_stmt(node->data.for_loop.body);

            prog_add_num(prog, loop_start_pc);
            prog_add_op(prog, JUMP);

            // Patch exit
            int exit_jmp_trgt = prog_next_pc(prog);
            prog_set_num(prog, exit_jmp, exit_jmp_trgt);

            loop_pop(exit_jmp_trgt);

            symtab_exit_scope(prog_symtab);
            break;
        }

        case NODE_INT:
        case NODE_FLOAT:
        case NODE_BOOL:
        case NODE_STR:
        case NODE_VAR:
        case NODE_UNARY:
        case NODE_BINARY:
        case NODE_CALL:
        case NODE_CAST:
        case NODE_ARR_IDX:
        case NODE_ARR_LIT: {
            codegen_expr(node);
            yafl_t *t = type_check_expr(node);
            if (t && t->base_t != TYPE_VOID) {
                prog_add_op(prog, DISCARD);
            }
            type_free(t);
            break;
        }

        case NODE_ARR_ASSIGN: {
            codegen_expr(node->data.arr_assign.value);
            codegen_expr(node->data.arr_assign.idx);
            codegen_expr(node->data.arr_assign.base);

            yafl_t *base_t = type_check_expr(node->data.arr_assign.base);
            yafl_t *idx_t = type_check_expr(node->data.arr_assign.idx);
            yafl_t *val_t = type_check_expr(node->data.arr_assign.value);

            if (!base_t || base_t->base_t != TYPE_ARR) {
                log_error(node->line, "Array assignment on non-array type");
            }
            yafl_t *int_t = type_new_simple(TYPE_SINT);
            type_check_compatibility(int_t, idx_t, node->line, "array index");
            type_check_compatibility(base_t->comp_t, val_t, node->line, "array assignment value");

            type_free(base_t); type_free(idx_t); type_free(val_t); type_free(int_t);

            prog_add_op(prog, INDEXAS);
            break;
        }

        case NODE_NEXT: {
            if (!loop_stack) {
                log_error(node->line, "next (continue) statement outside of loop");
            }
            prog_add_num(prog, loop_stack->continue_pc);
            prog_add_op(prog, JUMP);
            break;
        }

        case NODE_STOP: {
            if (!loop_stack) {
                log_error(node->line, "stop (break) statement outside of loop");
            }

            // For loops have an iterator on the stack that needs to be popped
            if (loop_stack->has_iterator) {
                prog_add_op(prog, DISCARD);
            }

            int jmp_loc = prog_add_num(prog, -1);
            prog_add_op(prog, JUMP);
            loop_add_break_jump(jmp_loc);
            break;
        }

        default:
            log_error(node->line, "Invalid statement node type");
            break;
    }
}

void codegen(ast_node *root, char *filename) {
    prog = prog_new();
    prog_symtab = symtab_create();
    int start_pc = 0;

    // First pass: Register all functions in my symtab
    // This allows recursive and forward function calls
    for (ast_node *node = root; node; node = node->next) {
        if (node->type == NODE_FUNC) {
            int num_params = 0;
            yafl_t **param_types = extract_params(node->data.func.params, &num_params);
            ast_node **defaults = extract_defaults(node->data.func.params, num_params);

            func_sym *sym = symtab_add_func(
                prog_symtab,
                node->data.func.name,
                node->data.func.return_type,
                num_params,
                param_types,
                defaults,
                -1   // pc unknown for now
            );

            // Type check default arguments
            ast_node *param_node = node->data.func.params;
            for (int i = 0; i < num_params; i++) {
                if (defaults[i]) {
                    yafl_t *def_t = type_check_expr(defaults[i]);
                    type_check_compatibility(param_types[i], def_t, param_node->line, "default argument");
                    type_free(def_t);
                }
                param_node = param_node->next;
            }

            type_list_free(param_types, num_params);
            free(defaults);

            if (!sym) {
                log_error(0, "Function '%s' already declared or duplicate signature",
                            node->data.func.name);
            }
        } else if (node->type == NODE_DECL) {
            var_sym *sym = codegen_register_var(node->data.decl.name,
                node->data.decl.type, node->line);

            // Initialize global
            if (node->data.decl.init) {
                codegen_expr(node->data.decl.init);
            } else {
                codegen_zero_init(node->data.decl.type, node->line);
            }
            prog_add_num(prog, sym->nr);
            prog_add_op(prog, SETGLOBAL);
        }
    }
    // JUMP to start
    prog_add_num(prog, 0);
    // i manage mappings myself so no lookups needed
    start_pc = prog_add_num(prog, -1);
    prog_add_op(prog, CALL_PC);

    // End program
    prog_add_op(prog, HALT);

    // Code looks like:
    // -> imports (maybe in the future)
    // -> global vars
    // -> jmp to fn start()
    // -> HALT
    // -> function bodies


    // Second pass: Generate actual function bodies
    for (ast_node *node = root; node; node = node->next) {
        // Global vars done in the first pass
        // this works since it iterates over the toplevel linked list
        if (node->type != NODE_FUNC)
            continue;

        // Register correct entry point in symtab + register function in vm
        int num_params = 0;
        yafl_t **param_types = extract_params(node->data.func.params, &num_params);

        func_sym *func = symtab_lookup_func(prog_symtab, node->data.func.name, param_types, num_params);
        type_list_free(param_types, num_params);

        if (!func) {
            log_error(node->line, "Function '%s' not found during Pass 2", node->data.func.name);
        }

        int func_pc = prog_next_pc(prog);
        func->impl.pc = func_pc;
        prog_register_function(prog, node->data.func.name, func_pc);
        log_debug(0, "Generating function %s at pc=%d (sym addr: %p)",
            node->data.func.name, func_pc, (void*)func);

        // Apply fixups
        fixup_node *fixup = func->fixups;
        while (fixup) {
            log_debug(0, "  Backpatching call to %s at %d", node->data.func.name, fixup->pc_location);
            prog_set_num(prog, fixup->pc_location, func_pc);

            fixup_node *next = fixup->next;
            free(fixup);
            fixup = next;
        }
        func->fixups = NULL;

        // Enter function scope
        symtab_enter_scope(prog_symtab);
        // The CALL/CALL_PC opcodes create a stack frame so variables should start from zero again
        prog_symtab->current->var_offset = 0;

        // Set return type context
        current_func_ret_type = func->ret_type;

        // Parameters become locals 0..n-1
        for (ast_node *p = node->data.func.params; p; p = p->next) {
            symtab_add_var(prog_symtab, p->data.param.name, p->data.param.type);
        }

        // Generate function body
        codegen_stmt(node->data.func.body);

        // Check for return statement in non-void functions
        if (!type_check_return_paths(node->data.func.body) && node->data.func.return_type->base_t != TYPE_VOID) {
            log_error(node->line, "Control reaches end of non-void function '%s'", node->data.func.name);
        } else {
            // Implicit return for void functions
            prog_add_op(prog, RET);
        }

        // "Free" the ids of the current scope
        symtab_exit_scope(prog_symtab);
    }

    // start JUMP
    func_sym *start = symtab_lookup_func(prog_symtab, "start", NULL, 0);
    if (!start) {
        log_error(0, "Entry point 'start()' not found");
    }
    if (start->num_params != 0) {
        log_error(0, "Entry point 'start' must have 0 parameters");
    }
    if (start->ret_type->base_t != TYPE_VOID && start->ret_type->base_t != TYPE_SINT) {
        log_error(0, "Entry point 'start' must return 'none' or 'int'");
    }
    log_debug(0, "Start function found at pc=%d (sym addr: %p)", start->impl.pc, (void*)start);
    prog_set_num(prog, start_pc, start->impl.pc);

    if(logger_get_level() > LOG_INFO){
        log_debug(-1, "Dumping symbol table and bytecode:");
        symtab_dump(prog_symtab);
        prog_dump(prog);
    }

    symtab_free(prog_symtab);
    log_info(0, "Code generation complete.");

    // Write to file
    if (prog_write(prog, filename)) {
        log_info(0, "Bytecode written to %s", filename);
    } else {
        log_error(0, "Error writing bytecode to %s", filename);
    }
}
