#include <stdio.h>
#include <stdlib.h>

#include "roflang.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    return 0;
  }
  static cell_t mem[1024];
  cell_t *lim = mem + 1024;
  struct parser p = {
    .cur = argv[1],
    .top = mem,
    .names = NULL,
  };
  cell_t *env = parse(&p);
  if (env == NULL) {
    return 0;
  }
  printf("parsed\n");
  struct context ctx = {
    .mp = mem,
    .hp = p.top,
    .sp = lim - 1,
    .fp = lim - 1,
    .gp = env,
  };
  ctx.fp->frame.op = OP_APPLY;
  ctx.fp->frame.ar = 0;
  ctx.fp->frame.r1 = AS_PAIR(AS_PAIR(env).head).tail;
  ctx.fp->frame.r2 = NULL;
  ctx.fp->frame.bp = NULL;
  cell_t *obj = run(&ctx);
  switch (TAG(obj)) {
    case TAG_INTEGER:
      printf("%d\n", AS_INTEGER(obj).value);
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
