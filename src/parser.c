#include <stdio.h>
#include <stdlib.h>

#include "roflang.h"

static int is_space(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
static int is_alpha(int c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }
static int is_digit(int c) { return '0' <= c && c <= '9'; }

static char tokenize(struct parser *p) {
  while (is_space(*p->cur)) {
    ++p->cur;
  }
  return *p->cur;
}

static cell_t *parse_name(struct parser *p) {
  static cell_t nil = { .object = { .tag = TAG_NIL, .markword = 0 } };
  cell_t *name = &nil;
  cell_t *top = p->top;
  while (is_alpha(*p->cur)) {
    // todo memcheck 2
    cell_t *pair = p->top++;
    cell_t *sym = p->top++;
    make_symbol(sym, *p->cur++);
    make_pair(pair, sym, name);
    name = pair;
  }
  for (cell_t *iter = p->names; iter != NULL; iter = FORWARD(iter)) {
    cell_t *x = name, *y = iter;
    while (IS_PAIR(x) && IS_PAIR(y)) {
      if (AS_SYMBOL(AS_PAIR(y).head).unboxed != AS_SYMBOL(AS_PAIR(x).head).unboxed) {
        break;
      }
      x = AS_PAIR(x).tail;
      y = AS_PAIR(y).tail;
    }
    if (!IS_PAIR(x) && !IS_PAIR(y)) {
      p->top = top;
      return iter;
    }
  }
  FORWARD(name) = p->names;
  p->names = name;
  return name;
}

cell_t *parse_defs(struct parser *p) {
  static cell_t nil = { .object = { .tag = TAG_NIL, .markword = 0 } };
  cell_t *env = &nil;
  for (;;) {
    char c = tokenize(p);
    if (c == '\0') {
      break;
    }
    if (!is_alpha(c)) {
      fprintf(stderr, "Error: name expected\n");
      return NULL;
    }
    cell_t *name = parse_name(p);
    if (name == NULL) {
      return NULL;
    }
    c = tokenize(p);
    if (c != '=') {
      fprintf(stderr, "Error: '=' expected\n");
      return NULL;
    }
    ++p->cur;
    cell_t *expr = parse_expr(p);
    if (expr == NULL) {
      return NULL;
    }
    c = tokenize(p);
    if (c != ';') {
      fprintf(stderr, "Error: ';' expected\n");
      return NULL;
    }
    ++p->cur;
    // todo memcheck 3
    cell_t *pair = p->top++;
    cell_t *obj = p->top++;
    cell_t *tmp = p->top++;
    if (IS_LAMEX(expr)) {
      make_closure(obj, expr, &nil);
    } else {
      make_thunk(obj, expr, &nil);
    }
    make_pair(pair, name, obj);
    make_pair(tmp, pair, env);
    env = tmp;
  }
  return env;
}

cell_t *parse_expr(struct parser *p) {
  char c = tokenize(p);
  if (c != '@') {
    return parse_term(p);
  }
  c = *++p->cur;
  if (!is_alpha(c)) {
    fprintf(stderr, "Error: param expected\n");
    return NULL;
  }
  cell_t *param = parse_name(p);
  if (param == NULL) {
    return NULL;
  }
  c = tokenize(p);
  if (c != '.') {
    fprintf(stderr, "Error: '.' expected\n");
    return NULL;
  }
  ++p->cur;
  cell_t *body = parse_expr(p);
  if (body == NULL) {
    return NULL;
  }
  // todo memcheck 1
  cell_t *expr = p->top++;
  make_lamex(expr, param, body);
  return expr;
}

cell_t *parse_term(struct parser *p) {
  tokenize(p);
  cell_t *term = parse_item(p);
  if (term == NULL) {
    return NULL;
  }
  for (;;) {
    char c = tokenize(p);
    if (!is_alpha(c) && c != '(' && !is_digit(c)) {
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
  char c = *p->cur;
  if (c == '(') {
    ++p->cur;
    cell_t *item = parse_expr(p);
    if (item == NULL) {
      return NULL;
    }
    c = tokenize(p);
    if (c != ')') {
      fprintf(stderr, "Error: ')' expected\n");
      return NULL;
    }
    ++p->cur;
    return item;
  }
  if (is_alpha(c)) {
    // todo memcheck 1
    cell_t *item = p->top++;
    cell_t *name = parse_name(p);
    if (name == NULL) {
      return NULL;
    }
    make_varex(item, name);
    return item;
  }
  if (is_digit(c)) {
    // todo memcheck 2
    cell_t *item = p->top++;
    cell_t *obj = p->top++;
    make_integer(obj, c - '0');
    make_litex(item, obj);
    ++p->cur;
    return item;
  }
  fprintf(stderr, "Error: wrong item\n");
  return NULL;
}
