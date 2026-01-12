#include <assert.h>
#include <stdlib.h>

#include "stack.h"

vm_stack_t *stack_new (void) {
  vm_stack_t *stack = malloc (sizeof *stack);
  assert(stack != NULL);
  stack->vals = NULL;
  stack->size = 0;

  return stack;
}

void stack_push (vm_stack_t *stack, stackval_t val) {
  stack->vals = realloc(stack->vals, (stack->size + 1) * sizeof (stackval_t));
  assert(stack->vals != NULL);
  stack->vals[stack->size++] = val;
}

void stack_set (vm_stack_t *stack, int pos, stackval_t val) {
  assert(stack->size > pos);
  stack->vals[pos] = val;
}

stackval_t stack_pop (vm_stack_t *stack) {
  assert(stack->size > 0);
  return stack->vals[--stack->size];
}

stackval_t stack_peek (vm_stack_t *stack) {
  assert(stack->size > 0);
  return stack->vals[stack->size - 1];
}

int stack_size (vm_stack_t *stack) {
  return stack->size;
}

int stack_empty (vm_stack_t *stack) {
  return stack_size(stack) == 0;
}

void stack_free (vm_stack_t *stack) {
  free(stack->vals);
  free(stack);
}

