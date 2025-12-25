#include "codegen.h"
#include "symtab.h"
#include "prog.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* Global state */
static prog_t *prog = NULL;
static symtab *prog_symtab = NULL;

/* Forward declarations */
static void codegen_stmt(ast_node *node);
static void codegen_expr(ast_node *node);

/* --- Type Checking --- */
yafl_t get_expr_type(symtab *table, ast_node *node) {
    if (!node) return TYPE_VOID;

    switch (node->type) {
        case NODE_INT:
            return TYPE_SINT;
        case NODE_FLOAT:
            return TYPE_FLOAT;
        case NODE_BOOL:
            return TYPE_BOOL;
        case NODE_STR:
            return TYPE_STR;
        case NODE_VAR: {
            symbol *sym = symtab_lookup(table, node->data.var.name);
            return sym ? sym->type : TYPE_VOID;
        }
        case NODE_BINARY:
            if (node->data.binary.op == OP_ADD) {
                yafl_t left = get_expr_type(table, node->data.binary.left);
                if (left == TYPE_STR) return TYPE_STR;
            }
            // Comparison operators return bool
            if (node->data.binary.op == OP_LT ||
                node->data.binary.op == OP_GT ||
                node->data.binary.op == OP_LE ||
                node->data.binary.op == OP_GE ||
                node->data.binary.op == OP_EQ ||
                node->data.binary.op == OP_NE) {
                return TYPE_BOOL;
            }
            return TYPE_SINT;
        case NODE_CALL: {
            symbol *sym = symtab_lookup(table, node->data.call.name);
            return sym ? sym->func.ret_type : TYPE_VOID;
        }

        case NODE_UNARY:
            if (node->data.unary.op == OP_NOT) {
                return TYPE_BOOL;
            }
            return get_expr_type(table, node->data.unary.operand);

        case NODE_CAST:
            return node->data.cast.type;

        default:
            return TYPE_VOID;
    }
}

/* --- REPORTING --- */
void codegen_error(int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Yafl codegen error (line %d): ", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

void codegen_warn(int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Yafl codegen warning (line %d): ", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

/* --- Codegen --- */

/* Helper for automatic zero-init (local + global vars) */
static void codegen_zero_init(yafl_t type, int line) {
    int const_id;
    switch (type){
        case TYPE_STR:
            // Empty String for string type
            val_t *str = v_str_create();
            const_id = prog_new_constant(prog, str);
            prog_add_num(prog, const_id);
            prog_add_op(prog, CONSTANT);
            break;

        case TYPE_BOOL:
        case TYPE_SINT:
        case TYPE_UINT:
            prog_add_num(prog, 0);
            break;
        case TYPE_FLOAT:
            val_t *fl = v_real_new_double(0.0);
            const_id = prog_new_constant(prog, fl);
            prog_add_num(prog, const_id);
            prog_add_op(prog, CONSTANT);
        default:
            codegen_error(line, "Zero-init on type '%s' is not implemented", type_to_string(type));
            break;
    }
}

static symbol *codegen_register_var(char *name, yafl_t type, int line){
    symbol *sym = symtab_add_var(prog_symtab, name, type);
    if (!sym) {
        codegen_error(line, "Variable '%s' already declared", name);
    }
    return sym;
}

static symbol *codegen_lookup_var(char *name, int line){
    symbol *sym = symtab_lookup(prog_symtab, name);
    if (!sym || sym->type == TYPE_FUNC) {
        codegen_error(line, "Variable '%s' not declared", name);
    }
    return sym;
}

/* Helper to lookup variable id and generate the correct OPCODE (local/global)*/
static void codegen_set_get_var(char *name, int line, enum opcodes op_type){
    symbol *sym = codegen_lookup_var(name, line);
    prog_add_num(prog, sym->var.var_nr);
    if(op_type == GETVAR){
        prog_add_op(prog, (sym->var.global) ? GETGLOBAL : GETVAR);
    } else {
        prog_add_op(prog, (sym->var.global) ? SETGLOBAL : SETVAR);
    }
}

static int yafl_t_to_vm_type(yafl_t type, int line) {
    switch (type) {
        case TYPE_SINT:
        case TYPE_UINT:
        case TYPE_BOOL: return T_NUM;
        case TYPE_STR:  return T_STR;
        case TYPE_FLOAT: return T_REAL;
        default:
            codegen_error(line, "Unsupported type for cast: %s", type_to_string(type));
            return -1; // Should not reach here
    }
}

static void codegen_expr(ast_node *node) {
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
                    codegen_error(node->line, "Unknown binary operator");
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
                    codegen_error(node->line, "Unknown unary operator");
            }
            break;
        }

        case NODE_CALL: {
            symbol *sym = symtab_lookup(prog_symtab, node->data.call.name);
            if (!sym || sym->type != TYPE_FUNC) {
                codegen_error(node->line, "Function '%s' not declared",
                            node->data.call.name);
            }

            // Push arguments
            int arg_count = 0;
            ast_node *arg = node->data.call.args;
            while (arg) {
                codegen_expr(arg);
                arg_count++;
                arg = arg->next;
            }

            prog_add_num(prog, arg_count);
            prog_add_num(prog, sym->func.pc);
            prog_add_op(prog, CALL_PC);
            break;
        }

        case NODE_CAST: {
            yafl_t src_type = get_expr_type(prog_symtab, node->data.cast.expr);
            yafl_t dest_type = node->data.cast.type;

            // Self cast
            if (src_type == dest_type) {
                codegen_expr(node->data.cast.expr);
                break;
            }

            codegen_expr(node->data.cast.expr);

            if (dest_type == TYPE_BOOL) {
                if (src_type == TYPE_SINT || src_type == TYPE_UINT) {
                    // int -> bool: != 0
                    prog_add_num(prog, 0);
                    prog_add_op(prog, NOTEQUAL);
                } else if (src_type == TYPE_FLOAT) {
                    // float -> bool: != 0.0
                    val_t *fl = v_real_new_double(0.0);
                    int const_id = prog_new_constant(prog, fl);
                    prog_add_num(prog, const_id);
                    prog_add_op(prog, CONSTANT);
                    prog_add_op(prog, NOTEQUAL);
                } else if (src_type == TYPE_STR) {
                    // str -> bool: >>Yes<< = Yes, >>No<< & else = No
                    val_t *str_yes = v_str_new_cstr("Yes");
                    int const_yes = prog_new_constant(prog, str_yes);
                    prog_add_num(prog, const_yes);
                    prog_add_op(prog, CONSTANT);
                    prog_add_op(prog, EQUAL);
                } else {
                    prog_add_num(prog, yafl_t_to_vm_type(dest_type, node->line));
                    prog_add_op(prog, CAST);
                }
            } else if (dest_type == TYPE_STR) {
                if (src_type == TYPE_BOOL) {
                    // bool -> str: Yes -> >>Yes<<, No -> >>No<< (if else bytecode)
                    int label_false = prog_add_num(prog, -1);
                    prog_add_op(prog, JUMPF);

                    val_t *str_yes = v_str_new_cstr("Yes");
                    int const_yes = prog_new_constant(prog, str_yes);
                    prog_add_num(prog, const_yes);
                    prog_add_op(prog, CONSTANT);

                    int jump_end = prog_add_num(prog, -1);
                    prog_add_op(prog, JUMP);

                    int false_target = prog_next_pc(prog);
                    prog_set_num(prog, label_false, false_target);

                    val_t *str_no = v_str_new_cstr("No");
                    int const_no = prog_new_constant(prog, str_no);
                    prog_add_num(prog, const_no);
                    prog_add_op(prog, CONSTANT);

                    // End target
                    int end_target = prog_next_pc(prog);
                    prog_set_num(prog, jump_end, end_target);
                } else {
                    // int/float -> str: standard CAST
                    prog_add_num(prog, yafl_t_to_vm_type(dest_type, node->line));
                    prog_add_op(prog, CAST);
                }
            } else if (dest_type == TYPE_SINT || dest_type == TYPE_UINT) {
                if (src_type == TYPE_STR) {
                    // str -> int: 0
                    prog_add_op(prog, DISCARD);
                    prog_add_num(prog, 0);
                } else {
                    // float -> int: standard CAST
                    prog_add_num(prog, yafl_t_to_vm_type(dest_type, node->line));
                    prog_add_op(prog, CAST);
                }
            } else if (dest_type == TYPE_FLOAT) {
                if (src_type == TYPE_STR) {
                    // str -> float: 0.0
                    prog_add_op(prog, DISCARD);
                    val_t *fl = v_real_new_double(0.0);
                    int const_id = prog_new_constant(prog, fl);
                    prog_add_num(prog, const_id);
                    prog_add_op(prog, CONSTANT);
                } else {
                    // int -> float: standard CAST
                    prog_add_num(prog, yafl_t_to_vm_type(dest_type, node->line));
                    prog_add_op(prog, CAST);
                }
            } else {
                // Generic fallback
                prog_add_num(prog, yafl_t_to_vm_type(node->data.cast.type, node->line));
                prog_add_op(prog, CAST);
            }
            break;
        }

        default:
            codegen_error(node->line, "Invalid expression node type");
    }
}

static void codegen_stmt(ast_node *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_DECL: {
            // Generate initializer
            if (node->data.decl.init) {
                codegen_expr(node->data.decl.init);
            } else {
                // Auto-init to zero
                codegen_zero_init(node->data.decl.type, node->line);
            }

            symbol *sym = codegen_register_var(node->data.decl.name,
                node->data.decl.type, node->line);
            prog_add_num(prog, sym->var.var_nr);
            prog_add_op(prog, SETVAR);
            break;
        }

        case NODE_ASSIGN: {
            codegen_expr(node->data.assign.value);
            codegen_set_get_var(node->data.assign.name, node->line, SETVAR);
            break;
        }

        case NODE_RETURN:
            if (node->data.ret.value) {
                codegen_expr(node->data.ret.value);
            }
            prog_add_op(prog, RET);
            break;

        case NODE_PRINT:
            codegen_expr(node->data.print.arg);
            prog_add_op(prog, PRINT);
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
            codegen_expr(node->data.while_loop.condition);

            int exit_jmp = prog_add_num(prog, -1);
            prog_add_op(prog, JUMPF);

            codegen_stmt(node->data.while_loop.body);

            prog_add_num(prog, cond_pc);
            prog_add_op(prog, JUMP);

            int ext_jmp_trgt = prog_next_pc(prog);
            prog_set_num(prog, exit_jmp, ext_jmp_trgt);
            break;
        }

        case NODE_FOR: {
            // Equivalent to:
            //   i = start
            //   while (i < end) {
            //     body
            //     i += step
            //   }

            // Generate loop variable
            symtab_enter_scope(prog_symtab); // loop_var gets extra scope
            ast_node *var_node = node->data.for_loop.var;
            symbol *loop_var;
            if (var_node->type == NODE_FOR_DECL){
                loop_var = codegen_register_var(var_node->data.for_decl.name, var_node->data.for_decl.type, var_node->line);
            } else {
                loop_var = codegen_lookup_var(var_node->data.for_var.name, var_node->line);
            }

            codegen_expr(node->data.for_loop.start);
            codegen_set_get_var(loop_var->name, node->data.for_loop.start->line, SETVAR);

            int cond_pc = prog_next_pc(prog);
            codegen_expr(node->data.for_loop.end);
            codegen_set_get_var(loop_var->name, var_node->line, GETVAR);
            prog_add_op(prog, LESS);

            int exit_jmp = prog_add_num(prog, -1);
            prog_add_op(prog, JUMPF);

            codegen_stmt(node->data.for_loop.body);

            codegen_expr(node->data.for_loop.step);
            codegen_set_get_var(loop_var->name, node->data.for_loop.step->line, GETVAR);
            prog_add_op(prog, ADD);
            codegen_set_get_var(loop_var->name, node->data.for_loop.step->line, SETVAR);

            prog_add_num(prog, cond_pc);
            prog_add_op(prog, JUMP);

            int ext_jmp_trgt = prog_next_pc(prog);
            prog_set_num(prog, exit_jmp, ext_jmp_trgt);

            symtab_exit_scope(prog_symtab);
            break;
        }

        case NODE_UNARY:
        case NODE_BINARY:
        case NODE_CALL:
            codegen_expr(node);
            if (get_expr_type(prog_symtab, node) != TYPE_VOID) {
                prog_add_op(prog, DISCARD);
            }
            break;

        default:
            codegen_error(node->line, "Invalid statement node type");
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
            symbol *sym = symtab_add_func(
                prog_symtab,
                node->data.func.name,
                node->data.func.return_type,
                -1   // pc unknown for now
            );

            if (!sym) {
                codegen_error(0, "Function '%s' already declared",
                            node->data.func.name);
            }
        } else if (node->type == NODE_DECL) {
            symbol *sym = codegen_register_var(node->data.decl.name,
                node->data.decl.type, node->line);

            // Initialize global
            if (node->data.decl.init) {
                codegen_expr(node->data.decl.init);
            } else {
                codegen_zero_init(node->data.decl.type, node->line);
            }
            prog_add_num(prog, sym->var.var_nr);
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

    printf("Registered %d functions\n", prog_symtab->current->map->size);

    // Second pass: Generate actual function bodies
    for (ast_node *node = root; node; node = node->next) {
        // Global vars done in the first pass
        // this works since it iterates over the toplevel linked list
        if (node->type != NODE_FUNC)
            continue;

        // Register correct entry point in symtab + register function in vm
        symbol *func = symtab_lookup(prog_symtab, node->data.func.name);
        int func_pc = prog_next_pc(prog);
        func->func.pc = func_pc;
        prog_register_function(prog, node->data.func.name, func_pc);
        printf("Generating function %s at pc=%d\n",
            node->data.func.name, func_pc);

        // Enter function scope
        symtab_enter_scope(prog_symtab);
        // The CALL/CALL_PC opcodes creates a stack frame so variables should start from zero again
        prog_symtab->current->var_offset = 0;

        // Parameters become locals 0..n-1
        for (ast_node *p = node->data.func.params; p; p = p->next) {
            symtab_add_var(prog_symtab, p->data.param.name, p->data.param.type);
        }

        // Generate function body
        codegen_stmt(node->data.func.body);

        // Implicit return for void functions
        if (node->data.func.return_type == TYPE_VOID) {
            prog_add_op(prog, RET);
        }

        // "Free" the ids of the current scope
        symtab_exit_scope(prog_symtab);
    }

    // start JUMP - validation that start exists after ast creation
    symbol *start = symtab_lookup(prog_symtab, "start");
    prog_set_num(prog, start_pc, start->func.pc);

    // dump symbol table
    printf("\n");
    symtab_dump(prog_symtab);

    prog_dump(prog);

    // Write to file
    if (prog_write(prog, filename)) {
        printf("\nBytecode written to %s\n", filename);
    } else {
        fprintf(stderr, "Error writing bytecode to %s\n", filename);
    }

    symtab_free(prog_symtab);
    printf("Code generation complete.\n");
}
