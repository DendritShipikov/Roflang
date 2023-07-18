#include <stdio.h>
#include <stdlib.h>

#include "roflang.h"

cell_t *parse_prog(struct parser *p) {
  static cell_t nil = { .object = { .tag = TAG_NIL } };
  cell_t *env = &nil;
  for (;;) {
    char c = *p->cur++;
    if (c == '\0') {
      break;
    }
    if (('a' > c || c > 'z') && ('A' > c || c > 'Z')) {
      fprintf(stderr, "Error: name expected\n");
      return NULL;
    }
    // todo memcheck 4
    cell_t *name = p->top++;
    cell_t *pair = p->top++;
    cell_t *obj = p->top++;
    cell_t *tmp = p->top++;
    make_symex(name, c);
    c = *p->cur++;
    if (c != '=') {
      fprintf(stderr, "Error: '=' expected\n");
      return NULL;
    }
    cell_t *expr = parse_expr(p);
    if (expr == NULL) {
      return NULL;
    }
    c = *p->cur++;
    if (c != ';') {
      fprintf(stderr, "Error: ';' expected\n");
      return NULL;
    }
    if (IS_LAMEX(expr)) {
      make_closure(obj, expr, NULL);
    } else {
      make_thunk(obj, expr, NULL);
    }
    make_pair(pair, name, obj);
    make_pair(tmp, pair, env);
    env = tmp;
  }
  for (cell_t *iter = env; IS_PAIR(iter); iter = AS_PAIR(iter).tail) {
    cell_t *obj = AS_PAIR(AS_PAIR(iter).head).tail;
    if (IS_CLOSURE(obj)) {
      AS_CLOSURE(obj).env = env;
    } else {
      AS_THUNK(obj).env = env;
    }
  }
  return env;
}

cell_t *parse_expr(struct parser *p) {
  char c = *p->cur;
  if (c != '@') {
    return parse_term(p);
  }
  c = *++p->cur;
  if (('a' > c || c > 'z') && ('A' > c || c > 'Z')) {
    fprintf(stderr, "Error: param expected\n");
    return NULL;
  }
  char name = c;
  c = *++p->cur;
  if (c != '.') {
    fprintf(stderr, "Error: '.' expected\n");
    return NULL;
  }
  ++p->cur;
  cell_t *body = parse_expr(p);
  if (body == NULL) {
    return NULL;
  }
  // todo memcheck 2
  cell_t *param = p->top++;
  cell_t *expr = p->top++;
  make_symex(param, name);
  make_lamex(expr, param, body);
  return expr;
}

cell_t *parse_term(struct parser *p) {
  cell_t *term = parse_item(p);
  if (term == NULL) {
    return NULL;
  }
  for (;;) {
    char c = *p->cur;
    if (('a' > c || c > 'z') && ('A' > c || c > 'Z') && c != '(' && ('0' > c || c > '9')) {
      return term;
    }
    cell_t *item = parse_item(p);
    if (item == NULL) {
      return NULL;
    }
    // todo memcheck 1
    cell_t *tmp = p->top++;
    make_appex(tmp, term, item);
    term = tmp;
  }
}

cell_t *parse_item(struct parser *p) {
  char c = *p->cur++;
  if (c == '(') {
    cell_t *item = parse_expr(p);
    if (item == NULL) {
      return NULL;
    }
    c = *p->cur++;
    if (c != ')') {
      fprintf(stderr, "Error: ')' expected\n");
      return NULL;
    }
    return item;
  }
  if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
    // todo memcheck 1
    cell_t *item = p->top++;
    make_symex(item, c);
    return item;
  }
  if ('0' <= c && c <= '9') {
    // todo memcheck 2
    cell_t *item = p->top++;
    cell_t *obj = p->top++;
    make_integer(obj, c - '0');
    make_litex(item, obj);
    return item;
  }
  fprintf(stderr, "Error: wrong item\n");
  return NULL;
}
