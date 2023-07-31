#include "roflang.h"

static void make_object(cell_t *p, unsigned char tag) {
  TAG(p) = tag;
  MARKWORD(p) = 0;
}

void make_pair(cell_t *p, cell_t *head, cell_t *tail) {
  make_object(p, TAG_PAIR);
  AS_PAIR(p).head = head;
  AS_PAIR(p).tail = tail;
}

void make_symbol(cell_t *p, char head, cell_t *tail) {
  make_object(p, TAG_SYMBOL);
  AS_SYMBOL(p).head = head;
  AS_SYMBOL(p).tail = tail;
}

void make_closure(cell_t *p, cell_t *expr, cell_t *env) {
  make_object(p, TAG_CLOSURE);
  AS_CLOSURE(p).expr = expr;
  AS_CLOSURE(p).env = env;
}

void make_thunk(cell_t *p, cell_t *expr, cell_t *env) {
  make_object(p, TAG_THUNK);
  AS_THUNK(p).expr = expr;
  AS_THUNK(p).env = env;
}

void make_indirect(cell_t *p, cell_t *actual) {
  make_object(p, TAG_INDIRECT);
  AS_INDIRECT(p).actual = actual;
}

void make_integer(cell_t *p, int unboxed) {
  make_object(p, TAG_INTEGER);
  AS_INTEGER(p).unboxed = unboxed;
}

void make_binex(cell_t *p, unsigned char exttag, cell_t *left, cell_t *right) {
  make_object(p, TAG_BINEX);
  EXTTAG(p) = exttag;
  AS_BINEX(p).left = left;
  AS_BINEX(p).right = right;
}

void make_appex(cell_t *p, cell_t *fun, cell_t *arg) {
  make_object(p, TAG_APPEX);
  AS_APPEX(p).fun = fun;
  AS_APPEX(p).arg = arg;
}

void make_lamex(cell_t *p, cell_t *param, cell_t *body) {
  make_object(p, TAG_LAMEX);
  AS_LAMEX(p).param = param;
  AS_LAMEX(p).body = body;
}

void make_varex(cell_t *p, cell_t *name) {
  make_object(p, TAG_VAREX);
  AS_VAREX(p).name = name;
}

void make_litex(cell_t *p, cell_t *object) {
  make_object(p, TAG_LITEX);
  AS_LITEX(p).object = object;
}

void make_hole(cell_t *p) {
  make_object(p, TAG_HOLE);
}

void make_frame(cell_t *p, unsigned char op, unsigned char extop, cell_t *r1, cell_t *r2, cell_t *fp) {
  p->frame.op = op;
  p->frame.extop = extop;
  p->frame.r1 = r1;
  p->frame.r2 = r2;
  p->frame.fp = fp;
}
