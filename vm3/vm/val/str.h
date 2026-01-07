#ifndef _STR_H
#define _STR_H

#include "val.h"

struct str {
  char *buf;
  int len;
  int cap;
};

typedef struct str str_t;

void val_register_str(void);

val_t *v_str_create (void);
val_t *v_str_new_cstr (const char *cstr);
val_t *v_str_new_buf (const char *buf, int len);

str_t *str_add_buf (str_t *str, const char *buf, int len);
str_t *str_add_cstr (str_t *str, const char *cstr);


char *cstr (val_t *v);


#endif
