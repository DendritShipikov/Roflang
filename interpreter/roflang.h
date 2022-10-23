#ifndef ROFLANG_H
#define ROFLANG_H

#include <stdio.h>
#include <stdlib.h>

enum {
  NIL_TAG,
  INTEGER_TAG,
  SYMBOL_TAG,
  CONS_TAG,
  CLOSURE_TAG,
  LITERAL_TAG,
  IDENTIFIER_TAG,
  LAMBDA_TAG,
  APPLICATION_TAG
};

typedef struct cell {
  union {
    struct {
      int unboxed;
    } integer;
    struct {
      char unboxed;
    } symbol;
    struct {
      struct cell *head;
      struct cell *tail;
    } cons;
    struct {
      struct cell *lambda;
      struct cell *locals;
    } closure;
    struct {
      struct cell *object;
    } literal;
    struct {
      char name;
    } identifier;
    struct {
      struct cell *body;
      char param;
    } lambda;
    struct {
      struct cell *function;
      struct cell *argument;
    } application;
  } data;
  struct cell *forward;
  int tag;
} cell_t;

/*
 *             HEAP SPACE          FREE SPACE      STACK SPACE
 *       -------------------------------------------------------
 *       ######################                  ###############   
 *       -------------------------------------------------------
 *       ^                     ^                 ^              ^
 *     vm.bp                 vm.hp             vm.sp          vm.ep
 */

struct vm {
  cell_t *bp;  // begin of memory
  cell_t *ep;  // end of memory
  cell_t *hp;  // heap pointer
  cell_t *sp;  // stack pointer
  cell_t *ar;  // accumulator register
};

/* size of free space = vm.sp - vm.hp */

extern struct vm *vm;

/* constructors without memory checks */
cell_t *new_integer(int unboxed);
cell_t *new_symbol(char unboxed);
cell_t *new_cons(cell_t *head, cell_t *tail);
cell_t *new_closure(cell_t *lambda, cell_t *locals);
cell_t *new_literal(cell_t *object);
cell_t *new_identifier(char name);
cell_t *new_lambda(char param, cell_t *body);
cell_t *new_application(cell_t *function, cell_t *argument);

#endif