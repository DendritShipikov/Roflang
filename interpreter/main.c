#include "roflang.h"
#include <limits.h>

struct lexer {
  FILE *file;
  cell_t *value;
  int kind;
};

int is_space(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
int is_alpha(int c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_'; }
int is_digit(int c) { return '0' <= c && c <= '9'; }

int tokenize(struct vm *vm, struct lexer *lex) {
  int c;
  while (is_space(c = fgetc(lex->file))) ;
  switch (c) {
    case EOF:
      return lex->kind = 0;
    case '(':
    case ')':
    case '*':
    case '+':
    case '-':
    case '=':
    case '<':
    case '>':
    case '.':
    case ':':
    case ';':
    case '\\':
      return lex->kind = c;
    default:
      break;
  }
  if (is_digit(c)) {
    long long unboxed = 0;
    do {
      unboxed = 10 * unboxed + c -'0';
      if (unboxed > INT_MAX) return lex->kind = -1;
    } while (is_digit(c = fgetc(lex->file)));
    ungetc(c, lex->file);
    if (vm->sp - vm->hp < 1) { fprintf(stderr, "Error: memory is out\n"); return lex->kind = -1; }
    lex->value = new_integer(vm, unboxed);
    return lex->kind = '0';
  }
  if (is_alpha(c)) {
    cell_t *value = new_nil();
    do {
      if (vm->sp - vm->hp < 2) { fprintf(stderr, "Error: memory is out\n"); return lex->kind = -1; }
      value = new_cons(vm, new_symbol(vm, c), value);
    } while (is_alpha(c = fgetc(lex->file)));
    ungetc(c, lex->file);
    lex->value = value;
    return lex->kind = 'a';
  }
  return lex->kind = -1;
}

cell_t *parse_stms(struct vm *vm, struct lexer *lex);
cell_t *parse_stmt(struct vm *vm, struct lexer *lex);
cell_t *parse_expr(struct vm *vm, struct lexer *lex);
cell_t *parse_comp(struct vm *vm, struct lexer *lex);
cell_t *parse_sumb(struct vm *vm, struct lexer *lex);
cell_t *parse_prod(struct vm *vm, struct lexer *lex);
cell_t *parse_appl(struct vm *vm, struct lexer *lex);
cell_t *parse_term(struct vm *vm, struct lexer *lex);

cell_t *parse_stms(struct vm *vm, struct lexer *lex) {
  cell_t *stms = new_nil();
  while (lex->kind != 0) {
    cell_t *stmt = parse_stmt(vm, lex);
    if (stmt == NULL) return NULL;
    if (vm->sp - vm->hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
    stms = new_cons(vm, stmt, stms);
  }
  return stms;
}

cell_t *parse_stmt(struct vm *vm, struct lexer *lex) {
  if (lex->kind != 'a') {
    fprintf(stderr, "Error: name expected in defenition\n");
    return NULL;
  }
  cell_t *name = lex->value;
  tokenize(vm, lex);
  if (lex->kind != ':') {
    fprintf(stderr, "Error: ':' expected in defenition\n");
    return NULL;
  }
  tokenize(vm, lex);
  cell_t *expr = parse_expr(vm, lex);
  if (expr == NULL) return NULL;
  if (vm->sp - vm->hp < 2) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
  cell_t *stmt = new_cons(vm, new_identifier(vm, name), expr);
  if (lex->kind != ';') {
    fprintf(stderr, "Error: ';' expected in the end of defenition\n");
    return NULL;
  }
  tokenize(vm, lex);
  return stmt;
}

cell_t *parse_expr(struct vm *vm, struct lexer *lex) {
  if (lex->kind != '\\') {
    return parse_comp(vm, lex);
  }
  tokenize(vm, lex);
  if (lex->kind != 'a') {
    fprintf(stderr, "Error: expected param of lambda\n");
    return NULL;
  }
  cell_t *name = lex->value;
  tokenize(vm, lex);
  if (lex->kind != '.') {
    fprintf(stderr, "Error: expected '.'\n");
    return NULL;
  }
  tokenize(vm, lex);
  cell_t *body = parse_expr(vm, lex);
  if (body == NULL) return NULL;
  if (vm->sp - vm->hp < 2) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
  cell_t *lambda = new_lambda(vm, new_identifier(vm, name), body);
  return lambda;
}

cell_t *parse_comp(struct vm *vm, struct lexer *lex) {
  cell_t *left = parse_sumb(vm, lex);
  if (left == NULL) return NULL;
  int op;
  switch (lex->kind) {
    case '=':
      op = OP_EQUAL;
      break;
    case '<':
      op = OP_LESS;
      break;
    case '>':
      op = OP_GREATER;
      break;
    default:
      return left;
  }
  tokenize(vm, lex);
  cell_t *right = parse_sumb(vm, lex);
  if (right == NULL) return NULL;
  if (vm->sp - vm->hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
  return new_binop(vm, left, right, op);
}

cell_t *parse_sumb(struct vm *vm, struct lexer *lex) {
  cell_t *sumb = parse_prod(vm, lex);
  if (sumb == NULL) return NULL;
  while (lex->kind == '+' || lex->kind == '-') {
    char kind = lex->kind == '+' ? OP_ADD : OP_SUB;
    tokenize(vm, lex);
    cell_t *prod = parse_prod(vm, lex);
    if (prod == NULL) return NULL;
    if (vm->sp - vm->hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
    sumb = new_binop(vm, sumb, prod, kind);
  }
  return sumb;
}

cell_t *parse_prod(struct vm *vm, struct lexer *lex) {
  cell_t *prod = parse_appl(vm, lex);
  if (prod == NULL) return NULL;
  while (lex->kind == '*') {
    tokenize(vm, lex);
    cell_t *appl = parse_appl(vm, lex);
    if (appl == NULL) return NULL;
    if (vm->sp - vm->hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
    prod = new_binop(vm, prod, appl, OP_MUL);
  }
  return prod;
}

cell_t *parse_appl(struct vm *vm, struct lexer *lex) {
  cell_t *appl = parse_term(vm, lex);
  if (appl == NULL) return NULL;
  while (lex->kind == '0' || lex->kind == 'a' || lex->kind == '(') {
    cell_t *term = parse_term(vm, lex);
    if (term == NULL) return NULL;
    if (vm->sp - vm->hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
    appl = new_application(vm, appl, term);
  }
  return appl;
}

cell_t *parse_term(struct vm *vm, struct lexer *lex) {
  cell_t *term, *expr;
  switch (lex->kind) {
    case '0':
      if (vm->sp - vm->hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
      term = new_literal(vm, lex->value);
      tokenize(vm, lex);
      return term;
    case 'a':
      if (vm->sp - vm->hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
      term = new_identifier(vm, lex->value);
      tokenize(vm, lex);
      return term;
    case '(':
      tokenize(vm, lex);
      expr = parse_expr(vm, lex);
      if (expr == NULL) return NULL;
      if (lex->kind != ')') {
        fprintf(stderr, "Error: ')' expected\n");
        return NULL;
      }
      tokenize(vm, lex);
      return expr;
    default:
      fprintf(stderr, "Error: worng term\n");
      return NULL;
  }
}

cell_t *parse(struct vm *vm, FILE *file) {
  struct lexer lex = { .file = file, .value = 0, .kind = 0 };
  tokenize(vm, &lex);
  return parse_stms(vm, &lex);
}

void print_cell(cell_t *cell);
void print_ast(cell_t *cell);

void print_cell(cell_t *cell) {
  switch (TAG(cell)) {
    case NIL_TAG:
      printf("<nil>");
      return;
    case INTEGER_TAG:
      printf("%d", AS_INTEGER(cell).unboxed);
      return;
    case SYMBOL_TAG:
      printf("%c", AS_SYMBOL(cell).unboxed);
      return;
    case CONS_TAG:
      printf("(");
      do {
        print_cell(AS_CONS(cell).head);
        cell = AS_CONS(cell).tail;
        printf(" ");
      } while (TAG(cell) == CONS_TAG);
      if (TAG(cell) == NIL_TAG) {
        printf(")");
        return;
      }
      printf(". ");
      print_cell(cell);
      printf(")");
      return;
    case CLOSURE_TAG:
      printf("<closure>");
      return;
    case LITERAL_TAG:
    case IDENTIFIER_TAG:
    case LAMBDA_TAG:
    case APPLICATION_TAG:
    case BINOP_TAG:
      print_ast(cell);
      return;
    default:
      printf("X3");
      return;
  }
}

void print_ast(cell_t *cell) {
  switch (TAG(cell)) {
    case LITERAL_TAG:
      printf("%d", cell->data.literal.object->data.integer.unboxed);
      return;
    case IDENTIFIER_TAG:
      for (cell_t *name = cell->data.identifier.name; TAG(name) != NIL_TAG; name = AS_CONS(name).tail) {
        cell_t *symbol = AS_CONS(name).head;
        printf("%c", AS_SYMBOL(symbol).unboxed);
      }
      return;
    case LAMBDA_TAG:
      printf("\\");
      print_ast(cell->data.lambda.param);
      printf(" -> ");
      print_ast(cell->data.lambda.body);
      return;
    case APPLICATION_TAG:
      printf("(");
      print_ast(cell->data.application.function);
      printf(" ");
      print_ast(cell->data.application.argument);
      printf(")");
      return;
    case BINOP_TAG:
      printf("(");
      print_ast(cell->data.binop.left);
      switch (IND(cell)) {
        case OP_ADD:
          printf(" + ");
          break;
        case OP_SUB:
          printf(" - ");
          break;
        case OP_MUL:
          printf(" * ");
          break;
        case OP_EQUAL:
          printf(" = ");
          break;
        case OP_LESS:
          printf(" < ");
          break;
        case OP_GREATER:
          printf(" > ");
          break;
      }
      print_ast(cell->data.binop.right);
      printf(")");
      return;
    default:
      printf("X3");
  }
}

#define VALUE(C) (AS_CONS(C).head)

int main() {
  static cell_t cells[1024];
  static struct vm vm = {
    .bp = cells,
    .ep = cells + 1024,
    .hp = cells,
    .sp = cells + 1024,
    .ar = NULL,
    .gr = NULL
  };
  cell_t *cell = parse(&vm, stdin);
  if (cell == NULL) return 1;
  print_cell(cell);
  printf("\n");
  // cell MODULE:nil
  vm.sp -= 1;
  VALUE(vm.sp + 0) = new_opcode(OP_MODULE);
  vm.ar = cell;
  vm.gr = new_nil();
  printf("RUN...\n");
  cell = eval(&vm);
  if (cell == NULL) return 1;
  print_cell(AS_CONS(AS_CONS(cell).head).tail);
  return 0;
}