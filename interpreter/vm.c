#include "roflang.h"

static cell_t cells[1024];

struct vm vm = {
  .bp = cells,
  .ep = cells + 1024,
  .hp = cells,
  .sp = cells + 1024,
  .ar = NULL
};

#define OPCODE(OP) { .data = { .integer = {OP} }, .forward = NULL, .tag = INTEGER_TAG }

cell_t opcodes[] = {
  [OP_HALT] = OPCODE(OP_HALT),
  [OP_EVAL] = OPCODE(OP_EVAL),
  [OP_EVALFUNC] = OPCODE(OP_EVALFUNC),
  [OP_APPLY] = OPCODE(OP_APPLY)
};

#define VALUE(C) (AS_CONS(C).head)
#define MEMCHECK(N) if (vm.sp - vm.hp < N) goto MEMORY_ERROR;

cell_t *run() {
  cell_t *object, *env, *cons;
  for (;;) {
    switch (AS_INTEGER(VALUE(vm.sp + 0)).unboxed) {
      case OP_HALT:
        // object HALT:stack -> object
        return vm.ar;
      case OP_EVAL:
        switch (TAG(vm.ar)) {
          case LITERAL_TAG:
            // literal EVAL:env:stack -> literal.object stack
            vm.sp += 2;
            vm.ar = AS_LITERAL(vm.ar).object;
            continue;
          case IDENTIFIER_TAG:
            // ident EVAL:env:stack -> (lookup ident env) stack
            object = NULL;
            for (env = VALUE(vm.sp + 1); TAG(env) != NIL_TAG; env = AS_CONS(env).tail) {
              cons = AS_CONS(env).head;
              if (AS_IDENTIFIER(AS_CONS(cons).head).name == AS_IDENTIFIER(vm.ar).name) {
                object = AS_CONS(cons).tail;
                break;
              }
            }
            if (object == NULL) {
              fprintf(stderr, "Error: unbounded identifier\n");
              return NULL;
            }
            vm.sp += 2;
            vm.ar = object;
            continue;
          case LAMBDA_TAG:
            // lambda EVAL:env:stack -> (mk-closure lambda env) stack
            env = VALUE(vm.sp + 1);
            vm.sp += 2;
            vm.ar = new_closure(vm.ar, env);
            continue;
          case APPLICATION_TAG:
            // application EVAL:env:stack -> application.argument EVAL:env:EVALFUNC:application.function:env:stack
            MEMCHECK(3);
            vm.sp -= 3;
            VALUE(vm.sp + 0) = &opcodes[OP_EVAL];
            VALUE(vm.sp + 1) = VALUE(vm.sp + 4);
            VALUE(vm.sp + 2) = &opcodes[OP_EVALFUNC];
            VALUE(vm.sp + 3) = AS_APPLICATION(vm.ar).function;
            vm.ar = AS_APPLICATION(vm.ar).argument;
            continue;
          default:
            fprintf(stderr, "Error: wrong expr\n");
            return NULL;
        }
      case OP_EVALFUNC:
        // object EVALFUNC:function:env:stack -> function EVAL:env:APPLY:object:stack
        MEMCHECK(1);
        vm.sp -= 1;
        VALUE(vm.sp + 0) = &opcodes[OP_EVAL];
        VALUE(vm.sp + 1) = VALUE(vm.sp + 3);
        VALUE(vm.sp + 3) = vm.ar;
        vm.ar = VALUE(vm.sp + 2);
        VALUE(vm.sp + 2) = &opcodes[OP_APPLY];
        continue;
      case OP_APPLY:
        // closure APPLY:object:stack -> closure.lambda.body EVAL:((closure.lambda.param:object):closure.env):stack
        MEMCHECK(2);
        if (TAG(vm.ar) != CLOSURE_TAG) {
          fprintf(stderr, "Error: function is not a closure\n");
          return NULL;
        }
        cons = new_cons(AS_LAMBDA(AS_CLOSURE(vm.ar).lambda).param, VALUE(vm.sp + 1));
        env = new_cons(cons, AS_CLOSURE(vm.ar).env);
        VALUE(vm.sp + 0) = &opcodes[OP_EVAL];
        VALUE(vm.sp + 1) = env;
        vm.ar = AS_LAMBDA(AS_CLOSURE(vm.ar).lambda).body;
        continue;
      default:
        fprintf(stderr, "Error: wrong op\n");
        return NULL;
    }
  }
MEMORY_ERROR:
  fprintf(stderr, "Error: memory is out\n");
  return NULL;
}

/* constructors */

cell_t *new_integer(int unboxed) {
  cell_t *cell = vm.hp++;
  cell->tag = INTEGER_TAG;
  cell->data.integer.unboxed = unboxed;
  return cell;
}

cell_t *new_symbol(char unboxed) {
  cell_t *cell = vm.hp++;
  cell->tag = SYMBOL_TAG;
  cell->data.symbol.unboxed = unboxed;
  return cell;
}

cell_t *new_cons(cell_t *head, cell_t *tail) {
  cell_t *cell = vm.hp++;
  cell->tag = CONS_TAG;
  cell->data.cons.head = head;
  cell->data.cons.tail = tail;
  return cell;
}

cell_t *new_closure(cell_t *lambda, cell_t *env) {
  cell_t *cell = vm.hp++;
  cell->tag = CLOSURE_TAG;
  cell->data.closure.lambda = lambda;
  cell->data.closure.env = env;
  return cell;
}

cell_t *new_literal(cell_t *object) {
  cell_t *cell = vm.hp++;
  cell->tag = LITERAL_TAG;
  cell->data.literal.object = object;
  return cell;
}

cell_t *new_identifier(char name) {
  cell_t *cell = vm.hp++;
  cell->tag = IDENTIFIER_TAG;
  cell->data.identifier.name = name;
  return cell;
}

cell_t *new_lambda(cell_t *param, cell_t *body) {
  cell_t *cell = vm.hp++;
  cell->tag = LAMBDA_TAG;
  cell->data.lambda.param = param;
  cell->data.lambda.body = body;
  return cell;
}

cell_t *new_application(cell_t *function, cell_t *argument) {
  cell_t *cell = vm.hp++;
  cell->tag = APPLICATION_TAG;
  cell->data.application.function = function;
  cell->data.application.argument = argument;
  return cell;
}
