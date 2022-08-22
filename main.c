#include <stdio.h>
#include <stdlib.h>

#define BLACKHOLE(ANYTHING)

typedef unsigned char byte_t;

enum {
  OP_PUSH_LOCAL,
  OP_PUSH_CAPTURE,
  OP_PUSH_LITERAL,
  OP_POP,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MAKE_TUPLE,
  OP_MAKE_CLOSURE,
  OP_INVOKE,
  OP_RETURN
};

typedef struct object {
  size_t size;
  // struct object *reserved;
  // void (*iterate)(struct object *, void (*)(struct object **));
} object_t;

object_t *heap_new(size_t size) { return NULL; }
void heap_gc(object_t **root) { return; }

typedef struct integer {
  object_t base;
  int value;
} integer_t;

typedef struct tuple {
  object_t base;
  size_t nobjects;
  object_t *objects[1];
} tuple_t;

typedef struct code {
  object_t base;
  tuple_t *literals;
  size_t depth;
  size_t nunits;
  byte_t units[1];
} code_t;

typedef struct closure {
  object_t base;
  code_t *code;
  tuple_t *captures;
  size_t arity;
} closure_t;

/* HELPER */

byte_t *simple_alloc(size_t size) { return NULL; }

/* AST */

struct code_builder;
struct scope;

typedef struct ast_expr {
  int (*code_builder_compile)(struct code_builder *, struct ast_expr *, struct scope *, int);
  union {
    struct {
      const char *begin;
      const char *end;
      int kind;
    } literal;
    struct {
      const char *begin;
      const char *end;
    } name;
    struct {
      struct ast_expr *left;
      struct ast_expr *right;
      int kind;
    } binop;
    struct {
      struct ast_expr *function;
      struct ast_expr_iter *arguments;
    } apply;
    struct {
      struct ast_name_iter *params;
      struct ast_expr *body;
    } lambda;
    struct {
      struct ast_expr_iter *exprs;
    } seq;
  } v;
} ast_expr_t;

typedef struct ast_expr_iter {
  ast_expr_t *expr;
  struct ast_expr_iter *next;
} ast_expr_iter_t;

typedef struct ast_name_iter {
  struct {
    const char *begin;
    const char *end;
  } name;
  struct ast_name_iter *next;
} ast_name_iter_t;


/* SCOPE */

typedef struct entry {
  struct {
    const char *begin;
    const char *end;
  } name;
  struct entry *parent;
  struct entry *next;
  size_t index;
} entry_t;

typedef struct scope {
  struct scope *outer;
  entry_t *locals;
  entry_t *captures;
  size_t nlocals;
  size_t ncaptures;
} scope_t;

scope_t *create_empty_scope() { return NULL; }
scope_t *create_scope_from_params(ast_name_iter_t *params, scope_t *outer) { return NULL; }
entry_t *scope_lookup(scope_t *scope, const char *begin, const char *end) { return NULL; }

/* COMPILER */

typedef struct literal_iter {
  object_t *object;
  struct literal_iter *next;
} literal_iter_t;

typedef struct code_builder {
  literal_iter_t *literals;
  byte_t *units;
  size_t nliterals;
  size_t nunits;
  size_t capacity;
} code_builder_t;

code_builder_t *create_code_builder() { return NULL; }
code_t *code_builder_build(code_builder_t *builder) { return NULL; }
int code_builder_add_literal(code_builder_t *builder, object_t *object) { return -1; }
int code_builder_add_unit(code_builder_t *builder, byte_t unit) { return -1; }

code_t *compile(ast_expr_t *expr, scope_t *scope, int ret) {
  code_builder_t *builder = create_code_builder();
  if (builder == NULL) return NULL;
  if (expr->code_builder_compile(builder, expr, scope, ret) < 0) return NULL;
  code_t *code = code_builder_build(builder);
  return code;
}

#define CODE_BUILDER_ADD_UNIT(BUILDER, UNIT) \
  if (code_builder_add_unit(BUILDER, UNIT) < 0) return -1

#define CODE_BUILDER_COMPILE(BUILDER, EXPR, SCOPE, RET) \
  if (EXPR->code_builder_compile(BUILDER, EXPR, SCOPE, RET) < 0) return -1

int code_builder_compile_literal(code_builder_t *builder, ast_expr_t *expr, scope_t *scope, int ret) {
  return -1;
}

int code_builder_compile_name(code_builder_t *builder, ast_expr_t *expr, scope_t *scope, int ret) {
  entry_t *entry = scope_lookup(scope, expr->v.name.begin, expr->v.name.end);
  if (entry == NULL) return -1;
  if (entry->parent != NULL) {
    CODE_BUILDER_ADD_UNIT(builder, OP_PUSH_LOCAL);
  } else {
    CODE_BUILDER_ADD_UNIT(builder, OP_PUSH_CAPTURE);
  }
  CODE_BUILDER_ADD_UNIT(builder, entry->index);
  if (ret) {
    CODE_BUILDER_ADD_UNIT(builder, OP_RETURN);
  }
  return 0;
}

int code_builder_compile_binop(code_builder_t *builder, ast_expr_t *expr, scope_t *scope, int ret) {
  CODE_BUILDER_COMPILE(builder, expr->v.binop.left, scope, 0);
  CODE_BUILDER_COMPILE(builder, expr->v.binop.right, scope, 0);
  CODE_BUILDER_ADD_UNIT(builder, expr->v.binop.kind);
  if (ret) {
    CODE_BUILDER_ADD_UNIT(builder, OP_RETURN);
  }
  return 0;
}

int code_builder_compile_apply(code_builder_t *builder, ast_expr_t *expr, scope_t *scope, int ret) {
  for (ast_expr_iter_t *iter = expr->v.apply.arguments; iter != NULL; iter = iter->next) {
    CODE_BUILDER_COMPILE(builder, iter->expr, scope, 0);
  }
  CODE_BUILDER_COMPILE(builder, expr->v.apply.function, scope, 0);
  CODE_BUILDER_ADD_UNIT(builder, OP_INVOKE);
  if (ret) {
    CODE_BUILDER_ADD_UNIT(builder, OP_RETURN);
  }
  return 0;
}

int code_builder_compile_lambda(code_builder_t *builder, ast_expr_t *expr, scope_t *scope, int ret) {
  scope_t *inner = create_scope_from_params(expr->v.lambda.params, scope);
  if (inner == NULL) return -1;
  code_t *code = compile(expr->v.lambda.body, inner, 1);
  if (code == NULL) return -1;
  int index = code_builder_add_literal(builder, (object_t *) code);
  if (index < 0) return -1;
  CODE_BUILDER_ADD_UNIT(builder, OP_PUSH_LITERAL);
  CODE_BUILDER_ADD_UNIT(builder, index);
  for (entry_t *entry = inner->captures; entry != NULL; entry = entry->next) {
    entry_t *parent = entry->parent;
    if (parent->parent != NULL) {
      CODE_BUILDER_ADD_UNIT(builder, OP_PUSH_CAPTURE);
    } else {
      CODE_BUILDER_ADD_UNIT(builder, OP_PUSH_LOCAL);
    }
    CODE_BUILDER_ADD_UNIT(builder, parent->index);
  }
  CODE_BUILDER_ADD_UNIT(builder, OP_MAKE_CLOSURE);
  return 0;
}

int compile_seq(code_builder_t *builder, ast_expr_t *expr, scope_t *scope, int ret) {
  return -1;
}

int main() {
BLACKHOLE(
  string_t *source = read();
  ast_expr_t *expr = parse(source);
  scope_t *scope = create_empty_scope();
  code_t *code = compile(expr, scope, 0);
  heap_gc(&code);
)
  // interpreting
  return 0;
}

