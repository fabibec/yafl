#include "logger.h"
#include "vector.h"
#include <stdbool.h>
#include <stdlib.h>

#define VEC_INITIAL_CAPACITY 16

void vector_init(vector *v, vector_destructor destroy) {
    v->size = 0;
    v->capacity = VEC_INITIAL_CAPACITY;
    v->destroy = destroy;
    v->data = malloc(sizeof(void *) * v->capacity);
}

void vector_push(vector *v, void *item) {
    if (v->size == v->capacity) {
        v->capacity *= 2;
        v->data = realloc(v->data, sizeof(void *) * v->capacity);
        if(!v->data) log_error(NO_LINE, "Realloc failed in vector.");
    }
    v->data[v->size++] = item;
}

void *vector_get(vector *v, size_t index) {
    if (index >= v->size) return NULL;
    return v->data[index];
}

void *vector_pop(vector *v) {
    if (v->size == 0) return NULL;
    return v->data[--v->size];
}

size_t vector_size(vector *v) {
    return v->size;
}

bool vector_is_empty(vector *v) {
    return !v->size;
}

void vector_free(vector *v) {
    if (v->destroy) {
        for (size_t i = 0; i < v->size; i++) {
            v->destroy(v->data[i]);
        }
    }
    free(v->data);
}
