#ifndef _SYMTAB_H_
#define _SYMTAB_H_
#include "ast.h"
#include "hashmap.h"
#include "types.h"
#include <stdbool.h>
#include <prog.h>

typedef struct {
    char *name;
    yafl_t *type;
    int nr;
    bool is_global;
} var_sym;

typedef struct func_sym {
    char *name;
    yafl_t** param_types;
    struct ast_node **default_values;
    int num_params;
    bool is_builtin;
    union {
        // User function: program counter
        int pc;
        // Builtin: A C function pointer that writes the bytecode
        void (*codegen_fn)(prog_t *p, ast_node *node, struct func_sym *sym, int arg_count);
    } impl;
    struct func_sym *next_overload;
    yafl_t *ret_type;
    struct fixup_node *fixups;
} func_sym;

typedef struct fixup_node {
    int pc_location;
    struct fixup_node *next;
} fixup_node;

/* Scope - linked list of hash maps */
typedef struct scope {
    hashmap *vars;
    struct scope *parent;
    // vars in current scope (start from 0)
    int var_count;
    int level;
    int var_offset;
} scope;

/* Symbol table state */
typedef struct symtab {
    // Current scope
    scope *current;
    hashmap* funcs;
    int total_scopes;
} symtab;

/* --- Symbol Table --- */
symtab *symtab_create(void);
void symtab_free(symtab *table);
void symtab_enter_scope(symtab *table);
void symtab_exit_scope(symtab *table);

var_sym *symtab_add_var(symtab *table, const char *name, yafl_t *type);
var_sym *symtab_lookup_var(symtab *table, const char *name);

func_sym *symtab_add_func(symtab *table, const char *name, yafl_t *ret_type,
                           int num_params, yafl_t **param_types, struct ast_node **default_values, int pc);

void symtab_add_fixup(func_sym *sym, int pc_location);

func_sym *symtab_add_builtin(symtab *table, const char *name, yafl_t* ret_type,
                               int num_params, yafl_t **param_types, struct ast_node **default_values,
                               void (*codegen_fn)(prog_t *p, ast_node *node, func_sym *sym, int arg_count));

func_sym *symtab_lookup_func(symtab *table, const char *name,
                                        yafl_t **arg_types, int num_args);

void symtab_dump(symtab *table);

#endif // _SYMTAB_H_
