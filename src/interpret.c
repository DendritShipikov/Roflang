#include <stdio.h>
#include <stdlib.h>

#include "roflang.h"

#define NEW() (free_mem++)
#define POP() ((sp++)->obj)
#define PUSH(C) ((--sp)->obj = (C))

#define STACK_CHECK(N) \
	do { \
		if (sp - ep < N + 2) { \
			fprintf(stderr, "Fatal error: stack overflow\n"); \
			exit(1); \
		} \
	} while (0)

#define GC() \
	do { \
		ctx->sp = sp; \
		ctx->fp = fp; \
		ctx->free_mem = free_mem; \
		compact(ctx); \
		free_mem = ctx->free_mem; \
	} while (0)

cell_t *interpret(struct context *ctx) {
	value_t *ep, *sp, *fp, *bp;
	cell_t *free_mem, *meta_mem, *binds;
	cell_t *expr, *env, *obj, *name, *iter, *pair, *tmp, *left, *right;
	unsigned char op;
	/* init */
	free_mem = ctx->free_mem;
	meta_mem = ctx->meta_mem;
	binds = ctx->binds;
	ep = ctx->ep;
	sp = ctx->sp;
	fp = ctx->fp;
	bp = ctx->bp;
	obj = AS_PAIR(AS_PAIR(binds).head).tail;
enter:
	/* arguments: obj */
	switch (TAG(obj)) {
	case TAG_HOLE:
		fprintf(stderr, "Fatal error: cyclic dependency\n");
		exit(1);
	case TAG_NIL:
	case TAG_PAIR:
	case TAG_INTEGER:
		goto cont;
	case TAG_INDIRECT:
		obj = AS_INDIRECT(obj).actual;
		goto enter;
	case TAG_CLOSURE:
		if (sp == fp) {
			goto cont;
		}
		expr = AS_CLOSURE(obj).expr;
		env = AS_CLOSURE(obj).env;
		goto eval;
	case TAG_THUNK:
		STACK_CHECK(3);
		expr = AS_CLOSURE(obj).expr;
		env = AS_CLOSURE(obj).env;
		make_hole(obj);
		{
			/* push update */
			PUSH(obj);
			(--sp)->fp = fp;
			(--sp)->op = OP_UPDATE;
			fp = sp;
		}
		goto eval;
	default:
		fprintf(stderr, "Fatal error: wrong tag for apply\n");
		exit(1);
	}
eval:
	/* arguments: expr, env */
	switch (TAG(expr)) {
	case TAG_LITEX:
		obj = AS_LITEX(expr).object;
		goto enter;
	case TAG_VAREX:
		name = AS_VAREX(expr).name;
		for (iter = env; IS_PAIR(iter); iter = AS_PAIR(iter).tail) {
			pair = AS_PAIR(iter).head;
			if (name == AS_PAIR(pair).head) {
				obj = AS_PAIR(pair).tail;
				goto enter;
			}
		}
		for (iter = binds; IS_PAIR(iter); iter = AS_PAIR(iter).tail) {
			pair = AS_PAIR(iter).head;
			if (name == AS_PAIR(pair).head) {
				obj = AS_PAIR(pair).tail;
				goto enter;
			}
		}
		fprintf(stderr, "Fatal error: unbounded name\n");
		exit(1);
	case TAG_LAMEX:
		if (meta_mem - free_mem < 2) {
			PUSH(expr);
			PUSH(env);
			GC();
			if (meta_mem - free_mem < 2) {
				fprintf(stderr, "Runtime error: memory is out\n");
				exit(1);
			}
			env = POP();
			expr = POP();
		}
		if (sp == fp) {
			obj = NEW();
			make_closure(obj, expr, env);
			goto cont;
		}
		obj = POP();
		pair = NEW();
		make_pair(pair, AS_LAMEX(expr).param, obj);
		tmp = NEW();
		make_pair(tmp, pair, env);
		env = tmp;
		expr = AS_LAMEX(expr).body;
		goto eval;
	case TAG_APPEX:
		STACK_CHECK(4);
		{
			/* push eval */
			PUSH(AS_APPEX(expr).function);
			PUSH(env);
			(--sp)->fp = fp;
			(--sp)->op = OP_EVAL;
			fp = sp;
		}
		expr = AS_APPEX(expr).argument;
		goto eval;
	case TAG_BINEX:
		STACK_CHECK(4);
		{
			/* push right */
			PUSH(expr);
			PUSH(env);
			(--sp)->fp = fp;
			(--sp)->op = OP_RIGHT;
			fp = sp;
		}
		expr = AS_BINEX(expr).left;
		goto eval;
	default:
		fprintf(stderr, "Fatal error: wrong tag for eval\n");
		exit(1);
	}
cont:
	/* arguments: obj */
	if (fp == bp) {
		return obj;
	}
	sp = fp;
	op = (sp++)->op;
	fp = (sp++)->fp;
	switch (op) {
	case OP_UPDATE:
		tmp = POP();
		make_indirect(tmp, obj);
		goto enter;
	case OP_EVAL:
		env = POP();
		expr = POP();
		PUSH(obj);
		goto eval;
	case OP_RIGHT:
		env = POP();
		expr = POP();
		{
			/* push bin */
			PUSH(expr);
			PUSH(obj);
			(--sp)->fp = fp;
			(--sp)->op = OP_BIN;
			fp = sp;
		}
		expr = AS_BINEX(expr).right;
		goto eval;
	case OP_BIN:
		if (meta_mem - free_mem < 1) {
			PUSH(obj);
			GC();
			if (meta_mem - free_mem < 1) {
				fprintf(stderr, "Runtime error: memory is out\n");
				exit(1);
			}
			obj = POP();
		}
		right = obj;
		left = POP();
		expr = POP();
		obj = NEW();
		switch (EXTTAG(expr)) {
		case EXT_ADD:
			make_integer(obj, AS_INTEGER(left).value + AS_INTEGER(right).value);
			break;
		case EXT_SUB:
			make_integer(obj, AS_INTEGER(left).value - AS_INTEGER(right).value);
			break;
		case EXT_MUL:
			make_integer(obj, AS_INTEGER(left).value * AS_INTEGER(right).value);
			break;
		default:
			fprintf(stderr, "Fatal error: wrong extop in binary operator\n");
			exit(1);
		}
		goto cont;
	default:
		fprintf(stderr, "Fatal error: unknown op\n");
		exit(1);
	}
}
