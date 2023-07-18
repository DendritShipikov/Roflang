#include <stdio.h>
#include <stdlib.h>

#include "roflang.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    return 0;
  }
  static cell_t bp[1024];
  cell_t *ep = bp + 1024;
  struct parser p = {
    .cur = argv[1],
    .top = bp,
  };
  cell_t *env = parse_prog(&p);
  if (env == NULL) {
    return 0;
  }
  printf("parsed\n");
  struct context ctx = {
    .bp = bp,
    .hp = p.top,
    .sp = ep - 1,
    .fp = ep - 1,
    .ep = ep,
  };
  ctx.fp->frame.op = OP_EVAL;
  ctx.fp->frame.r1 = AS_PAIR(AS_PAIR(env).head).head;
  ctx.fp->frame.r2 = env;
  ctx.fp->frame.bfp = NULL;
  cell_t *obj = run(&ctx);
  switch (TAG(obj)) {
    case TAG_INTEGER:
      printf("%d\n", AS_INTEGER(obj).unboxed);
      break;
    case TAG_CLOSURE:
      printf("<closure>\n");
      break;
    default:
      printf("<obj>\n");
      break;
  }
  return 0;
}
