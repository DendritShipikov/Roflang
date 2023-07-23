#include <stdio.h>
#include <stdlib.h>

#include "roflang.h"

static void ensure_space(struct context *ctx, int count);

#define NEW()   (ctx->hp++)
#define R1      (ctx->fp->frame.r1)
#define R2      (ctx->fp->frame.r2)
#define OP      (ctx->fp->frame.op)
#define FP      (ctx->fp->frame.fp)
#define TOS()   ((ctx->sp)->ref.value)
#define POP()   ((ctx->sp++)->ref.value)
#define PUSH(C) ((--ctx->sp)->ref.value = (C))

cell_t *run(struct context *ctx) {
  cell_t *expr, *obj, *env, *pair, *name;
  for (;;) {
    /* maybe i should save ctx->fp in local variable here (i can do this since ctx->fp is not moved by gc) */
    switch (OP) {
      case OP_EVAL:
        ensure_space(ctx, 2);
        expr = R1;
        switch (TAG(expr)) {
          case TAG_LAMEX:
            if (ctx->sp == ctx->fp) {
              /* eval(@x.E, env, []) = cls{@x.E, env} */
              R1 = NEW();
              make_closure(R1, expr, R2);
              OP = OP_RETURN;
              continue;
            }
            /* eval(@x.E, env, obj:stack) = eval(E, env[x=obj], stack) */
            obj = POP();
            pair = NEW();
            env = NEW();
            make_pair(pair, AS_LAMEX(expr).param, obj);
            make_pair(env, pair, R2);
            R2 = env;
            R1 = AS_LAMEX(expr).body;
            continue;
          case TAG_APPEX:
            expr = AS_APPEX(expr).arg;
            break;
          default:
            break;
        }
        switch (TAG(expr)) {
          case TAG_APPEX:
            obj = NEW();
            make_thunk(obj, expr, R2);
            break;
          case TAG_LAMEX:
            obj = NEW();
            make_closure(obj, expr, R2);
            break;
          case TAG_VAREX:
            name = AS_VAREX(expr).name;
            for (env = R2; IS_PAIR(env); env = AS_PAIR(env).tail) {
              pair = AS_PAIR(env).head;
              if (AS_PAIR(pair).head == name) {
                obj = AS_PAIR(pair).tail;
                break;
              }
            }
            if (IS_PAIR(env)) {
              break;
            }
            for (env = ctx->gp; IS_PAIR(env); env = AS_PAIR(env).tail) {
              pair = AS_PAIR(env).head;
              if (AS_PAIR(pair).head == name) {
                obj = AS_PAIR(pair).tail;
                break;
              }
            }
            if (IS_PAIR(env)) {
              break;
            }
            fprintf(stderr, "Fatal error: unbounded name\n");
            exit(1);
          case TAG_LITEX:
            obj = AS_LITEX(expr).object;
            break;
          default:
            fprintf(stderr, "Fatal error: wrong tag for eval\n");
            exit(1);
        }
        expr = R1;
        if (IS_APPEX(expr)) {
          /* eval(A(#obj), env, stack) = apply(A, env obj:stack) */
          /* eval(Ax, env, stack) = eval(A, env, env[x]:stack) */
          /* eval(A(@x.B), env, stack) = eval(A, env, cls{@x.B, env}:stack) */
          /* eval(A(BC), env, stack) = eval(A, env, thn{BC, env}:stack) */
          PUSH(obj);
          R1 = AS_APPEX(expr).fun;
          continue;
        }
        /* eval(#obj, env, stack) = apply(obj, stack) */
        /* eval(x, env, stack) = apply(env[x], stack) */
        R1 = obj;
        OP = OP_APPLY;
        continue;
      case OP_APPLY:
        switch (TAG(R1)) {
          case TAG_HOLE:
            fprintf(stderr, "Fatal error: cyclic dependency\n");
            exit(1);
          case TAG_NIL:
          case TAG_PAIR:
          case TAG_INTEGER:
            /* apply(obj, stack) = obj */
            OP = OP_RETURN;
            continue;
          case TAG_INDIRECT:
            /* apply(ind{obj}, stack) = apply(obj, stack) */
            R1 = AS_INDIRECT(R1).actual;
            continue;
          case TAG_CLOSURE:
            if (ctx->sp == ctx->fp) {
              /* apply(cls{A, env}, []) = cls{A, env} */
              OP = OP_RETURN;
              continue;
            }
            /* apply(cls{A, env}, stack) = eval(A, env, stack) */
            R2 = AS_CLOSURE(R1).env;
            R1 = AS_CLOSURE(R1).expr;
            OP = OP_EVAL;
            continue;
          case TAG_THUNK:
            /* apply(thn{A, env}, stack) = update(thn{A, env}, eval(A, env, stack)) */
            ensure_space(ctx, 1);
            --ctx->sp;
            make_frame(ctx->sp, OP_EVAL, AS_THUNK(R1).expr, AS_THUNK(R1).env, ctx->fp);
            make_hole(R1);
            OP = OP_UPDATE;
            ctx->fp = ctx->sp;
            continue;
          default:
            fprintf(stderr, "Fatal error: wrong tag for apply\n");
            exit(1);
        }
      case OP_RETURN:
        obj = R1;
        ctx->sp = ctx->fp;
        ctx->fp = FP;
        if (ctx->fp == NULL) {
          return obj;
        }
        TOS() = obj;
        continue;
      case OP_UPDATE:
        obj = POP();
        make_indirect(R1, obj);
        R1 = obj;
        OP = OP_APPLY;
        continue;
      default:
        fprintf(stderr, "Fatal error: unknown op\n");
        exit(1);
    }
  }
}

static void ensure_space(struct context *ctx, int count) {
  if (ctx->sp - ctx->hp < count) {
    fprintf(stderr, "Runtime error: memory is out\n");
    exit(1);
  }
}
