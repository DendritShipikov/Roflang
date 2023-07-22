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
    .names = NULL,
  };
  cell_t *env = parse_defs(&p);
  if (env == NULL) {
    return 0;
  }
  printf("parsed\n");
  struct context ctx = {
    .mp = bp,
    .hp = p.top,
    .sp = ep - 1,
    .bp = ep - 1,
    .gp = env,
  };
  ctx.bp->frame.op = OP_APPLY;
  ctx.bp->frame.r1 = AS_PAIR(AS_PAIR(env).head).tail;
  ctx.bp->frame.r2 = NULL;
  ctx.bp->frame.bp = NULL;
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
