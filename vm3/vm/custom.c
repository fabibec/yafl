#include "prog.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>

/* Simple swap */
OPCODE(SWAP) {
  MINARGS(2);
  val_t *v_1 = POP;
  val_t *v_2 = POP;
  PUSH(v_1);
  PUSH(v_2);
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
    // Cap at 10 seconds
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

OPCODE(MKITER){
  val_t *v = POP;

  assert(v->type == T_ARR || v->type == T_STR);

  // (str and arr) iterator [iterable, idx, len]
  val_t *a = v_arr_create();
  arr_set(a->u.arr, 0, v);
  arr_set(a->u.arr, 1, v_num_new_int(0));
  int len = (v->type == T_STR) ? strlen(v->u.str->buf) : arr_len(v->u.arr);
  arr_set(a->u.arr, 2, v_num_new_int(len));
  PUSH(a);
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

/* Switch */
NATIVE(SWITCH_LOOKUP) {
  val_t *v_match = ARG(0);
  val_t *v_default_pc = ARG(1);
  val_t *v_map = ARG(2);

  assert(v_default_pc->type == T_NUM);

  assert(v_map->type == T_MAP);
  map_t *m = v_map->u.map;
  assert(v_match->type == m->keys[0]->type);

  val_t *maybe_pc = map_get(m, v_match);

  // Key not found -> return default location
  if(maybe_pc == &val_undef) {
    return v_default_pc;
  }

  assert(maybe_pc->type == T_NUM);
  return maybe_pc;
}

/* Input arguments */
NATIVE(ARGS) {
  // Push arr to stack
  return val_copy(exec->args);
}

/* File IO */
NATIVE(READ_FILE) {
  val_t *v_path = ARG(0);
  if (v_path->type != T_STR) return v_arr_create();

  // Very simple, read whole file at once, create str for every line
  FILE *f = fopen(v_path->u.str->buf, "r");
  val_t *lines = v_arr_create();
  if (!f) return lines;

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, f)) != -1) {
    if (read > 0 && line[read - 1] == '\n') line[read - 1] = '\0';
    arr_push(lines->u.arr, v_str_new_cstr(line));
  }
  free(line);
  fclose(f);
  return lines;
}

NATIVE(WRITE_FILE) {
  val_t *v_path = ARG(0);
  val_t *v_content = ARG(1);
  val_t *v_append = ARG(2);

  // Any error will be returned as 0
  if (v_path->type != T_STR || v_content->type != T_STR) return v_num_new_int(0);

  int append = 0;
  if (v_append->type != T_UNDEF) {
    append = val_to_bool(v_append);
  }

  FILE *f = fopen(v_path->u.str->buf, append ? "a" : "w");
  if (!f) return v_num_new_int(0);

  size_t len = strlen(v_content->u.str->buf);
  size_t written = fwrite(v_content->u.str->buf, 1, len, f);

  int ok = (written == len && fclose(f) == 0);
  return v_num_new_int(ok);
}

NATIVE(CONTAINS) {
  val_t *v_haystack = ARG(0);
  val_t *v_needle = ARG(1);
  assert(v_needle->type == T_STR && v_haystack->type == T_STR);
  char *haystack = v_haystack->u.str->buf;
  char *needle = v_needle->u.str->buf;

  size_t hlen = strlen(haystack);
  size_t nlen = strlen(needle);

  // If needle empty return 0
  if (nlen == 0) return 0;

  int ret = -1;
  for (size_t i = 0; i + nlen <= hlen; i++) {
    size_t j = 0;
    while (j < nlen && haystack[i + j] == needle[j]) j++;
    if (j == nlen) {
      ret = i;
      break;
    }
  }
  return v_num_new_int(ret);
}

NATIVE(SLICE) {
  val_t *v = ARG(0);
  val_t *v_start = ARG(1);
  val_t *v_end = ARG(2);

  assert(v->type == T_ARR || v->type == T_STR);
  assert(v_start->type == T_NUM && v_end->type == T_NUM);

  int start = v_start->u.num;
  int end = v_end->u.num;
  int len = 0;

  if (v->type == T_ARR) {
    len = arr_len(v->u.arr);
  } else {
    len = v->u.str->len;
  }

  // Allow negative indexing (start from the back)
  if (start < 0) start += len;
  if (end < 0) end += len;

  // Still out of bound? Clip
  if (start < 0) start = 0;
  if (end < 0) end = 0;
  if (end > len) end = len;

  if (start > end) start = end;

  int new_len = end - start;

  if (v->type == T_ARR) {
    val_t *res = v_arr_create();
    for (int i = 0; i < new_len; i++) {
        val_t *el = arr_get(v->u.arr, start + i);
        arr_push(res->u.arr, val_copy(el));
    }
    return res;
  } else {
    return v_str_new_buf(v->u.str->buf + start, new_len);
  }
}
