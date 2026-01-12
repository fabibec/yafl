#ifndef _STACK_H
#define _STACK_H

typedef int stackval_t;

struct stack {
  stackval_t *vals;
  int size;
};

/* Renamed to vm_stack_t due to collision */
typedef struct stack vm_stack_t;

vm_stack_t   *stack_new  (void);
void       stack_push (vm_stack_t *stack, stackval_t val);
void       stack_set  (vm_stack_t *stack, int pos, stackval_t val);
stackval_t stack_pop  (vm_stack_t *stack);
stackval_t stack_peek (vm_stack_t *stack);
void       stack_free (vm_stack_t *stack);
int        stack_size (vm_stack_t *stack);
int        stack_empty(vm_stack_t *stack);


#endif /* _STACK_H */
