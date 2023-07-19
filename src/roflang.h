#ifndef ROFLANG_H
#define ROFLANG_H

enum {
  TAG_NIL,
  TAG_PAIR,
  TAG_CLOSURE,
  TAG_THUNK,
  TAG_INDIRECT,
  TAG_APPEX,
  TAG_LAMEX,
  TAG_SYMEX,
  TAG_LITEX,
  TAG_INTEGER,
  TAG_HOLE,
};

typedef union cell {
  struct {
    unsigned char tag;
    union cell *forward;
    union {
      struct {
        union cell *head;
        union cell *tail;
      } pair;
      struct {
        union cell *expr;
        union cell *env;
      } closure, thunk;
      struct {
        union cell *fun;
        union cell *arg;
      } appex;
      struct {
        union cell *param;
        union cell *body;
      } lamex;
      struct {
        char name;
      } symex;
      struct {
        union cell *object;
      } litex;
      struct {
        union cell *actual;
      } indirect;
      struct {
        int unboxed;
      } integer;
    } payload;
  } object;
  struct {
    unsigned int op;
    union cell *r1;
    union cell *r2;
    union cell *bfp;
  } frame;
  struct {
    union cell *value;
  } ref;
} cell_t;

#define TAG(C) ((C)->object.tag)

#define AS_PAIR(C)     ((C)->object.payload.pair)
#define AS_CLOSURE(C)  ((C)->object.payload.closure)
#define AS_THUNK(C)    ((C)->object.payload.thunk)
#define AS_INDIRECT(C) ((C)->object.payload.indirect)
#define AS_INTEGER(C)  ((C)->object.payload.integer)
#define AS_APPEX(C)    ((C)->object.payload.appex)
#define AS_LAMEX(C)    ((C)->object.payload.lamex)
#define AS_SYMEX(C)    ((C)->object.payload.symex)
#define AS_LITEX(C)    ((C)->object.payload.litex)

#define IS_PAIR(C)     (TAG(C) == TAG_PAIR)
#define IS_CLOSURE(C)  (TAG(C) == TAG_CLOSURE)
#define IS_THUNK(C)    (TAG(C) == TAG_THUNK)
#define IS_INDIRECT(C) (TAG(C) == TAG_INDIRECT)
#define IS_INTEGER(C)  (TAG(C) == TAG_INTEGER)
#define IS_APPEX(C)    (TAG(C) == TAG_APPEX)
#define IS_LAMEX(C)    (TAG(C) == TAG_LAMEX)
#define IS_SYMEX(C)    (TAG(C) == TAG_SYMEX)
#define IS_LITEX(C)    (TAG(C) == TAG_LITEX)

void make_pair(cell_t *p, cell_t *head, cell_t *tail);
void make_closure(cell_t *p, cell_t *expr, cell_t *env);
void make_thunk(cell_t *p, cell_t *expr, cell_t *env);
void make_indirect(cell_t *p, cell_t *actual);
void make_integer(cell_t *p, int unboxed);
void make_appex(cell_t *p, cell_t *fun, cell_t *arg);
void make_lamex(cell_t *p, cell_t *param, cell_t *body);
void make_symex(cell_t *p, char name);
void make_litex(cell_t *p, cell_t *object);
void make_hole(cell_t *p);

void make_frame(cell_t *p, unsigned int op, cell_t *r1, cell_t *r2, cell_t *bfp);


struct context {
  cell_t *bp, *hp, *sp, *fp, *ep;
};

enum {
  OP_EVAL,
  OP_APPLY,
  OP_RETURN,
  OP_UPDATE,
};

cell_t *run(struct context *ctx);


struct parser {
  const char *cur;
  cell_t *top;
};

cell_t *parse_defs(struct parser *p);
cell_t *parse_expr(struct parser *p);
cell_t *parse_term(struct parser *p);
cell_t *parse_item(struct parser *p);


#endif
