#ifndef _VECTOR_H_
#define _VECTOR_H_
#include <stdbool.h>
#include <stddef.h>
/* Simple dynamic array with destructor */

typedef void (*vector_destructor)(void *);

typedef struct {
    void **data;
    size_t size;
    size_t capacity;
    vector_destructor destroy;
} vector;

void vector_init(vector *v, vector_destructor destroy);
void vector_push(vector *v, void *item);
void *vector_get(vector *v, size_t index);
void *vector_pop(vector *v);
size_t vector_size(vector *v);
bool vector_is_empty(vector *v);
void vector_free(vector *v);

#endif // _VECTOR_H_


