#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

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

hashmap *hashmap_create(int initial_capacity, float load_factor, hashmap_destructor destroy) {
    hashmap *map = malloc(sizeof(hashmap));
    map->capacity = initial_capacity;
    map->size = 0;
    map->load_factor = load_factor;
    map->destroy = destroy;
    map->buckets = calloc(initial_capacity, sizeof(hashmap_entry*));
    return map;
}

void hashmap_free(hashmap *map) {
    if (!map) return;

    for (int i = 0; i < map->capacity; i++) {
        hashmap_entry *entry = map->buckets[i];
        while (entry) {
            hashmap_entry *next = entry->next;
            free(entry->key);
            // Call custom value destructor
            if (map->destroy) {
                map->destroy(entry->value);
            }
            free(entry);
            entry = next;
        }
    }

    free(map->buckets);
    free(map);
}

int hashmap_put(hashmap *map, const char *key, void *value) {
    // Check if key already exists
    if (hashmap_get(map, key) != NULL) {
        return -1;
    }

    // Check for resize
    if ((float)map->size / map->capacity > map->load_factor) {
        hashmap_resize(map);
    }

    unsigned int idx = hash(key) % map->capacity;

    hashmap_entry *entry = malloc(sizeof(hashmap_entry));
    entry->key = strdup(key);
    entry->value = value;
    entry->next = map->buckets[idx];

    map->buckets[idx] = entry;
    map->size++;

    return 0;
}

void *hashmap_get(hashmap *map, const char *key) {
    unsigned int idx = hash(key) % map->capacity;

    for (hashmap_entry *entry = map->buckets[idx]; entry; entry = entry->next) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
    }
    return NULL;
}

void hashmap_resize(hashmap *map) {
    int old_capacity = map->capacity;
    int new_capacity = old_capacity * 2;
    hashmap_entry **old_buckets = map->buckets;

    map->buckets = calloc(new_capacity, sizeof(hashmap_entry*));
    map->capacity = new_capacity;
    map->size = 0;

    // Rehash all entries
    for (int i = 0; i < old_capacity; i++) {
        hashmap_entry *entry = old_buckets[i];
        while (entry) {
            hashmap_entry *next = entry->next;

            // Reinsert
            unsigned int idx = hash(entry->key) % new_capacity;
            entry->next = map->buckets[idx];
            map->buckets[idx] = entry;
            map->size++;

            entry = next;
        }
    }

    free(old_buckets);
}
