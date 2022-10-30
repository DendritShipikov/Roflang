#include "roflang.h"
#include <limits.h>

struct lexer {
  FILE *file;
  int kind;
  int value;
};

int is_space(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
int is_alpha(int c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }
int is_digit(int c) { return '0' <= c && c <= '9'; }

int tokenize(struct lexer *lex) {
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
    case '\\':
      return lex->kind = c;
    default:
      break;
  }
  if (is_digit(c)) {
    long long value = 0;
    do {
      value = 10 * value + c -'0';
      if (value > INT_MAX) return lex->kind = -1;
    } while (is_digit(c = fgetc(lex->file)));
    ungetc(c, lex->file);
    lex->value = (int) value;
    return lex->kind = '0';
  }
  if (is_alpha(c)) {
    lex->value = c;
    return lex->kind = 'a';
  }
  return lex->kind = -1;
}

cell_t *parse_expr(struct lexer *lex);
cell_t *parse_sumb(struct lexer *lex);
cell_t *parse_prod(struct lexer *lex);
cell_t *parse_appl(struct lexer *lex);
cell_t *parse_term(struct lexer *lex);

cell_t *parse_expr(struct lexer *lex) {
  if (lex->kind != '\\') {
    return parse_sumb(lex);
  }
  tokenize(lex);
  if (lex->kind != 'a') {
    fprintf(stderr, "Error: expected param of lambda\n");
    return NULL;
  }
  char name = (char) lex->value;
  tokenize(lex);
  cell_t *body = parse_expr(lex);
  if (body == NULL) return NULL;
  if (vm.sp - vm.hp < 2) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
  cell_t *lambda = new_lambda(new_identifier(name), body);
  return lambda;
}

cell_t *parse_sumb(struct lexer *lex) {
  cell_t *sumb = parse_prod(lex);
  if (sumb == NULL) return NULL;
  while (lex->kind == '+' || lex->kind == '-') {
    char kind = lex->kind == '+' ? OP_ADD : OP_SUB;
    tokenize(lex);
    cell_t *prod = parse_prod(lex);
    if (prod == NULL) return NULL;
    if (vm.sp - vm.hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
    sumb = new_binop(sumb, prod, kind);
  }
  return sumb;
}

cell_t *parse_prod(struct lexer *lex) {
  cell_t *prod = parse_appl(lex);
  if (prod == NULL) return NULL;
  while (lex->kind == '*') {
    tokenize(lex);
    cell_t *appl = parse_appl(lex);
    if (appl == NULL) return NULL;
    if (vm.sp - vm.hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
    prod = new_binop(prod, appl, OP_MUL);
  }
  return prod;
}

cell_t *parse_appl(struct lexer *lex) {
  cell_t *appl = parse_term(lex);
  if (appl == NULL) return NULL;
  while (lex->kind == '0' || lex->kind == 'a' || lex->kind == '(') {
    cell_t *term = parse_term(lex);
    if (term == NULL) return NULL;
    if (vm.sp - vm.hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
    appl = new_application(appl, term);
  }
  return appl;
}

cell_t *parse_term(struct lexer *lex) {
  cell_t *term, *expr;
  switch (lex->kind) {
    case '0':
      if (vm.sp - vm.hp < 2) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
      term = new_literal(new_integer(lex->value));
      tokenize(lex);
      return term;
    case 'a':
      if (vm.sp - vm.hp < 1) { fprintf(stderr, "Error: memory is out\n"); return NULL; }
      term = new_identifier((char) lex->value);
      tokenize(lex);
      return term;
    case '(':
      tokenize(lex);
      expr = parse_expr(lex);
      if (lex->kind != ')') {
        fprintf(stderr, "Error: ')' expected\n");
        return NULL;
      }
      tokenize(lex);
      return expr;
    default:
      fprintf(stderr, "Error: worng term\n");
      return NULL;
  }
}

cell_t *parse(FILE *file) {
  struct lexer lex = { .file = file, .value = 0, .kind = 0 };
  tokenize(&lex);
  return parse_expr(&lex);
}

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
      printf("<ast>");
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
      printf("%c", cell->data.identifier.name);
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
  cell_t nil = { .data = { .integer = {0} }, .forward = NULL, .tag = NIL_TAG, .ind = 0 };
  cell_t *cell = parse(stdin);
  if (cell == NULL) return 1;
  print_ast(cell);
  printf("\n");
  vm.sp -= 3;
  VALUE(vm.sp + 0) = &opcodes[OP_EVAL];
  VALUE(vm.sp + 1) = &nil;
  VALUE(vm.sp + 2) = &opcodes[OP_HALT];
  vm.ar = cell;
  printf("RUN...\n");
  cell = run();
  if (cell == NULL) return 1;
  print_cell(cell);
  return 0;
}