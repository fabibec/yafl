#include "prog.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
// Add individual opcodes or native functions here

NATIVE(myadd) {
  val_t *v1 = ARG(0);
  val_t *v2 = ARG(1);

  return val_add(v1, v2);
}

OPCODE(mymul) {
  val_t *v1 = POP;
  val_t *v2 = POP;
  PUSH(val_mul(v1, v2));
}

/* CALL that uses a pc instead of a function name -> just a blatant copy paste of CALL */
OPCODE(CALL_PC) {
    MINARGS(2); // nrargs, func_pc

    val_t *func_pc_val = POP;
    val_t *nrargs_val = POP;

    assert(func_pc_val->type == T_NUM);
    assert(nrargs_val->type == T_NUM);

    int nr = nrargs_val->u.num;
    int func_pc = func_pc_val->u.num;

    MINARGS(nr);

    // Prepare call frame array: [return_pc, loop_stack, arg0, arg1, ...]
    val_t *frame = v_arr_create();
    arr_set(frame->u.arr, 0, v_num_new_int(exec->pc)); // return PC
    arr_set(frame->u.arr, 1, v_arr_create());          // loop stack
    for (int i = 0; i < nr; i++) {
        arr_set(frame->u.arr, i+2, POP);
    }

    vstack_push(exec->vars, frame);

    vmerror(E_DEBUG, exec, "Calling function at PC %d with %d args", func_pc, nr);
    exec->pc = func_pc - 1; // adjust for automatic increment
}

/* Custom opcode to sleep amount of ms */
OPCODE(SLEEPMS) {
    MINARGS(1);
    val_t *v_ms = POP;

    assert(v_ms->type == T_NUM);
    assert(v_ms->u.num >= 0);

    long ms = v_ms->u.num;
    if (ms > 10000) ms = 10000;

    struct timespec req, rem;
    req.tv_sec  = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000L;

    /* Resume sleep if interrupted */
    while (nanosleep(&req, &rem) == -1) {
        req = rem;
    }
}

/* Iterators */

int is_range(val_t *a) {
  // lazy range iterator [STR_TAG, cur, end, step]
  val_t *str_tag = arr_get(a->u.arr, 0);
  val_t *cur = arr_get(a->u.arr, 1);
  val_t *end = arr_get(a->u.arr, 2);
  val_t *step = arr_get(a->u.arr, 3);
  return (str_tag->type == T_STR && strcmp(str_tag->u.str->buf, "__RANGE__") == 0
    && cur->type == T_NUM && end->type == T_NUM && step->type == T_NUM);
}

OPCODE(MKRANGE) {
  MINARGS(3);
  val_t *v_start = POP;
  val_t *v_stop = POP;
  val_t *v_step = POP;

  assert(v_step->type == T_NUM && v_step->u.num != 0);
  assert(v_start->type == T_NUM);
  assert(v_stop->type == T_NUM);

  val_t *a = v_arr_create();
  arr_set(a->u.arr, 0, v_str_new_cstr("__RANGE__"));
  arr_set(a->u.arr, 1, v_start);
  arr_set(a->u.arr, 2, v_stop);
  arr_set(a->u.arr, 3, v_step);
  PUSH(a);
}

OPCODE(ITER_BEGIN){
  val_t *v = POP;

  assert(v->type == T_ARR || v->type == T_STR);

  if(v->type == T_ARR && is_range(v)) {
    PUSH(v);
  } else {
    // str + arr iterator [iterable, idx, len]
    val_t *a = v_arr_create();
    arr_set(a->u.arr, 0, v);
    arr_set(a->u.arr, 1, v_num_new_int(0));
    int len = (v->type == T_STR) ? strlen(v->u.str->buf) : arr_len(v->u.arr);
    arr_set(a->u.arr, 2, v_num_new_int(len));
    PUSH(a);
  }
}

OPCODE(ITER_NEXT){
  val_t *v = PEEK;
  assert(v->type == T_ARR);

  if(is_range(v)){
    int cur = arr_get(v->u.arr, 1)->u.num;
    int end = arr_get(v->u.arr, 2)->u.num;
    int step = arr_get(v->u.arr, 3)->u.num;

    // Iterator end -> pop iterator, push 0
    if((step > 0 && cur >= end) || (step < 0 && cur <= end)) {
      POP;
      PUSH(v_num_new_int(0));
      return;
    }

    // Iterate -> (1, next_val)
    PUSH(v_num_new_int(cur));
    arr_set(v->u.arr, 1, v_num_new_int(cur + step));
    PUSH(v_num_new_int(1));
  } else {
    assert(arr_len(v->u.arr) == 3);

    val_t *iterable = arr_get(v->u.arr, 0);
    int idx = arr_get(v->u.arr, 1)->u.num;
    int len = arr_get(v->u.arr, 2)->u.num;

    // Iterator end -> pop iterator, push 0
    if (idx >= len) {
      POP;
      PUSH(v_num_new_int(0));
      return;
    }

    // Iterate -> (1, next_val)
    val_t *el;
    if(iterable->type == T_STR) {
      el = v_str_new_buf(iterable->u.str->buf + idx, 1);
    } else {
      el = arr_get(iterable->u.arr, idx);
    }
    PUSH(el);
    arr_set(v->u.arr, 1, v_num_new_int(idx + 1));
    PUSH(v_num_new_int(1));
  }
}
