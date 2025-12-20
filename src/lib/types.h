#ifndef _TYPES_H_
#define _TYPES_H_

typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_FUNC,

    /* Integer types*/
    TYPE_SINT8,
    TYPE_SINT16,
    TYPE_SINT32,
    TYPE_SINT64,
    TYPE_UINT8,
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_UINT64
} yaflType;

#endif
