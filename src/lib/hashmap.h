#ifndef _HASHMAP_H_
#define _HASHMAP_H_

/* Generic hash map entry */
typedef struct hashmap_entry {
    char *key;
    void *value;
    struct hashmap_entry *next;
} hashmap_entry;

/* Generic hash map */
typedef struct hashmap {
    hashmap_entry **buckets;
    int capacity;
    int size;
    float load_factor;
    // Threshold for resizing (e.g., 0.75)
    // value destructor for cleanup
    void (*value_destructor)(void *value);
} hashmap;


hashmap *hashmap_create(int initial_capacity, float load_factor, void (*value_destructor)(void*));
void hashmap_free(hashmap *map);

int hashmap_put(hashmap *map, const char *key, void *value);
void *hashmap_get(hashmap *map, const char *key);

void hashmap_resize(hashmap *map);

#endif /* HASHMAP_H */
