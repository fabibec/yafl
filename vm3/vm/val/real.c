#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "prog.h"
#include "val.h"
#include "str.h"
#include "utils.h"



val_t *v_real_new_double (double i) {
  val_t *v = val_new(T_REAL);
  v->u.real = i;

  return v;
}


val_t *v_real_create (void) {
  val_t *v = val_new(T_REAL);
  v->u.real = 0;

  return v;
}

val_t *v_real_copy (val_t *v) {
  val_t *v2 = val_new(T_REAL);
  v2->u.real = v->u.real;

  return v;
}

int v_real_cmp (val_t *v1, val_t *v2) {
  double diff = v1->u.real - v2->u.real;
  if (diff < 0)
    return -1;
  else if (diff > 0)
    return 1;
  else
    return 0;
}

int v_real_to_bool (val_t *v) {
  return v->u.real == 0.0;
}

val_t *v_real_to_string (val_t *real) {
  int l = snprintf(NULL, 0, "%f", real->u.real);
  char buf[l+1];
  snprintf(buf, sizeof(buf), "%f", real->u.real);
  return v_str_new_cstr(buf);
}

/* add the ability to parse lexer like floats */
static double parse_yafl_float(const char *s) {
    int base = 10;
    const char *p = s;

    const char *hash = strchr(s, '#');
    if (hash) {
        if (hash == s) {
            base = 16;
            p = hash + 1;
        } else {
            base = parse_base(s);
            if (!base) return NAN;
            p = hash + 1;
        }
    }

    return parse_based_float(base, p);
}

val_t *v_real_conv (val_t *v) {
 char *ptr;
 switch (v->type) {
   case T_STR:
     ptr = v->u.str->buf;
     /* Changed this part */
     double nr = parse_yafl_float(ptr);
     if(isnan(nr)){
       /* Interpret single char string as their ascii value */
       if (v->u.str->len == 1) {
         return v_real_new_double((double)(unsigned char)v->u.str->buf[0]);
       }
      return &val_undef;
     }
     return v_real_new_double(nr);
   /* Add support for num-> real */
   case T_NUM:
     return v_real_new_double((double)v->u.num);
   default:
    return &val_undef;
 }
}

void v_real_serialize (FILE *f, val_t *v) {
  fwrite(&v->u.real, sizeof(v->u.real), 1, f);
}

val_t *v_real_deserialize (FILE *f) {
  val_t *v = val_new(T_REAL);
  int st = fread(&v->u.real, sizeof(v->u.real), 1, f);
  assert(st == 1);
  return v;
}

void val_register_real (void) {
  val_ops[T_REAL] = (struct val_ops) {
    .create = v_real_create,
    .free   = NULL,
    .len    = NULL,
    .copy    = v_real_copy,
    .cmp    = v_real_cmp,
    .to_bool= v_real_to_bool,
    .index  = NULL,
    .index_assign = NULL,
    .to_string = v_real_to_string,
    .conv   = v_real_conv,
    .serialize = v_real_serialize,
    .deserialize = v_real_deserialize,
  };
}





