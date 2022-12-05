#include "roflang.h"

static cell_t *lookup(cell_t *identifier, cell_t *env) {
  cell_t *cons, *p, *q;
  for (; TAG(env) != NIL_TAG; env = AS_CONS(env).tail) {
    cons = AS_CONS(env).head;
    p = AS_IDENTIFIER(AS_CONS(cons).head).name;
    q = AS_IDENTIFIER(identifier).name;
    while (TAG(p) != NIL_TAG && TAG(q) != NIL_TAG && AS_SYMBOL(AS_CONS(p).head).unboxed == AS_SYMBOL(AS_CONS(q).head).unboxed) {
      p = AS_CONS(p).tail;
      q = AS_CONS(q).tail;
    }
    if (TAG(p) == NIL_TAG && TAG(q) == NIL_TAG) {
      return AS_CONS(cons).tail;
    }
  }
  return NULL;
}

#define VALUE(C) (AS_CONS(C).head)
#define MEMCHECK(N) if (vm->sp - vm->hp < N) { gc(vm); if (vm->sp - vm->hp < N) goto MEMORY_ERROR; }

cell_t *eval(struct vm *vm) {
  cell_t *object, *env, *cons, *right;
  for (;;) {
    switch (AS_INTEGER(VALUE(vm->sp + 0)).unboxed) {
      case OP_HALT:
        // object HALT:stack -> object
        return vm->ar;
      case OP_EVAL:
        switch (TAG(vm->ar)) {
          case LITERAL_TAG:
            // literal EVAL:env:stack -> literal.object stack
            vm->sp += 2;
            vm->ar = AS_LITERAL(vm->ar).object;
            continue;
          case IDENTIFIER_TAG:
            // ident EVAL:env:stack -> (lookup ident env) stack
            object = lookup(vm->ar, VALUE(vm->sp + 1));
            if (object == NULL) {
              object = lookup(vm->ar, vm->gr);
              if (object == NULL) {
                fprintf(stderr, "Error: unbounded identifier\n");
                return NULL;
              }
            }
            vm->sp += 2;
            vm->ar = object;
            continue;
          case LAMBDA_TAG:
            // lambda EVAL:env:stack -> (mk-closure lambda env) stack
            env = VALUE(vm->sp + 1);
            vm->sp += 2;
            vm->ar = new_closure(vm, vm->ar, env);
            continue;
          case APPLICATION_TAG:
            // application EVAL:env:stack -> application.argument EVAL:env:EVALFUNC:application.function:env:stack
            MEMCHECK(3);
            vm->sp -= 3;
            VALUE(vm->sp + 0) = new_opcode(OP_EVAL);
            VALUE(vm->sp + 1) = VALUE(vm->sp + 4);
            VALUE(vm->sp + 2) = new_opcode(OP_EVALFUNC);
            VALUE(vm->sp + 3) = AS_APPLICATION(vm->ar).function;
            vm->ar = AS_APPLICATION(vm->ar).argument;
            continue;
          case BINOP_TAG:
            // binop EVAL:env:stack -> binop.left EVAL:env:BINOP:binop.right:OP[binop.ind]:env:stack
            MEMCHECK(4);
            vm->sp -= 4;
            VALUE(vm->sp + 0) = new_opcode(OP_EVAL);
            VALUE(vm->sp + 1) = VALUE(vm->sp + 5);
            VALUE(vm->sp + 2) = new_opcode(OP_BINOP);
            VALUE(vm->sp + 3) = AS_BINOP(vm->ar).right;
            VALUE(vm->sp + 4) = new_opcode(IND(vm->ar)); // todo translate
            vm->ar = AS_BINOP(vm->ar).left;
            continue;
          default:
            fprintf(stderr, "Error: wrong expr\n");
            return NULL;
        }
      case OP_EVALFUNC:
        // object EVALFUNC:function:env:stack -> function EVAL:env:APPLY:object:stack
        MEMCHECK(1);
        vm->sp -= 1;
        VALUE(vm->sp + 0) = new_opcode(OP_EVAL);
        VALUE(vm->sp + 1) = VALUE(vm->sp + 3);
        VALUE(vm->sp + 3) = vm->ar;
        vm->ar = VALUE(vm->sp + 2);
        VALUE(vm->sp + 2) = new_opcode(OP_APPLY);
        continue;
      case OP_APPLY:
        // closure APPLY:object:stack -> closure.lambda.body EVAL:((closure.lambda.param:object):closure.env):stack
        MEMCHECK(2);
        if (TAG(vm->ar) != CLOSURE_TAG) {
          fprintf(stderr, "Error: function is not a closure\n");
          return NULL;
        }
        cons = new_cons(vm, AS_LAMBDA(AS_CLOSURE(vm->ar).lambda).param, VALUE(vm->sp + 1));
        env = new_cons(vm, cons, AS_CLOSURE(vm->ar).env);
        VALUE(vm->sp + 0) = new_opcode(OP_EVAL);
        VALUE(vm->sp + 1) = env;
        vm->ar = AS_LAMBDA(AS_CLOSURE(vm->ar).lambda).body;
        continue;
      case OP_BINOP:
        // left BINOP:binop.right:OP[binop.ind]:env:stack -> binop.right EVAL:env:OP[binop.ind]:left:stack
        VALUE(vm->sp + 0) = new_opcode(OP_EVAL);
        right = VALUE(vm->sp + 1);
        VALUE(vm->sp + 1) = VALUE(vm->sp + 3);
        VALUE(vm->sp + 3) = vm->ar;
        vm->ar = right;
        continue;
      case OP_ADD:
      case OP_SUB:
      case OP_MUL:
        // right ADD:left:stack -> (add left right) stack
        // right SUB:left:stack -> (sub left right) stack
        // right MUL:left:stack -> (mul left right) stack
        if (TAG(vm->ar) != INTEGER_TAG || TAG(VALUE(vm->sp + 1)) != INTEGER_TAG) {
          fprintf(stderr, "Error: operands of binop should be integers\n");
          return NULL;
        }
        vm->sp += 2;
        switch (AS_INTEGER(VALUE(vm->sp - 2)).unboxed) {
          case OP_ADD:
            vm->ar = new_integer(vm, AS_INTEGER(VALUE(vm->sp - 1)).unboxed + AS_INTEGER(vm->ar).unboxed);
            continue;
          case OP_SUB:
            vm->ar = new_integer(vm, AS_INTEGER(VALUE(vm->sp - 1)).unboxed - AS_INTEGER(vm->ar).unboxed);
            continue;
          case OP_MUL:
            vm->ar = new_integer(vm, AS_INTEGER(VALUE(vm->sp - 1)).unboxed * AS_INTEGER(vm->ar).unboxed);
            continue;
          default:
            fprintf(stderr, "Really?\n");
            return NULL;
        }
        continue;
      case OP_MODULE:
        // iter MODULE:stack -> iter.head.tail EVAL:nil:GLOBAL:iter.head.head:iter:tail:stack
        // nil MODULE:stack -> ...
        if (TAG(vm->ar) == NIL_TAG) {
          return vm->gr;
        }
        MEMCHECK(4);
        vm->sp -= 4;
        cons = AS_CONS(vm->ar).head;
        VALUE(vm->sp + 0) = new_opcode(OP_EVAL);
        VALUE(vm->sp + 1) = new_nil();
        VALUE(vm->sp + 2) = new_opcode(OP_GLOBAL);
        VALUE(vm->sp + 3) = AS_CONS(cons).head;
        VALUE(vm->sp + 4) = AS_CONS(vm->ar).tail;
        vm->ar = AS_CONS(cons).tail;
        continue;
      case OP_GLOBAL:
        // object GLOBAL:ident:next:stack -> next MODULE:stack
        vm->sp += 1;
        cons = new_cons(vm, VALUE(vm->sp + 0), vm->ar);
        vm->sp += 1;
        vm->gr = new_cons(vm, cons, vm->gr);
        vm->ar = VALUE(vm->sp + 0);
        VALUE(vm->sp + 0) = new_opcode(OP_MODULE);
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
