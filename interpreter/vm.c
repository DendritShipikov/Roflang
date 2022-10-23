#include "roflang.h"

struct vm *vm = NULL;

#define OPCODE(OP) { .data = { .integer = {OP} }, .forward = NULL, .tag = INTEGER_TAG }

cell_t opcodes[] = {
  [OP_HALT] = OPCODE(OP_HALT),
  [OP_EVAL] = OPCODE(OP_EVAL),
  [OP_APPLY] = OPCODE(OP_APPLY)
};

/* constructors */

cell_t *new_integer(int unboxed) {
  cell_t *cell = vm->hp++;
  cell->tag = INTEGER_TAG;
  cell->data.integer.unboxed = unboxed;
  return cell;
}

cell_t *new_symbol(char unboxed) {
  cell_t *cell = vm->hp++;
  cell->tag = SYMBOL_TAG;
  cell->data.symbol.unboxed = unboxed;
  return cell;
}

cell_t *new_cons(cell_t *head, cell_t *tail) {
  cell_t *cell = vm->hp++;
  cell->tag = CONS_TAG;
  cell->data.cons.head = head;
  cell->data.cons.tail = tail;
  return cell;
}

cell_t *new_closure(cell_t *lambda, cell_t *locals) {
  cell_t *cell = vm->hp++;
  cell->tag = CLOSURE_TAG;
  cell->data.closure.lambda = lambda;
  cell->data.closure.locals = locals;
  return cell;
}

cell_t *new_literal(cell_t *object) {
  cell_t *cell = vm->hp++;
  cell->tag = LITERAL_TAG;
  cell->data.literal.object = object;
  return cell;
}

cell_t *new_identifier(char name) {
  cell_t *cell = vm->hp++;
  cell->tag = IDENTIFIER_TAG;
  cell->data.identifier.name = name;
  return cell;
}

cell_t *new_lambda(char param, cell_t *body) {
  cell_t *cell = vm->hp++;
  cell->tag = LAMBDA_TAG;
  cell->data.lambda.param = param;
  cell->data.lambda.body = body;
  return cell;
}

cell_t *new_application(cell_t *function, cell_t *argument) {
  cell_t *cell = vm->hp++;
  cell->tag = APPLICATION_TAG;
  cell->data.application.function = function;
  cell->data.application.argument = argument;
  return cell;
}
