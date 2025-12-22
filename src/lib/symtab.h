#ifndef _SYMTAB_H_
#define _SYMTAB_H_
#include "types.h"

// Size of symbol table hashmap
#define SYMTAB_SIZE 1024

// Single symbol
typedef struct symbol {
    char *name;
    yafl_t type;
    union {
        // variable number
        int var_nr;
        struct {
            // program counter
            int pc;
            // Return type
            yafl_t ret_type;
        } func;
    };
    // For hash collision chaining
    struct symbol *next;
} symbol;

/* Hashmap for one scope */
typedef struct hashmap {
    symbol **buckets;
    int capacity;
    int size;
    // Threshold for resizing (e.g., 0.75)
    float load_factor;
} hashmap;

/* Scope - linked list of hash maps */
typedef struct scope {
    hashmap *map;
    struct scope *parent;
    // vars in current scope (start from 0)
    int var_count;
    int level;
} scope;

/* Symbol table state */
typedef struct symtab {
    // Current scope
    scope *current;
    int total_scopes;
} symtab;

/* --- Hash Map --- */
hashmap *hashmap_create(int initial_capacity, float load_factor);
void hashmap_free(hashmap *map);

symbol *hashmap_put(hashmap *map, const char *name, yafl_t type);
symbol *hashmap_get(hashmap *map, const char *name);

void hashmap_dump(hashmap *map, int scope_level);

/* --- Symbol Table --- */
symtab *symtab_create(void);
void symtab_free(symtab *table);
void symtab_enter_scope(symtab *table);
void symtab_exit_scope(symtab *table);

symbol *symtab_add_var(symtab *table, const char *name, yafl_t type);
symbol *symtab_add_func(symtab *table, const char *name, yafl_t ret_type, int pc);
symbol *symtab_lookup(symtab *table, const char *name);
symbol *symtab_lookup_current_scope(symtab *table, const char *name);

void symtab_dump(symtab *table);

#endif // _SYMTAB_H_
