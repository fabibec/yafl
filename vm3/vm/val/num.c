#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

#include "prog.h"
#include "val.h"
#include "str.h"


val_t *v_num_new_int (int i) {
  val_t *v = val_new(T_NUM);
  v->u.num = i;

  return v;
}


val_t *v_num_create (void) {
  val_t *v = val_new(T_NUM);
  v->u.num = 0;

  return v;
}

val_t *v_num_copy (val_t *v) {
  val_t *v2 = val_new(T_NUM);
  v2->u.num = v->u.num;

  return v2;
}

int v_num_cmp (val_t *v1, val_t *v2) {
  return v1->u.num - v2->u.num;
}

val_t *v_num_to_string (val_t *num) {
  int l = snprintf(NULL, 0, "%d", num->u.num);
  char buf[l+1];
  snprintf(buf, sizeof(buf), "%d", num->u.num);
  return v_str_new_cstr(buf);
}

/* add the ability to parse lexer like ints */
static int parse_yafl_int(const char *s, int *out) {
    int base = 10;
    const char *p = s;

    const char *hash = strchr(s, '#');
    if (hash) {
        if (hash == s) {
            base = 16;
            p = hash + 1;
        } else {
            base = parse_base(s);
            if (!base) return 0; // Error
            p = hash + 1;
        }
    }

    return parse_based_int(base, p, out);
}

val_t *v_num_conv (val_t *v) {
 char *ptr;
 switch (v->type) {
   case T_STR:
     if (v->u.str->len == 1) {
       return v_num_new_int((unsigned char)v->u.str->buf[0]);
     }
     ptr = v->u.str->buf;
     int nr;
     int ok = parse_yafl_int(ptr, &nr);
     if (!ok) {
       return &val_undef;
     }
     return v_num_new_int(nr);
   /* Add support for real -> num */
   case T_REAL:
     return v_num_new_int((int)v->u.real);
   default:
    return &val_undef;
 }
}

int v_num_to_bool (val_t *v) {
  return v->u.num != 0;
}

void v_num_serialize (FILE *f, val_t *v) {
  write_int(f, v->u.num);
}

val_t *v_num_deserialize (FILE *f) {
  val_t *v = val_new(T_NUM);
  v->u.num = read_int(f);
  return v;
}

void val_register_num (void) {
  val_ops[T_NUM] = (struct val_ops) {
    .create = v_num_create,
    .free   = NULL,
    .len    = NULL,
    .copy    = v_num_copy,
    .cmp    = v_num_cmp,
    .to_bool= v_num_to_bool,
    .index  = NULL,
    .index_assign = NULL,
    .to_string = v_num_to_string,
    .conv   = v_num_conv,
    .serialize = v_num_serialize,
    .deserialize = v_num_deserialize,
  };
}
