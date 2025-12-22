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
static int prog_var_counter = 0;

/* Forward declarations */
static void codegen_stmt(ast_node *node);
static void codegen_expr(ast_node *node);

/* --- Type Checking --- */
yafl_t get_expr_type(symtab *table, ast_node *node) {
    if (!node) return TYPE_VOID;

    switch (node->type) {
        case NODE_INT:
            return TYPE_SINT;
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
        default:
            return TYPE_VOID;
    }
}

/* --- ERROR REPORTING --- */
void codegen_error(int line, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Yafl codegen error (line %d): ", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

/* --- Codegen --- */
static void codegen_expr(ast_node *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_INT:
            prog_add_num(prog, node->data.integer.value);
            break;

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
            symbol *sym = symtab_lookup(prog_symtab, node->data.var.name);
            if (!sym) {
                codegen_error(node->line, "Variable '%s' not declared",
                            node->data.var.name);
            }
            prog_add_num(prog, sym->var_nr);
            prog_add_op(prog, GETVAR);
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
            switch (node->data.unary.op) {
                case OP_NEG: prog_add_op(prog, NEG); break;
                case OP_NOT: prog_add_op(prog, NOT); break;
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
                switch (node->data.decl.type)
                {
                    case TYPE_STR:
                        // Empty String for string type
                        val_t *str = v_str_create();
                        int const_id = prog_new_constant(prog, str);
                        prog_add_num(prog, const_id);
                        prog_add_op(prog, CONSTANT);
                        break;
                    case TYPE_BOOL:
                    case TYPE_SINT:
                    case TYPE_UINT:
                        prog_add_num(prog, 0);
                    default:
                        codegen_error(node->line, "Zero-init on type '%s' is not implemented", type_to_string(node->data.decl.type));
                        break;
                }
            }

            // Add variable - var_nr is relative in symtab for codegen we have a global counter
            symbol *sym = symtab_add_var(prog_symtab, node->data.decl.name, node->data.decl.type);
            if (!sym) {
                codegen_error(node->line, "Variable '%s' already declared", node->data.decl.name);
            }
            sym->var_nr = prog_var_counter++;

            prog_add_num(prog, sym->var_nr);
            prog_add_op(prog, SETVAR);
            break;
        }

        case NODE_ASSIGN: {
            symbol *sym = symtab_lookup(prog_symtab, node->data.assign.name);
            if (!sym) {
                codegen_error(node->line, "Variable '%s' not declared",
                            node->data.assign.name);
            }

            codegen_expr(node->data.assign.value);
            prog_add_num(prog, sym->var_nr);
            prog_add_op(prog, SETVAR);
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
            prog_add_op(prog, JUMPF);
            int else_jmp_pc = prog_add_num(prog, -1);

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
            // TODO: Implement for loops
            codegen_error(node->line, "For loops not yet implemented");
            break;
        }

        case NODE_CALL:
            codegen_expr(node);
            prog_add_op(prog, DISCARD);  // Discard return value
            break;

        default:
            // Try as expression
            codegen_expr(node);
            prog_add_op(prog, DISCARD);
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
        }
        // TODO: codegen for global vars

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
    }

    printf("Registered %d functions\n", prog_symtab->current->map->size);

    // Second pass: Generate actual function bodies
    for (ast_node *node = root; node; node = node->next) {
        // Everything else done in first pass
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
        prog_var_counter -= prog_symtab->current->var_count;
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
