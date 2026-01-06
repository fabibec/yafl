#ifndef _ARITH_H_
#define _ARITH_H_

/* Enums for Unary and Binary operations */

typedef enum {
    OP_NEG, OP_NOT,
    OP_INC, OP_DEC
} un_op_t;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_LT, OP_GT, OP_LE, OP_GE, OP_EQ, OP_NE,
    OP_AND, OP_OR
} bin_op_t;

#endif // _ARITH_H_
