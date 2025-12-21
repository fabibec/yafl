#include "symtab.h"
#include "utils.c"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
    djb2 hash algorithm
    See: http://www.cse.yorku.ca/~oz/hash.html
*/
static unsigned int hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

/* --- Hash Map --- */
hashmap *hashmap_create(int initial_capacity, float load_factor) {
    hashmap *map = malloc(sizeof(hashmap));
    map->capacity = initial_capacity;
    map->size = 0;
    map->load_factor = load_factor;
    map->buckets = calloc(initial_capacity, sizeof(symbol*));
    return map;
}

void hashmap_free(hashmap *map) {
    if (!map) return;

    // Free all symbols in all buckets
    for (int i = 0; i < map->capacity; i++) {
        symbol *sym = map->buckets[i];
        while (sym) {
            symbol *next = sym->next;
            free(sym->name);
            free(sym);
            sym = next;
        }
    }

    free(map->buckets);
    free(map);
}

symbol *hashmap_put(hashmap *map, const char *name, yafl_t type) {
    // Check if resize needed
    if ((float)map->size / map->capacity > map->load_factor) {
        _hashmap_resize(map);
    }

    unsigned int idx = hash(name) % map->capacity;

    // Check if already exists
    for (symbol *s = map->buckets[idx]; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            return NULL;
        }
    }

    symbol *sym = malloc(sizeof(symbol));
    sym->name = strdup(name);
    sym->type = type;
    sym->next = map->buckets[idx];

    map->buckets[idx] = sym;
    map->size++;

    return sym;
}

symbol *hashmap_get(hashmap *map, const char *name) {
    unsigned int idx = hash(name) % map->capacity;

    for (symbol *s = map->buckets[idx]; s; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            return s;
        }
    }

    return NULL;
}

void hashmap_dump(hashmap *map, int scope_level) {
    printf("  Scope %d (size=%d, capacity=%d, load=%.2f):\n",
           scope_level, map->size, map->capacity,
           (float)map->size / map->capacity);

    for (int i = 0; i < map->capacity; i++) {
        for (symbol *s = map->buckets[i]; s; s = s->next) {
            printf("    [%d] %s: %s", i, s->name, type_to_string(s->type));
            if (s->type == TYPE_FUNC) {
                printf(" (pc=%d, ret=%s)", s->func.pc, type_to_string(s->func.ret_type));
            } else {
                printf(" (var_nr=%d)", s->var_nr);
            }
            printf("\n");
        }
    }
}

void _hashmap_resize(hashmap *map) {
    int old_capacity = map->capacity;
    int new_capacity = old_capacity * 2;
    symbol **old_buckets = map->buckets;

    // Allocate new buckets
    map->buckets = calloc(new_capacity, sizeof(symbol*));
    map->capacity = new_capacity;
    map->size = 0;

    // Rehash all symbols
    for (int i = 0; i < old_capacity; i++) {
        symbol *sym = old_buckets[i];
        while (sym) {
            symbol *next = sym->next;

            // Reinsert into new table
            unsigned int idx = hash(sym->name) % new_capacity;
            sym->next = map->buckets[idx];
            map->buckets[idx] = sym;
            map->size++;

            sym = next;
        }
    }

    free(old_buckets);
    printf("  [HashMap resized: %d -> %d buckets]\n", old_capacity, new_capacity);
}

/* --- Symbol Table --- */
symtab *symtab_create(void) {
    symtab *table = malloc(sizeof(symtab));
    table->total_scopes = 0;

    // Create global scope
    table->current = malloc(sizeof(symtab));
    table->current->map = hashmap_create(16, 0.75f);
    table->current->parent = NULL;
    table->current->level = 0;

    return table;
}

void symtab_free(symtab *table) {
    if (!table) return;

    // Pop all scopes
    while (table->current) {
        scope *parent = table->current->parent;
        hashmap_free(table->current->map);
        free(table->current);
        table->current = parent;
    }

    free(table);
}

void symtab_enter_scope(symtab *table) {
    scope *new_scope = malloc(sizeof(scope));
    // Smaller for local scopes
    new_scope->map = hashmap_create(8, 0.75f);
    new_scope->parent = table->current;
    new_scope->level = table->current->level + 1;

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

    hashmap_free(old_scope->map);
    free(old_scope);
}

symbol *symtab_add_var(symtab *table, const char *name, yafl_t type) {
    // Check if already exists in current scope -> error in codegen
    if (hashmap_get(table->current->map, name)) {
        return NULL;
    }

    symbol *sym = hashmap_put(table->current->map, name, type);
    if (!sym) return NULL;

    // TODO add a counter maybe
    sym->var_nr = -1;

    return sym;
}

symbol *symtab_add_func(symtab *table, const char *name, yafl_t ret_type, int pc) {
    // Functions are always added to global scope
    scope *global = table->current;
    while (global->parent) {
        global = global->parent;
    }

    // Check if already exists
    if (hashmap_get(global->map, name)) {
        return NULL;  // Already declared
    }

    symbol *sym = hashmap_put(global->map, name, TYPE_FUNC);
    if (!sym) return NULL;

    sym->func.pc = pc;
    sym->func.ret_type = ret_type;

    return sym;
}

symbol *symtab_lookup(symtab *table, const char *name) {
    // Search from current scope up to global
    for (scope *scope = table->current; scope; scope = scope->parent) {
        symbol *sym = hashmap_get(scope->map, name);
        if (sym) {
            return sym;
        }
    }

    return NULL;  // Not found in any scope
}

symbol *symtab_lookup_current_scope(symtab *table, const char *name) {
    return hashmap_get(table->current->map, name);
}

void symtab_dump(symtab *table) {
    printf("--- Symbol Table Dump ---\n");
    printf("Total scopes created: %d\n", table->total_scopes);
    printf("Current scope depth: %d\n", table->current->level);
    printf("\n");

    // Print from current scope to global
    int scope_count = 0;
    for (scope *scope = table->current; scope; scope = scope->parent) {
        hashmap_dump(scope->map, scope->level);
        scope_count++;
    }

    printf("\nTotal active scopes: %d\n", scope_count);
    printf("-------------------------\n");
}
