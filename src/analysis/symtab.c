#include "ast.h"
#include "builtins.h"
#include "symtab.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void var_sym_free(void *ptr) {
    var_sym *sym = (var_sym*)ptr;
    free(sym->name);
    type_free(sym->type);
    free(sym);
}

static void func_sym_free(void *ptr) {
    func_sym *sym = (func_sym*)ptr;

    // Free entire overload chain
    while (sym) {
        func_sym *next = sym->next_overload;
        free(sym->name);
        type_free(sym->ret_type);
        if (sym->param_types) {
             for(int i = 0; i < sym->num_params; i++){
                type_free(sym->param_types[i]);
            }
            free(sym->param_types);
        }
        if (sym->default_values) {
            // We own the default value AST nodes (created in builtins.c)
            if (sym->is_builtin) {
                for (int i = 0; i < sym->num_params; i++) {
                    ast_free(sym->default_values[i]);
                }
            }
            free(sym->default_values);
        }

        // Free fixups
        fixup_node *f = sym->fixups;
        while (f) {
            fixup_node *next_f = f->next;
            free(f);
            f = next_f;
        }

        free(sym);
        sym = next;
    }
}

void symtab_add_fixup(func_sym *sym, int pc_location) {
    if (!sym) return;
    fixup_node *node = malloc(sizeof(fixup_node));
    node->pc_location = pc_location;
    node->next = sym->fixups;
    sym->fixups = node;
}

symtab *symtab_create(void) {
    symtab *table = malloc(sizeof(symtab));
    table->total_scopes = 0;

    // Create global variable scope
    table->current = malloc(sizeof(scope));
    table->current->vars = hashmap_create(16, 0.75f, var_sym_free);
    table->current->parent = NULL;
    table->current->level = 0;
    table->current->var_count = 0;
    table->current->var_offset = 0;

    table->funcs = hashmap_create(32, 0.75f, func_sym_free);

    builtins_register(table);

    return table;
}

void symtab_free(symtab *table) {
    if (!table) return;

    // Destroy all scopes
    while (table->current) {
        scope *parent = table->current->parent;
        hashmap_free(table->current->vars);
        free(table->current);
        table->current = parent;
    }

    // Destroy function table
    hashmap_free(table->funcs);

    free(table);
}

void symtab_enter_scope(symtab *table) {
    scope *new_scope = malloc(sizeof(scope));
    // Smaller for local scopes
    new_scope->vars = hashmap_create(8, 0.75f, var_sym_free);
    new_scope->parent = table->current;
    new_scope->level = table->current->level + 1;
    new_scope->var_count = 0;
    new_scope->var_offset = table->current->var_offset + table->current->var_count;

    table->current = new_scope;
    table->total_scopes++;
}

void symtab_exit_scope(symtab *table) {
    // Error when accidentally trying to exit global scope
    if (!table->current || !table->current->parent) {
        fprintf(stderr, "Error: Cannot exit global scope\n");
        return;
    }

    scope *old_scope = table->current;
    table->current = old_scope->parent;

    hashmap_free(old_scope->vars);
    free(old_scope);
}

var_sym *symtab_add_var(symtab *table, const char *name, yafl_t *type) {
    // Check if already exists in current scope -> error in codegen
    if (hashmap_get(table->current->vars, name)) {
        return NULL;
    }

    var_sym *sym = malloc(sizeof(var_sym));
    sym->name = strdup(name);
    sym->type = type_clone(type);
    sym->nr = table->current->var_offset + table->current->var_count++;
    sym->is_global = (table->current->level == 0);

    if(hashmap_put(table->current->vars, name, sym)) {
        var_sym_free(sym);
        return NULL;
    }
    return sym;
}

var_sym *symtab_lookup_var(symtab *table, const char *name) {
    for (scope *scope = table->current; scope; scope = scope->parent) {
        var_sym *sym = hashmap_get(scope->vars, name);
        if (sym) {
            return sym;
        }
    }
    return NULL;
}

func_sym *symtab_add_func(symtab *table, const char *name, yafl_t *ret_type,
        int num_params, yafl_t **param_types, struct ast_node **default_values, int pc) {

    func_sym *existing = hashmap_get(table->funcs, name);

    // Check if this exact signature already exists
    for (func_sym *overload = existing; overload; overload = overload->next_overload) {
        if (overload->num_params == num_params) {
            int same = 1;
            for (int i = 0; i < num_params; i++) {
                if (!type_is_identical(overload->param_types[i], param_types[i])) {
                    same = 0;
                    break;
                }
            }
            if (same) {
                return NULL;
            }
        }
    }

    func_sym *sym = malloc(sizeof(func_sym));
    sym->name = strdup(name);
    sym->ret_type = type_clone(ret_type);
    sym->num_params = num_params;
    sym->is_builtin = 0;
    sym->impl.pc = pc;
    sym->next_overload = NULL;
    sym->fixups = NULL;

    // Insert param types
    if (param_types && num_params > 0) {
        sym->param_types = malloc(num_params * sizeof(yafl_t *));
        for(int i=0; i<num_params; i++) {
            sym->param_types[i] = type_clone(param_types[i]);
        }
    } else {
        sym->param_types = NULL;
    }

    // Insert default values
    if (default_values && num_params > 0) {
        sym->default_values = malloc(num_params * sizeof(ast_node*));
        for (int i = 0; i < num_params; i++) {
            // Store pointer, do not clone AST node
            sym->default_values[i] = default_values[i];
        }
    } else {
        sym->default_values = NULL;
    }

    if (!existing) {
        // First function with this name
        if (hashmap_put(table->funcs, name, sym)) {
            func_sym_free(sym);
            return NULL;
        }
    } else {
        // Add to overload chain
        func_sym *last = existing;
        while (last->next_overload) {
            last = last->next_overload;
        }
        last->next_overload = sym;
    }

    return sym;
}

func_sym *symtab_add_builtin(symtab *table, const char *name, yafl_t* ret_type,
                               int num_params, yafl_t **param_types, struct ast_node **default_values, codegen_fn codegen) {
    func_sym *existing = hashmap_get(table->funcs, name);

   // Check if this exact signature already exists
    for (func_sym *overload = existing; overload; overload = overload->next_overload) {
        if (overload->num_params == num_params) {
            int same = 1;
            for (int i = 0; i < num_params; i++) {
                if (!type_is_identical(overload->param_types[i], param_types[i])) {
                    same = 0;
                    break;
                }
            }
            if (same) {
                return NULL;
            }
        }
    }

    func_sym *sym = malloc(sizeof(func_sym));
    sym->name = strdup(name);
    sym->ret_type = type_clone(ret_type);
    sym->num_params = num_params;
    sym->is_builtin = 1;
    sym->impl.codegen = codegen;
    sym->next_overload = NULL;
    sym->fixups = NULL;

    // Insert param types
    if (param_types && num_params > 0) {
        sym->param_types = malloc(num_params * sizeof(yafl_t *));
        for(int i=0; i<num_params; i++) {
            sym->param_types[i] = type_clone(param_types[i]);
        }
    } else {
        sym->param_types = NULL;
    }

    // Insert default values
    if (default_values && num_params > 0) {
        sym->default_values = malloc(num_params * sizeof(ast_node*));
        for (int i = 0; i < num_params; i++) {
            // Store pointer
            sym->default_values[i] = default_values[i];
        }
    } else {
        sym->default_values = NULL;
    }

    if (!existing) {
        // First function with this name
        if (hashmap_put(table->funcs, name, sym)) {
            func_sym_free(sym);
            return NULL;
        }
    } else {
        // Add to overload chain
        func_sym *last = existing;
        while (last->next_overload) {
            last = last->next_overload;
        }
        last->next_overload = sym;
    }

    return sym;
}

static int type_score(yafl_t *target, yafl_t *actual) {
    if (!target || !actual) return 0;

    // Generic match is weak
    if (target->base_t == TYPE_GENERIC) return 1;

    // Recursive check for composite types
    if (target->base_t == TYPE_ARR) {
        if (actual->base_t != TYPE_ARR) return 0;
        int inner = type_score(target->comp_t, actual->comp_t);
        return inner > 0 ? inner + 1 : 0; // Add 1 for the array layer itself matching
    }

    // Exact base match
    if (target->base_t == actual->base_t) return 2;

    return 0;
}

func_sym *symtab_lookup_func(symtab *table, const char *name,
                                        yafl_t **arg_types, int num_args) {
    func_sym *func = hashmap_get(table->funcs, name);

    if (!func) {
        return NULL;
    }

    func_sym *best_match = NULL;
    int max_score = -1;

    // Find best matching overload
    for (func_sym *overload = func; overload; overload = overload->next_overload) {
        // If args are less than required, default values for the rest
        if (num_args > overload->num_params) {
            continue;
        }

        // Check if missing args have defaults
        if (num_args < overload->num_params) {
            if (!overload->default_values) continue;
            int missing_ok = 1;
            for (int i = num_args; i < overload->num_params; i++) {
                if (!overload->default_values[i]) {
                    missing_ok = 0;
                    break;
                }
            }
            if (!missing_ok) continue;
        }

        int current_score = 0;
        int matches = 1;
        for (int i = 0; i < num_args; i++) {
            int score = type_score(overload->param_types[i], arg_types[i]);
            if (score == 0) {
                matches = 0;
                break;
            }
            current_score += score;
        }

        if (matches) {
            if (current_score > max_score) {
                max_score = current_score;
                best_match = overload;
            }
        }
    }
    return best_match;
}

void symtab_dump(symtab *table) {
    printf("--- Symbol Table Dump ---\n");
    printf("Total scopes created: %d\n", table->total_scopes);

    printf("\n[Functions]\n");
    char type_buf[128];
    hashmap *map = table->funcs;
    for (int i = 0; i < map->capacity; i++) {
        hashmap_entry *entry = map->buckets[i];
        while (entry) {
            func_sym *sym = (func_sym*)entry->value;
            // Traverse overloads
            while (sym) {
                type_to_str(sym->ret_type, type_buf, sizeof(type_buf));
                printf("  %s (%s): params=%d ", sym->name, type_buf, sym->num_params);

                if (sym->num_params > 0 && sym->param_types) {
                    printf("(");
                    for (int j = 0; j < sym->num_params; j++) {
                        type_to_str(sym->param_types[j], type_buf, sizeof(type_buf));
                        printf("%s%s", type_buf, (j < sym->num_params - 1) ? ", " : "");
                    }
                    printf(") ");
                }

                if (sym->is_builtin) {
                    printf("[BUILTIN]");
                } else {
                    printf("[USER PC=%d]", sym->impl.pc);
                    if (sym->fixups) {
                        printf(" [FIXUPS PENDING]");
                    }
                }
                printf("\n");
                sym = sym->next_overload;
            }
            entry = entry->next;
        }
    }
    printf("-------------------------\n");
}
