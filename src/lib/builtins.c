#include "builtins.h"
#include "types.h"
#include "ast.h"
#include <stdlib.h>
#include <limits.h>

/* fn print(any: |printable|) -> none */
void builtins_print(prog_t *prog, int arg_count) {
    prog_add_num(prog, arg_count);
    val_t *func_name = v_str_new_cstr("print");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
    prog_add_op(prog, DISCARD);
}

/* fn println(any: |printable|) -> none */
void builtins_println(prog_t *prog, int arg_count) {
    prog_add_num(prog, arg_count);
    val_t *func_name = v_str_new_cstr("println");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
    prog_add_op(prog, DISCARD);
}

/* fn input_int(any: |printable str|) -> int */
void builtins_input_int(prog_t *prog, int arg_count) {
    int redo_jmp_trgt = prog_next_pc(prog);
    builtins_print(prog, arg_count);

    prog_add_num(prog, 0);
    val_t *func_name = v_str_new_cstr("getint");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);

    prog_add_op(prog, DUP);
    val_t *none = val_create(T_UNDEF);
    int undef_const_id = prog_new_constant(prog, none);
    prog_add_num(prog, undef_const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, EQUAL);

    prog_add_num(prog, redo_jmp_trgt);
    prog_add_op(prog, JUMPT);
}

/* fn input_str(any: |printable str|) -> str */
void builtins_input_str(prog_t *prog, int arg_count) {
 int redo_jmp_trgt = prog_next_pc(prog);
    builtins_print(prog, arg_count);

    prog_add_num(prog, 0);
    val_t *func_name = v_str_new_cstr("getstring");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);

    prog_add_op(prog, DUP);
    val_t *none = val_create(T_UNDEF);
    int undef_const_id = prog_new_constant(prog, none);
    prog_add_num(prog, undef_const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, EQUAL);

    prog_add_num(prog, redo_jmp_trgt);
    prog_add_op(prog, JUMPT);
}

/* fn sleep(int: |duration ms|) -> none */
void builtins_sleep(prog_t *prog, int arg_count) {
    prog_add_op(prog, SLEEPMS);
}

/* fn randint(|int| : |max| <- MaxInt)*/
void builtins_randint(prog_t *prog, int arg_count) {
    prog_add_num(prog, arg_count);
    val_t *func_name = v_str_new_cstr("random");
    int const_id = prog_new_constant(prog, func_name);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, CALL);
}

/* --- Casting --- */
void builtins_to_int(prog_t *prog, int arg_count) {
    prog_add_num(prog, T_NUM);
    prog_add_op(prog, CAST);
}

void builtins_to_float(prog_t *prog, int arg_count) {
    prog_add_num(prog, T_REAL);
    prog_add_op(prog, CAST);
}

void builtins_to_bool(prog_t *prog, int arg_count) {
    prog_add_num(prog, T_NUM);
    prog_add_op(prog, CAST);
}

void builtins_to_str(prog_t *prog, int arg_count) {
    prog_add_num(prog, T_STR);
    prog_add_op(prog, CAST);
}

void builtins_to_bool_from_str(prog_t *prog, int arg_count) {
    // >>Yes<< -> Yes, everything else No
    val_t *str_yes = v_str_new_cstr("Yes");
    int const_yes = prog_new_constant(prog, str_yes);
    prog_add_num(prog, const_yes);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, EQUAL);
}

void builtins_to_bool_from_int(prog_t *prog, int arg_count) {
    // Everything != 0 is Yes
    prog_add_num(prog, 0);
    prog_add_op(prog, NOTEQUAL);
}

void builtins_to_bool_from_float(prog_t *prog, int arg_count) {
    // Everything != 0.0 is Yes
    val_t *fl = v_real_new_double(0.0);
    int const_id = prog_new_constant(prog, fl);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
    prog_add_op(prog, NOTEQUAL);
}

void builtins_to_int_from_str(prog_t *prog, int arg_count) {
    // Everything is just 0
    prog_add_op(prog, DISCARD); // Pop the string itself
    prog_add_num(prog, 0);
}

void builtins_to_float_from_str(prog_t *prog, int arg_count) {
    prog_add_op(prog, DISCARD); // Pop the string itself
    val_t *fl = v_real_new_double(0.0);
    int const_id = prog_new_constant(prog, fl);
    prog_add_num(prog, const_id);
    prog_add_op(prog, CONSTANT);
}

void builtins_to_str_from_bool(prog_t *prog, int arg_count) {
    // Yes -> >>Yes<<, No -> >>No<<
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

void builtins_register(symtab *s) {
    // print<ln>(any) -> none
    yafl_t *none_t = type_new_simple(TYPE_VOID);
    yafl_t *any_t = type_new_simple(TYPE_GENERIC);
    yafl_t *print_args[] = {any_t};

    symtab_add_builtin(s, "print", none_t, 1, print_args, NULL, builtins_print);
    symtab_add_builtin(s, "println", none_t, 1, print_args, NULL, builtins_println);

    // input_<str|int>(any) -> <str|int>
    yafl_t *int_t = type_new_simple(TYPE_SINT);
    yafl_t *str_t = type_new_simple(TYPE_STR);
    symtab_add_builtin(s, "input_int", int_t, 1, print_args, NULL, builtins_input_int);
    symtab_add_builtin(s, "input_str", str_t, 1, print_args, NULL, builtins_input_str);

    // sleep(ms) -> none
    yafl_t *sleep_args[] = {int_t};
    symtab_add_builtin(s, "sleep", none_t, 1, sleep_args, NULL, builtins_sleep);

    // randint(max) -> int
    yafl_t *randint_args[] = {int_t};
    ast_node *def_int_max = ast_new_int(INT_MAX);
    ast_node *randint_defaults[] = {def_int_max};
    symtab_add_builtin(s, "randint", int_t, 1, randint_args, randint_defaults, builtins_randint);

    // Casting
    yafl_t *float_t = type_new_simple(TYPE_FLOAT);
    yafl_t *bool_t = type_new_simple(TYPE_BOOL);

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
}
