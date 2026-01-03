#include "symtab.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void var_sym_free(void *ptr) {
    var_sym *sym = (var_sym*)ptr;
    // printf("Freeing var %s\n", sym->name);
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
        free(sym);
        sym = next;
    }
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
                           int num_params, yafl_t **param_types, int pc) {
    func_sym *existing = hashmap_get(table->funcs, name);

    // Check if this exact signature already exists
    for (func_sym *overload = existing; overload; overload = overload->next_overload) {
        if (overload->num_params == num_params) {
            int same = 1;
            for (int i = 0; i < num_params; i++) {
                if (!type_equals(overload->param_types[i], param_types[i])) {
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

    // Insert param types
    if (param_types && num_params > 0) {
        sym->param_types = malloc(num_params * sizeof(yafl_t *));
        for(int i=0; i<num_params; i++) {
            sym->param_types[i] = type_clone(param_types[i]);
        }
    } else {
        sym->param_types = NULL;
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
                               int num_params, yafl_t **param_types, void (*codegen_fn)(prog_t *p)) {
    func_sym *existing = hashmap_get(table->funcs, name);

   // Check if this exact signature already exists
    for (func_sym *overload = existing; overload; overload = overload->next_overload) {
        if (overload->num_params == num_params) {
            int same = 1;
            for (int i = 0; i < num_params; i++) {
                if (!type_equals(overload->param_types[i], param_types[i])) {
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
    sym->impl.codegen_fn = codegen_fn;
    sym->next_overload = NULL;

    // Insert param types
    if (param_types && num_params > 0) {
        sym->param_types = malloc(num_params * sizeof(yafl_t *));
        for(int i=0; i<num_params; i++) {
            sym->param_types[i] = type_clone(param_types[i]);
        }
    } else {
        sym->param_types = NULL;
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

func_sym *symtab_lookup_func(symtab *table, const char *name,
                                        yafl_t **arg_types, int num_args) {
    func_sym *func = hashmap_get(table->funcs, name);

    if (!func) {
        return NULL;
    }

    // Find matching overload
    for (func_sym *overload = func; overload; overload = overload->next_overload) {
        if (overload->num_params != num_args) {
            continue;
        }

        int matches = 1;
        for (int i = 0; i < num_args; i++) {
            if (!type_equals(overload->param_types[i], arg_types[i])) {
                matches = 0;
                break;
            }
        }

        if (matches) {
            return overload;
        }
    }
    return NULL;
}

void symtab_dump(symtab *table) {
    printf("--- Symbol Table Dump ---\n");
    printf("Total scopes created: %d\n", table->total_scopes);
    printf("Current scope depth: %d\n", table->current->level);
    printf("Current scope var counter: %d\n", table->current->var_count);
    printf("\n");

    // Print from current scope to global
    int scope_count = 0;
    for (scope *scope = table->current; scope; scope = scope->parent) {
        // hashmap_dump(scope->vars, scope->level);
        scope_count++;
    }

    printf("\nTotal active scopes: %d\n", scope_count);
    printf("-------------------------\n");
}
