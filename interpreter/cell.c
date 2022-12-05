#include "roflang.h"

#define OPCODE(OP) { .data = { .integer = {OP} }, .forward = NULL, .tag = INTEGER_TAG, .ind = 0 }

cell_t *new_nil() {
  static cell_t nil = { .data = { .integer = {0} }, .forward = NULL, .tag = NIL_TAG, .ind = 0 };
  return &nil;
}

cell_t *new_opcode(int op) {
  static cell_t opcodes[] = {
    [OP_HALT] = OPCODE(OP_HALT),
    [OP_EVAL] = OPCODE(OP_EVAL),
    [OP_EVALFUNC] = OPCODE(OP_EVALFUNC),
    [OP_APPLY] = OPCODE(OP_APPLY),
    [OP_BINOP] = OPCODE(OP_BINOP),
    [OP_ADD] = OPCODE(OP_ADD),
    [OP_SUB] = OPCODE(OP_SUB),
    [OP_MUL] = OPCODE(OP_MUL),
    [OP_MODULE] = OPCODE(OP_MODULE),
    [OP_GLOBAL] = OPCODE(OP_GLOBAL)
  };
  return &opcodes[op];
}

cell_t *new_integer(struct vm *vm, int unboxed) {
  cell_t *cell = vm->hp++;
  FORWARD(cell) = NULL;
  TAG(cell) = INTEGER_TAG;
  AS_INTEGER(cell).unboxed = unboxed;
  return cell;
}

cell_t *new_symbol(struct vm *vm, char unboxed) {
  cell_t *cell = vm->hp++;
  FORWARD(cell) = NULL;
  TAG(cell) = SYMBOL_TAG;
  AS_SYMBOL(cell).unboxed = unboxed;
  return cell;
}

cell_t *new_cons(struct vm *vm, cell_t *head, cell_t *tail) {
  cell_t *cell = vm->hp++;
  FORWARD(cell) = NULL;
  TAG(cell) = CONS_TAG;
  AS_CONS(cell).head = head;
  AS_CONS(cell).tail = tail;
  return cell;
}

cell_t *new_closure(struct vm *vm, cell_t *lambda, cell_t *env) {
  cell_t *cell = vm->hp++;
  FORWARD(cell) = NULL;
  TAG(cell) = CLOSURE_TAG;
  AS_CLOSURE(cell).lambda = lambda;
  AS_CLOSURE(cell).env = env;
  return cell;
}

cell_t *new_literal(struct vm *vm, cell_t *object) {
  cell_t *cell = vm->hp++;
  FORWARD(cell) = NULL;
  TAG(cell) = LITERAL_TAG;
  AS_LITERAL(cell).object = object;
  return cell;
}

cell_t *new_identifier(struct vm *vm, cell_t *name) {
  cell_t *cell = vm->hp++;
  FORWARD(cell) = NULL;
  TAG(cell) = IDENTIFIER_TAG;
  AS_IDENTIFIER(cell).name = name;
  return cell;
}

cell_t *new_lambda(struct vm *vm, cell_t *param, cell_t *body) {
  cell_t *cell = vm->hp++;
  FORWARD(cell) = NULL;
  TAG(cell) = LAMBDA_TAG;
  AS_LAMBDA(cell).param = param;
  AS_LAMBDA(cell).body = body;
  return cell;
}

cell_t *new_application(struct vm *vm, cell_t *function, cell_t *argument) {
  cell_t *cell = vm->hp++;
  FORWARD(cell) = NULL;
  TAG(cell) = APPLICATION_TAG;
  AS_APPLICATION(cell).function = function;
  AS_APPLICATION(cell).argument = argument;
  return cell;
}

cell_t *new_binop(struct vm *vm, cell_t *left, cell_t *right, char kind) {
  cell_t *cell = vm->hp++;
  FORWARD(cell) = NULL;
  TAG(cell) = BINOP_TAG;
  IND(cell) = kind;
  AS_BINOP(cell).left = left;
  AS_BINOP(cell).right = right;
  return cell;
}
