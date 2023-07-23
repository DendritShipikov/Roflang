#include "roflang.h"

void make_pair(cell_t *p, cell_t *head, cell_t *tail) {
  TAG(p) = TAG_PAIR;
  AS_PAIR(p).head = head;
  AS_PAIR(p).tail = tail;
}

void make_closure(cell_t *p, cell_t *expr, cell_t *env) {
  TAG(p) = TAG_CLOSURE;
  AS_CLOSURE(p).expr = expr;
  AS_CLOSURE(p).env = env;
}

void make_thunk(cell_t *p, cell_t *expr, cell_t *env) {
  TAG(p) = TAG_THUNK;
  AS_THUNK(p).expr = expr;
  AS_THUNK(p).env = env;
}

void make_indirect(cell_t *p, cell_t *actual) {
  TAG(p) = TAG_INDIRECT;
  AS_INDIRECT(p).actual = actual;
}

void make_integer(cell_t *p, int unboxed) {
  TAG(p) = TAG_INTEGER;
  AS_INTEGER(p).unboxed = unboxed;
}

void make_symbol(cell_t *p, char unboxed) {
  TAG(p) = TAG_SYMBOL;
  AS_SYMBOL(p).unboxed = unboxed;
}

void make_appex(cell_t *p, cell_t *fun, cell_t *arg) {
  TAG(p) = TAG_APPEX;
  AS_APPEX(p).fun = fun;
  AS_APPEX(p).arg = arg;
}

void make_lamex(cell_t *p, cell_t *param, cell_t *body) {
  TAG(p) = TAG_LAMEX;
  AS_LAMEX(p).param = param;
  AS_LAMEX(p).body = body;
}

void make_varex(cell_t *p, cell_t *name) {
  TAG(p) = TAG_VAREX;
  AS_VAREX(p).name = name;
}

void make_litex(cell_t *p, cell_t *object) {
  TAG(p) = TAG_LITEX;
  AS_LITEX(p).object = object;
}

void make_hole(cell_t *p) {
  TAG(p) = TAG_HOLE;
}

void make_frame(cell_t *p, unsigned int op, cell_t *r1, cell_t *r2, cell_t *fp) {
  p->frame.op = op;
  p->frame.r1 = r1;
  p->frame.r2 = r2;
  p->frame.fp = fp;
}
