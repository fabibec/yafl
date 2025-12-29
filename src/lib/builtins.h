#ifndef _BUILTINS_H_
#define _BUILTINS_H_
/* Codegen for builtin functions */
#include "prog.h"
#include "symtab.h"

/* fn print(any: |printable|) -> none */
void builtins_print(prog_t *prog, int arg_count);
/* fn println(any: |printable|) -> none */
void builtins_println(prog_t *prog, int arg_count);

/* fn clear() -> none */
void builtins_clear(prog_t * prog, int arg_count);

/* fn input'int() -> int */
/* fn input'str() -> str */

/* fn to'bool(any: |input|)*/
/* fn to'int(any: |input|)*/
/* fn to'float(any: |input|)*/
/* fn to'str(any: |input|)*/

/* fn push(arr<any>: |arr|, |input|) -> none */
/* fn pop(arr<any>: |arr|) -> any */

/* fn slice(arr<any>: |arr|, int |inclusive startIdx|, int |exclusive endIdx|) -> arr<any> */
/* fn slice(str: |arr|, str |inclusive startIdx|, str |exclusive endIdx|) -> str */

/* Casting */
void builtins_to_int(prog_t *prog, int arg_count);
void builtins_to_float(prog_t *prog, int arg_count);
void builtins_to_bool(prog_t *prog, int arg_count);
void builtins_to_str(prog_t *prog, int arg_count);

/* Specific overloads with special logic */
void builtins_to_bool_from_str(prog_t *prog, int arg_count);
void builtins_to_str_from_bool(prog_t *prog, int arg_count);
void builtins_to_bool_from_int(prog_t *prog, int arg_count);
void builtins_to_bool_from_float(prog_t *prog, int arg_count);
void builtins_to_int_from_str(prog_t *prog, int arg_count);
void builtins_to_float_from_str(prog_t *prog, int arg_count);

void builtins_register(symtab *s);

#endif // _BUILTINS_H_
