#ifndef ROFLANG_H
#define ROFLANG_H

enum {
	TAG_NIL,
	TAG_PAIR,
	TAG_CLOSURE,
	TAG_THUNK,
	TAG_INDIRECT,
	TAG_BINEX,
	TAG_APPEX,
	TAG_LAMEX,
	TAG_VAREX,
	TAG_LITEX,
	TAG_INTEGER,
	TAG_SYMBOL,
	TAG_HOLE,
};

enum {
	EXT_ADD,
	EXT_SUB,
	EXT_MUL,
};

typedef struct cell {
	unsigned char tag;
	unsigned char exttag;
	unsigned char markword;
	struct cell *forward;
	union {
		struct {
			struct cell *head;
			struct cell *tail;
		} pair;
		struct {
			char head;
			struct cell *tail;
		} symbol;
		struct {
			struct cell *expr;
			struct cell *env;
		} closure, thunk;
		struct {
			struct cell *function;
			struct cell *argument;
		} appex;
		struct {
			struct cell *left;
			struct cell *right;
		} binex;
		struct {
			struct cell *param;
			struct cell *body;
		} lamex;
		struct {
			struct cell *name;
		} varex;
		struct {
			struct cell *object;
		} litex;
		struct {
			struct cell *actual;
		} indirect;
		struct {
			int value;
		} integer;
	} payload;
} cell_t;

#define TAG(C) ((C)->tag)
#define EXTTAG(C) ((C)->exttag)
#define FORWARD(C) ((C)->forward)
#define MARKWORD(C) ((C)->markword)

#define AS_PAIR(C)     ((C)->payload.pair)
#define AS_CLOSURE(C)  ((C)->payload.closure)
#define AS_THUNK(C)    ((C)->payload.thunk)
#define AS_INDIRECT(C) ((C)->payload.indirect)
#define AS_INTEGER(C)  ((C)->payload.integer)
#define AS_SYMBOL(C)   ((C)->payload.symbol)
#define AS_APPEX(C)    ((C)->payload.appex)
#define AS_BINEX(C)    ((C)->payload.binex)
#define AS_LAMEX(C)    ((C)->payload.lamex)
#define AS_VAREX(C)    ((C)->payload.varex)
#define AS_LITEX(C)    ((C)->payload.litex)

#define IS_PAIR(C)     (TAG(C) == TAG_PAIR)
#define IS_CLOSURE(C)  (TAG(C) == TAG_CLOSURE)
#define IS_THUNK(C)    (TAG(C) == TAG_THUNK)
#define IS_INDIRECT(C) (TAG(C) == TAG_INDIRECT)
#define IS_INTEGER(C)  (TAG(C) == TAG_INTEGER)
#define IS_SYMBOL(C)   (TAG(C) == TAG_SYMBOL)
#define IS_BINEX(C)    (TAG(C) == TAG_BINEX)
#define IS_APPEX(C)    (TAG(C) == TAG_APPEX)
#define IS_LAMEX(C)    (TAG(C) == TAG_LAMEX)
#define IS_VAREX(C)    (TAG(C) == TAG_VAREX)
#define IS_LITEX(C)    (TAG(C) == TAG_LITEX)

void make_pair(cell_t *p, cell_t *head, cell_t *tail);
void make_symbol(cell_t *p, char head, cell_t *tail);
void make_closure(cell_t *p, cell_t *expr, cell_t *env);
void make_thunk(cell_t *p, cell_t *expr, cell_t *env);
void make_indirect(cell_t *p, cell_t *actual);
void make_integer(cell_t *p, int value);
void make_appex(cell_t *p, cell_t *function, cell_t *argument);
void make_binex(cell_t *p, unsigned char exttag, cell_t *left, cell_t *right);
void make_lamex(cell_t *p, cell_t *param, cell_t *body);
void make_varex(cell_t *p, cell_t *name);
void make_litex(cell_t *p, cell_t *object);
void make_hole(cell_t *p);


typedef union value {
	unsigned int op;
	union value *fp;
	cell_t *obj;
} value_t;

struct context {
	value_t *ep, *sp, *fp, *bp;
	cell_t *heap_mem, *free_mem, *meta_mem;
	cell_t *binds;
	cell_t *names;
};

enum {
	OP_EVAL,
	OP_UPDATE,
	OP_RIGHT,
	OP_BIN,
};

void compact(struct context *ctx);
cell_t *interpret(struct context *ctx);


struct parser {
	const char *cur;
	cell_t *top;
	cell_t *names;
};

cell_t *parse(struct parser *p);


#endif
