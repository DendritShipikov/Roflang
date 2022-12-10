#include "roflang.h"

#define PUSH(C) q = C; if (FORWARD(q) == NULL) { FORWARD(q) = top; top = q; }
#define ADJUST(C) C = FORWARD(C)

void gc(struct vm *vm) {

  static cell_t dummy = { .data = { .integer = {0} }, .forward = NULL, .ind = 0, .tag = 0 };
  cell_t *top = &dummy, *bp = vm->bp, *hp = vm->hp, *sp = vm->sp, *ep = vm->ep, *ar = vm->ar, *gr = vm->gr;
  cell_t *p, *q;

  // mark
  for (p = sp; p < ep; ++p) {
    PUSH(AS_CONS(p).head);
  }
  PUSH(ar);
  PUSH(gr);
  while (FORWARD(top) != NULL) {
    p = top;
    top = FORWARD(p);
    FORWARD(p) = p; // required because p can point to non-heap object
    switch (TAG(p)) {
      case NIL_TAG:
      case INTEGER_TAG:
      case SYMBOL_TAG:
        break;
      case CONS_TAG:
        PUSH(AS_CONS(p).head);
        PUSH(AS_CONS(p).tail);
        break;
      case CLOSURE_TAG:
        PUSH(AS_CLOSURE(p).lambda);
        PUSH(AS_CLOSURE(p).env);
        break;
      case LITERAL_TAG:
        PUSH(AS_LITERAL(p).object);
        break;
      case IDENTIFIER_TAG:
        PUSH(AS_IDENTIFIER(p).name);
        break;
      case LAMBDA_TAG:
        PUSH(AS_LAMBDA(p).param);
        PUSH(AS_LAMBDA(p).body);
        break;
      case APPLICATION_TAG:
        PUSH(AS_APPLICATION(p).function);
        PUSH(AS_APPLICATION(p).argument);
        break;
      case BINOP_TAG:
        PUSH(AS_BINOP(p).left);
        PUSH(AS_BINOP(p).right);
        break;
      default:
        fprintf(stderr, "Error: wrong tag\n");
    }
  }

  // calculate new localtion
  q = bp;
  for (p = bp; p < hp; ++p) {
    if (FORWARD(p) != NULL) {
      FORWARD(p) = q++;
    }
  }

  // adjust pointers
  ADJUST(vm->ar);
  ADJUST(vm->gr);
  for (p = sp; p < ep; ++p) {
    ADJUST(AS_CONS(p).head);
  }
  for (p = bp; p < hp; ++p) {
    if (FORWARD(p) != NULL) {
      switch (TAG(p)) {
        case NIL_TAG:
        case INTEGER_TAG:
        case SYMBOL_TAG:
          break;
        case CONS_TAG:
          ADJUST(AS_CONS(p).head);
          ADJUST(AS_CONS(p).tail);
          break;
        case CLOSURE_TAG:
          ADJUST(AS_CLOSURE(p).lambda);
          ADJUST(AS_CLOSURE(p).env);
          break;
        case LITERAL_TAG:
          ADJUST(AS_LITERAL(p).object);
          break;
       case IDENTIFIER_TAG:
          ADJUST(AS_IDENTIFIER(p).name);
          break;
        case LAMBDA_TAG:
          ADJUST(AS_LAMBDA(p).param);
          ADJUST(AS_LAMBDA(p).body);
          break;
        case APPLICATION_TAG:
          ADJUST(AS_APPLICATION(p).function);
          ADJUST(AS_APPLICATION(p).argument);
          break;
        case BINOP_TAG:
          ADJUST(AS_BINOP(p).left);
          ADJUST(AS_BINOP(p).right);
          break;
        default:
          fprintf(stderr, "Error: wrong tag\n");
      }
    }
  }

  // move
  q = bp;
  for (p = bp; p < hp; ++p) {
    if (FORWARD(p) != NULL) {
      if (p != q) {
        *q = *p;
      }
      FORWARD(q) = NULL;
      ++q;
    }
  }
  vm->hp = q;
}
