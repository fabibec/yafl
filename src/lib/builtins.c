#include "builtins.h"
#include "types.h"
#include "ast.h"
#include "codegen.h"
#include <stdlib.h>
#include <limits.h>

/* fn print(any: |printable|) -> none */
void builtins_print(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    yafl_t *arg_type = get_expr_type(node->data.call.args);
    bool is_range = arg_type && arg_type->base_t == TYPE_RANGE;
    type_free(arg_type);

    if(is_range) {
        /*ast_node *str = ast_new_str("range(");
        ast_node *print = ast_new_call("print", str);
        codegen_expr(print);
        ast_free(str);

        // Start
        print->data.call.args = node->data.call.args;
        codegen_expr(print);

        ast_node *str = ast_new_str(",");
        print->data.call.args = str;
        codegen_expr(print);
        ast_free(str);

        // Stop

        ast_node *str = ast_new_str(",");
        print->data.call.args = str;
        codegen_expr(print);
        ast_free(str);

        // Step

        ast_node *str = ast_new_str(")");
        print->data.call.args = str;
        codegen_expr(print);
        ast_free(str);

        prog_add_op(prog, DISCARD);*/
    } else {
        codegen_push_func_arguments(node, sym, arg_count);
        prog_add_num(prog, arg_count);
        val_t *func_name = v_str_new_cstr("print");
        int const_id = prog_new_constant(prog, func_name);
        prog_add_num(prog, const_id);
        prog_add_op(prog, CONSTANT);
        prog_add_op(prog, CALL);
        prog_add_op(prog, DISCARD);
    }
}

/* fn println(any: |printable|) -> none */
void builtins_println(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, arg_count);
    val_t *func_name = v_str_new_cstr("println");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
    prog_add_op(prog, DISCARD);
}

/* fn input_int(any: |printable str|) -> int */
void builtins_input_int(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    builtins_print(prog, node, sym, arg_count);

    prog_add_num(prog, 0);
    val_t *func_name = v_str_new_cstr("getint");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
}

/* fn input_str(any: |printable str|) -> str */
void builtins_input_str(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    builtins_print(prog, node, sym, arg_count);

    prog_add_num(prog, 0);
    val_t *func_name = v_str_new_cstr("getstring");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
}

/* fn sleep(int: |duration ms|) -> none */
void builtins_sleep(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_op(prog, SLEEPMS);
}

/* fn rng(int: |min|, int: |max|) -> int */
void builtins_rng(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    // random(max - min) + min
    ast_node * args = node->data.call.args;
    codegen_expr(args); // min
    prog_add_op(prog, DUP);
    codegen_expr(args->next); // max
    prog_add_op(prog, SUB);
    prog_add_num(prog, 1);
    val_t *func_name = v_str_new_cstr("random");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
    prog_add_op(prog, ADD);
}

/* fn rng(int: |max|) -> int */
void builtins_rng_max(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, 1);
    val_t *func_name = v_str_new_cstr("random");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
}

/* --- Casting --- */
void builtins_to_int(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, T_NUM);
    prog_add_op(prog, CAST);
}

void builtins_to_float(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, T_REAL);
    prog_add_op(prog, CAST);
}

void builtins_to_bool(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, T_NUM);
    prog_add_op(prog, CAST);
}

void builtins_to_str(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, T_STR);
    prog_add_op(prog, CAST);
}

void builtins_to_bool_from_str(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    // >>Yes<< -> Yes, everything else No
    codegen_push_func_arguments(node, sym, arg_count);
    val_t *str_yes = v_str_new_cstr("Yes");
    int const_yes = prog_new_constant(prog, str_yes);
    prog_add_num(prog, const_yes);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, EQUAL);
}

void builtins_to_bool_from_int(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    // Everything != 0 is Yes
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, 0);
    prog_add_op(prog, NOTEQUAL);
}

void builtins_to_bool_from_float(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    // Everything != 0.0 is Yes
    codegen_push_func_arguments(node, sym, arg_count);
    val_t *fl = v_real_new_double(0.0);
    int const_id = prog_new_constant(prog, fl);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, NOTEQUAL);
}

void builtins_to_int_from_str(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, T_NUM);
    prog_add_op(prog, CAST);

    // Check if result is undef
    prog_add_op(prog, DUP);
    prog_add_op(prog, TYPEOF);
    prog_add_num(prog, T_UNDEF);
    prog_add_op(prog, EQUAL);

    int jmp_valid = prog_add_num(prog, -1);
    prog_add_op(prog, JUMPF);

    // If undef, replace with 0
    prog_add_op(prog, DISCARD);
    prog_add_num(prog, 0);

    // Patch jump
    int valid_loc = prog_next_pc(prog);
    prog_set_num(prog, jmp_valid, valid_loc);
}

void builtins_to_float_from_str(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, T_REAL);
    prog_add_op(prog, CAST);

    // Check if result is undef
    prog_add_op(prog, DUP);
    prog_add_op(prog, TYPEOF);
    prog_add_num(prog, T_UNDEF);
    prog_add_op(prog, EQUAL);

    int jmp_ok = prog_add_num(prog, -1);
    prog_add_op(prog, JUMPF);

    // If undef, replace with 0.0
    prog_add_op(prog, DISCARD);
    val_t *fl = v_real_new_double(0.0);
    int const_id = prog_new_constant(prog, fl);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);

    // Patch jump
    int ok_trgt = prog_next_pc(prog);
    prog_set_num(prog, jmp_ok, ok_trgt);
}

void builtins_to_str_from_bool(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    // Yes -> >>Yes<<, No -> >>No<<
    codegen_push_func_arguments(node, sym, arg_count);

    int label_false = prog_add_num(prog, -1);
    prog_add_op(prog, JUMPF);

    val_t *str_yes = v_str_new_cstr("Yes");
    int const_yes = prog_new_constant(prog, str_yes);
    prog_add_num(prog, const_yes);
    prog_add_op(prog, CONSTANT);

    int jump_end = prog_add_num(prog, -1);
    prog_add_op(prog, JUMP);

    int false_target = prog_next_pc(prog);
    prog_set_num(prog, label_false, false_target);

    val_t *str_no = v_str_new_cstr("No");
    int const_no = prog_new_constant(prog, str_no);
    prog_add_num(prog, const_no);
    prog_add_op(prog, CONSTANT);

    int end_target = prog_next_pc(prog);
    prog_set_num(prog, jump_end, end_target);
}

/* fn range(int: |start|, int: |stop|, int: |step| <- 1) -> arr'int */
void builtins_range(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    ast_node *arg_start = node->data.call.args;
    ast_node *arg_stop = arg_start->next;

    (arg_count == 2) ? codegen_expr(sym->default_values[2]) : codegen_expr(arg_stop->next);
    codegen_expr(arg_stop);
    codegen_expr(arg_start);
    prog_add_op(prog, MKRANGE);
}

/* fn range(int: |stop|) -> arr'int */
void builtins_range_stop(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    prog_add_num(prog, 1); // step
    codegen_expr(node->data.call.args);
    prog_add_num(prog, 0); // start
    prog_add_op(prog, MKRANGE);
}

/* fn args() -> arr'str */
void builtins_args(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);

    prog_add_num(prog, 0);
    val_t *func_name = v_str_new_cstr("ARGS");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
}

/* fn read(str: |file path|) -> arr'str */
void builtins_read(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, 1);
    val_t *func_name = v_str_new_cstr("READ_FILE");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
}

/* fn write(str: |file path|, str: |content|, bool: |append| <- No) -> bool */
void builtins_write(prog_t *prog, ast_node *node, func_sym *sym, int arg_count) {
    codegen_push_func_arguments(node, sym, arg_count);
    prog_add_num(prog, 3);
    val_t *func_name = v_str_new_cstr("WRITE_FILE");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
}

void builtins_register(symtab *s) {
    // print<ln>(any) -> none
    yafl_t *none_t = type_new_simple(TYPE_VOID);
    yafl_t *any_t = type_new_simple(TYPE_GENERIC);
    yafl_t *print_args[] = {any_t};

    symtab_add_builtin(s, "print", none_t, 1, print_args, NULL, builtins_print);
    symtab_add_builtin(s, "println", none_t, 1, print_args, NULL, builtins_println);

    // args() -> arr'str
    yafl_t *arr_str_t = type_new_composite(type_new_simple(TYPE_STR));
    symtab_add_builtin(s, "args", arr_str_t, 0, NULL, NULL, builtins_args);

    // read(path) -> arr'str
    yafl_t *str_t = type_new_simple(TYPE_STR);
    yafl_t *read_args[] = {str_t};
    symtab_add_builtin(s, "read", arr_str_t, 1, read_args, NULL, builtins_read);

    // write (path, content, append) -> bool
    yafl_t *bool_t = type_new_simple(TYPE_BOOL);
    yafl_t *write_args[] = {str_t, str_t, bool_t};
    ast_node *append_default = ast_new_bool(0);
    ast_node *write_defaults[] = {NULL, NULL, append_default};
    symtab_add_builtin(s, "write", bool_t, 3, write_args, write_defaults, builtins_write);

    // input_<str|int>(any) -> <str|int>
    yafl_t *int_t = type_new_simple(TYPE_SINT);
    symtab_add_builtin(s, "input_int", int_t, 1, print_args, NULL, builtins_input_int);
    symtab_add_builtin(s, "input_str", str_t, 1, print_args, NULL, builtins_input_str);

    // sleep(ms) -> none
    yafl_t *sleep_args[] = {int_t};
    symtab_add_builtin(s, "sleep", none_t, 1, sleep_args, NULL, builtins_sleep);

    // rng(...) -> int
    yafl_t *rng_args[] = {int_t, int_t};
    symtab_add_builtin(s, "rng", int_t, 2, rng_args, NULL, builtins_rng);
    symtab_add_builtin(s, "rng", int_t, 1, sleep_args, NULL, builtins_rng_max);

    // range(...) -> range'int
    yafl_t *range_args[] = {int_t, int_t, int_t};
    yafl_t *range_args_stop[] = {int_t};
    yafl_t *range_t = type_new_simple(TYPE_RANGE);
    ast_node *step_default = ast_new_int(1);
    ast_node *range_defaults[] = {NULL, NULL, step_default};

    symtab_add_builtin(s, "range", range_t, 3, range_args, range_defaults, builtins_range);
    symtab_add_builtin(s, "range", range_t, 1, range_args_stop, NULL, builtins_range_stop);

    // Casting
    yafl_t *float_t = type_new_simple(TYPE_FLOAT);

    // to_int(...)
    yafl_t *arg_float[] = {float_t};
    yafl_t *arg_str[] = {str_t};
    yafl_t *arg_bool[] = {bool_t};
    symtab_add_builtin(s, "to_int", int_t, 1, arg_float, NULL, builtins_to_int);
    symtab_add_builtin(s, "to_int", int_t, 1, arg_bool, NULL, builtins_to_int);
    symtab_add_builtin(s, "to_int", int_t, 1, arg_str, NULL, builtins_to_int_from_str);

    // to_float(...)
    yafl_t *arg_int[] = {int_t};
    symtab_add_builtin(s, "to_float", float_t, 1, arg_float, NULL, builtins_to_float);
    symtab_add_builtin(s, "to_float", float_t, 1, arg_bool, NULL, builtins_to_float);
    symtab_add_builtin(s, "to_float", float_t, 1, arg_str, NULL, builtins_to_float_from_str);
    symtab_add_builtin(s, "to_float", float_t, 1, arg_int, NULL, builtins_to_float);

    // to_bool(...)
    symtab_add_builtin(s, "to_bool", bool_t, 1, arg_int, NULL, builtins_to_bool_from_int);
    symtab_add_builtin(s, "to_bool", bool_t, 1, arg_float, NULL, builtins_to_bool_from_float);
    symtab_add_builtin(s, "to_bool", bool_t, 1, arg_str, NULL, builtins_to_bool_from_str);

    // to_str(...)
    symtab_add_builtin(s, "to_str", str_t, 1, arg_int, NULL, builtins_to_str);
    symtab_add_builtin(s, "to_str", str_t, 1, arg_float, NULL, builtins_to_str);
    symtab_add_builtin(s, "to_str", str_t, 1, arg_bool, NULL, builtins_to_str_from_bool);

    type_free(int_t);
    type_free(float_t);
    type_free(bool_t);
    type_free(str_t);
    type_free(none_t);
    type_free(any_t);
    type_free(arr_str_t);
    type_free(range_t);
}

