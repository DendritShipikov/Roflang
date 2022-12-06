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
  APPLICATION_TAG,
  BINOP_TAG
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
      struct cell *env;
    } closure;
    struct {
      struct cell *object;
    } literal;
    struct {
      struct cell *name;
    } identifier;
    struct {
      struct cell *param;
      struct cell *body;
    } lambda;
    struct {
      struct cell *function;
      struct cell *argument;
    } application;
    struct {
      struct cell *left;
      struct cell *right;
    } binop;
  } data;
  struct cell *forward;
  unsigned char tag;
  unsigned char ind;
} cell_t;

#define TAG(C) ((C)->tag)
#define IND(C) ((C)->ind)
#define FORWARD(C) ((C)->forward)
#define AS_INTEGER(C) ((C)->data.integer)
#define AS_SYMBOL(C) ((C)->data.symbol)
#define AS_CONS(C) ((C)->data.cons)
#define AS_CLOSURE(C) ((C)->data.closure)
#define AS_LITERAL(C) ((C)->data.literal)
#define AS_IDENTIFIER(C) ((C)->data.identifier)
#define AS_LAMBDA(C) ((C)->data.lambda)
#define AS_APPLICATION(C) ((C)->data.application)
#define AS_BINOP(C) ((C)->data.binop)

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
  cell_t *gr;  // globals register
};

/* size of free space = vm.sp - vm.hp */

enum {
  OP_HALT,
  OP_EVAL,
  OP_EVALFUNC,
  OP_APPLY,
  OP_BINOP,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_EQUAL,
  OP_LESS,
  OP_COMPARE,
  OP_GREATER,
  OP_MODULE,
  OP_GLOBAL
};

extern cell_t opcodes[];

cell_t *eval(struct vm *vm);
void gc(struct vm *vm);

/* constructors without memory checks */
cell_t *new_nil();
cell_t *new_opcode(int op);
cell_t *new_integer(struct vm *vm, int unboxed);
cell_t *new_symbol(struct vm *vm, char unboxed);
cell_t *new_cons(struct vm *vm, cell_t *head, cell_t *tail);
cell_t *new_closure(struct vm *vm, cell_t *lambda, cell_t *env);
cell_t *new_literal(struct vm *vm, cell_t *object);
cell_t *new_identifier(struct vm *vm, cell_t *name);
cell_t *new_lambda(struct vm *vm, cell_t *param, cell_t *body);
cell_t *new_application(struct vm *vm, cell_t *function, cell_t *argument);
cell_t *new_binop(struct vm *vm, cell_t *left, cell_t *right, char kind);

#endif