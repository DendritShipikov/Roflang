#include <stdlib.h>
#include <stdio.h>

#include "roflang.h"

#define REACHABLE(C) (MARKWORD(C) = 1)
#define MOVEABLE(C) (MARKWORD(C) = 2)
#define IS_REACHABLE(C) (MARKWORD(C) == 1)
#define IS_MOVEABLE(C) (MARKWORD(C) == 2)

static cell_t *mark(cell_t *p, cell_t *stack) {
  if (IS_INDIRECT(p)) {
    p = AS_INDIRECT(p).actual;
  }
  if (!IS_REACHABLE(p)) {
    REACHABLE(p);
    FORWARD(p) = stack;
    return p;
  }
  return stack;
}

static cell_t *take(cell_t *p) {
  if (IS_INDIRECT(p)) {
    p = AS_INDIRECT(p).actual;
  }
  if (IS_MOVEABLE(p)) {
    return FORWARD(p);
  }
  return p;
}

void gc(struct context *ctx) {
  cell_t *p, *q;
  cell_t *fp = ctx->fp;
  cell_t *sp = ctx->sp;
  cell_t *mp = ctx->mp;
  cell_t *hp = ctx->hp;
  cell_t *stack = NULL;

  /* roots */
  p = sp;
  q = fp;
  while (q != NULL) {
    for (; p < q; ++p) {
      stack = mark(p->ref.value, stack);
    }
    switch (q->frame.op) {
      case OP_EVAL:
      case OP_BINCONT:
        stack = mark(q->frame.r2, stack);
      case OP_BIN:
      case OP_APPLY:
      case OP_UPDATE:
      case OP_RETURN:
        stack = mark(q->frame.r1, stack);
      default:
        break;
    }
    p = q + 1;
    q = q->frame.fp;
  }
  stack = mark(ctx->gp, stack);

  /* mark */
  while (stack != NULL) {
    p = stack;
    stack = FORWARD(p);
    switch (TAG(p)) {
      case TAG_NIL:
      case TAG_HOLE:
      case TAG_INTEGER:
        break;
      case TAG_SYMBOL:
        stack = mark(AS_SYMBOL(p).tail, stack);
        break;
      case TAG_PAIR:
        stack = mark(AS_PAIR(p).head, stack);
        stack = mark(AS_PAIR(p).tail, stack);
        break;
      case TAG_CLOSURE:
        stack = mark(AS_CLOSURE(p).expr, stack);
        stack = mark(AS_CLOSURE(p).env, stack);
        break;
      case TAG_THUNK:
        stack = mark(AS_THUNK(p).expr, stack);
        stack = mark(AS_THUNK(p).env, stack);
        break;
      case TAG_APPEX:
        stack = mark(AS_APPEX(p).fun, stack);
        stack = mark(AS_APPEX(p).arg, stack);
        break;
      case TAG_BINEX:
        stack = mark(AS_BINEX(p).left, stack);
        stack = mark(AS_BINEX(p).right, stack);
        break;
      case TAG_LAMEX:
        stack = mark(AS_LAMEX(p).param, stack);
        stack = mark(AS_LAMEX(p).body, stack);
        break;
      case TAG_VAREX:
        stack = mark(AS_VAREX(p).name, stack);
        break;
      case TAG_LITEX:
        stack = mark(AS_LITEX(p).object, stack);
        break;
      default:
        fprintf(stderr, "Fatal error: wrong tag in gc stack\n");
        exit(1);
    }
  }

  /* calculate */
  for (q = p = mp; p < hp; ++p) {
    if (MARKWORD(p)) {
      FORWARD(p) = q++;
      MOVEABLE(p);
    }
  }
  ctx->hp = q;

  /* adjust */
  p = sp;
  q = fp;
  while (q != NULL) {
    for (; p < q; ++p) {
      p->ref.value = take(p->ref.value);
    }
    switch (q->frame.op) {
      case OP_EVAL:
      case OP_BINCONT:
        q->frame.r2 = take(q->frame.r2);
      case OP_BIN:
      case OP_APPLY:
      case OP_UPDATE:
      case OP_RETURN:
        q->frame.r1 = take(q->frame.r1);
      default:
        break;
    }
    p = q + 1;
    q = q->frame.fp;
  }
  ctx->gp = take(ctx->gp);
  for (p = mp; p < hp; ++p) {
    if (MARKWORD(p)) {
      switch (TAG(p)) {
        case TAG_NIL:
        case TAG_HOLE:
        case TAG_INTEGER:
          break;
        case TAG_SYMBOL:
          AS_SYMBOL(p).tail = take(AS_SYMBOL(p).tail);
          break;
        case TAG_PAIR:
          AS_PAIR(p).head = take(AS_PAIR(p).head);
          AS_PAIR(p).tail = take(AS_PAIR(p).tail);
          break;
        case TAG_CLOSURE:
          AS_CLOSURE(p).expr = take(AS_CLOSURE(p).expr);
          AS_CLOSURE(p).env = take(AS_CLOSURE(p).env);
          break;
        case TAG_THUNK:
          AS_THUNK(p).expr = take(AS_THUNK(p).expr);
          AS_THUNK(p).env = take(AS_THUNK(p).env);
          break;
        case TAG_APPEX:
          AS_APPEX(p).fun = take(AS_APPEX(p).fun);
          AS_APPEX(p).arg = take(AS_APPEX(p).arg);
          break;
        case TAG_BINEX:
          AS_BINEX(p).left = take(AS_BINEX(p).left);
          AS_BINEX(p).right = take(AS_BINEX(p).right);
          break;
        case TAG_LAMEX:
          AS_LAMEX(p).param = take(AS_LAMEX(p).param);
          AS_LAMEX(p).body = take(AS_LAMEX(p).body);
          break;
        case TAG_VAREX:
          AS_VAREX(p).name = take(AS_VAREX(p).name);
          break;
        case TAG_LITEX:
          AS_LITEX(p).object = take(AS_LITEX(p).object);
          break;
        default:
          fprintf(stderr, "Fatal error: wrong tag of gc marked object\n");
          exit(1);
      }
    }
  }

  /* move */
  for (p = mp; p < hp; ++p) {
    if (MARKWORD(p)) {
      MARKWORD(p) = 0;
      q = FORWARD(p);
      if (q != p) {
        *q = *p;
      }
    }
  }
}


