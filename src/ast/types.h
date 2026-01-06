#ifndef _TYPES_H_
#define _TYPES_H_
#include <stddef.h>
/* Type system for the yafl lang */

typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_STR,
    TYPE_FUNC,
    TYPE_SINT,
    TYPE_FLOAT,
    TYPE_ARR,
    TYPE_RANGE,
    TYPE_GENERIC,
} yafl_base_t;

// nested types for elements
typedef struct yafl_t {
    yafl_base_t base_t;
    // Inner type for arrays
    struct yafl_t *comp_t;
} yafl_t;

/* Constructors */
yafl_t* type_new_simple(yafl_base_t base_t);
yafl_t* type_new_composite(yafl_t *element_type);

/* Destructor */
void type_free(yafl_t *t);
void type_list_free(yafl_t **t_list, int len);

/* Utilities */
int type_equals(yafl_t *t1, yafl_t *t2);
int type_is_identical(yafl_t *t1, yafl_t *t2);
void type_to_str(const yafl_t *t, char *buf, size_t len);
yafl_t* type_clone(yafl_t *t);

#endif
