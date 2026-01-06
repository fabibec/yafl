#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "types.h"

yafl_t* type_new_simple(yafl_base_t base_t) {
    yafl_t *t = malloc(sizeof(yafl_t));
    t->base_t = base_t;
    t->comp_t = NULL;
    return t;
}

yafl_t* type_new_composite(yafl_t *element_type) {
    yafl_t *t = malloc(sizeof(yafl_t));
    t->base_t = TYPE_ARR;
    t->comp_t = element_type;
    return t;
}

void type_free(yafl_t *t) {
    if (!t) return;
    if (t->comp_t != NULL) {
        type_free(t->comp_t);
    }
    free(t);
}

void type_list_free(yafl_t **t_list, int len) {
    for (int i = 0; i < len; i++) {
        type_free(t_list[i]);
    }
    free(t_list);
}

int type_equals(yafl_t *t1, yafl_t *t2) {
    if (!t1 || !t2) return 0;

    // Generic matches anything
    if (t1->base_t == TYPE_GENERIC || t2->base_t == TYPE_GENERIC) return 1;

    if (t1->base_t != t2->base_t) return 0;

    if (t1->base_t == TYPE_ARR) {
        return type_equals(t1->comp_t, t2->comp_t);
    }
    return 1;
}

int type_is_identical(yafl_t *t1, yafl_t *t2) {
    if (!t1 || !t2) return 0;
    if (t1->base_t != t2->base_t) return 0;
    if (t1->base_t == TYPE_ARR) {
        return type_is_identical(t1->comp_t, t2->comp_t);
    }
    return 1;
}

yafl_t* type_clone(yafl_t *t) {
    if (!t) return NULL;
    if (t->base_t == TYPE_ARR) {
        return type_new_composite(type_clone(t->comp_t));
    }
    return type_new_simple(t->base_t);
}

void type_to_str(const yafl_t *t, char *buf, size_t len) {
    if (!t || !buf || len == 0) return;

    switch (t->base_t) {
        case TYPE_VOID: snprintf(buf, len, "none"); break;
        case TYPE_BOOL: snprintf(buf, len, "bool"); break;
        case TYPE_STR:  snprintf(buf, len, "str"); break;
        case TYPE_FUNC: snprintf(buf, len, "func"); break;
        case TYPE_SINT: snprintf(buf, len, "int"); break;
        case TYPE_FLOAT:snprintf(buf, len, "float"); break;
        case TYPE_GENERIC:snprintf(buf, len, "any"); break;
        case TYPE_RANGE: snprintf(buf, len, "range"); break;
        case TYPE_ARR: {
            char inner[128];
            if (t->comp_t) {
                type_to_str(t->comp_t, inner, sizeof(inner));
                snprintf(buf, len, "arr'%s", inner);
            } else {
                snprintf(buf, len, "arr'?");
            }
            break;
        }
        default: snprintf(buf, len, "unknown"); break;
    }
}
