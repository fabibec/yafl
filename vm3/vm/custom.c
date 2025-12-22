#include "prog.h"

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

/* Since I'm using my own symtab i want to CALL with a pc -> just copy past of CALL opcode with minor changes */
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
