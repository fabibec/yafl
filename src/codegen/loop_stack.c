#include "loop_stack.h"
#include "codegen.h"
#include "logger.h"
#include <stdbool.h>

loop_ctx *loop_stack = NULL;

void loop_push(int continue_pc, bool has_iterator) {
    loop_ctx *ctx = malloc(sizeof(loop_ctx));
    ctx->continue_pc = continue_pc;
    ctx->has_iterator = has_iterator;
    ctx->break_jumps = NULL;
    ctx->prev = loop_stack;
    loop_stack = ctx;
}

void loop_add_break_jump(int pc_loc) {
    if (!loop_stack) {
        log_error(0, "Break statement outside of loop");
    }
    jmp_lst *j = malloc(sizeof(jmp_lst));
    j->pc_loc = pc_loc;
    j->next = loop_stack->break_jumps;
    loop_stack->break_jumps = j;
}

void loop_pop(int break_target_pc) {
    if (!loop_stack) return;

    loop_ctx *ctx = loop_stack;
    loop_stack = ctx->prev;

    // Patch all break jumps
    jmp_lst *j = ctx->break_jumps;
    while (j) {
        prog_set_num(prog, j->pc_loc, break_target_pc);
        jmp_lst *next = j->next;
        free(j);
        j = next;
    }
    free(ctx);
}
