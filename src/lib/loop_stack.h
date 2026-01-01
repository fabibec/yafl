#ifndef _LOOP_STACK_H_
#define _LOOP_STACK_H_
/* Loop stack for break/continue */
#include <stdlib.h>
#include "prog.h"
#include "symtab.h"

extern prog_t *prog;
extern symtab *prog_symtab;

typedef struct jmp_lst {
    int pc_loc;
    struct jmp_lst *next;
} jmp_lst;

typedef struct loop_ctx {
    int continue_pc;
    bool has_iterator;
    jmp_lst *break_jumps;
    struct loop_ctx *prev;
} loop_ctx;

extern loop_ctx *loop_stack;

void loop_push(int continue_pc, bool has_iterator);
void loop_add_break_jump(int pc_loc);
void loop_pop(int break_target_pc);

#endif // _LOOP_STACK_H_
