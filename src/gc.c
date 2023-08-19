#include <stdlib.h>
#include <stdio.h>

#include "roflang.h"

static void walk_context(struct context *ctx, void (*visit)(cell_t **)) {
	cell_t *p = ctx->sp;
	cell_t *q = ctx->fp;
	while (q != NULL) {
		for (; p < q; ++p) {
			visit(&p->ref.value);
		}
		switch (q->frame.op) {
		case OP_EVAL:
		case OP_BINCONT:
			visit(&q->frame.r2);
		case OP_BIN:
		case OP_APPLY:
		case OP_UPDATE:
		case OP_RETURN:
			visit(&q->frame.r1);
			break;
		default:
			fprintf(stderr, "Fatal error: wrong op while walking context\n");
			exit(1);
		}
		p = q + 1;
		q = q->frame.bp;
	}
	visit(&ctx->gp);
}

static void walk_object(cell_t *p, void (*visit)(cell_t **)) {
	switch (TAG(p)) {
	case TAG_NIL:
	case TAG_HOLE:
	case TAG_INTEGER:
		break;
	case TAG_SYMBOL:
		visit(&AS_SYMBOL(p).tail);
		break;
	case TAG_PAIR:
		visit(&AS_PAIR(p).head);
		visit(&AS_PAIR(p).tail);
		break;
	case TAG_CLOSURE:
		visit(&AS_CLOSURE(p).expr);
		visit(&AS_CLOSURE(p).env);
		break;
	case TAG_THUNK:
		visit(&AS_THUNK(p).expr);
		visit(&AS_THUNK(p).env);
		break;
	case TAG_APPEX:
		visit(&AS_APPEX(p).function);
		visit(&AS_APPEX(p).argument);
		break;
	case TAG_BINEX:
		visit(&AS_BINEX(p).left);
		visit(&AS_BINEX(p).right);
		break;
	case TAG_LAMEX:
		visit(&AS_LAMEX(p).param);
		visit(&AS_LAMEX(p).body);
		break;
	case TAG_VAREX:
		visit(&AS_VAREX(p).name);
		break;
	case TAG_LITEX:
		visit(&AS_LITEX(p).object);
		break;
	default:
		fprintf(stderr, "Fatal error: wrong tag while walking object\n");
		exit(1);
	}
}

#define SET_REACHABLE(C) (MARKWORD(C) = 1)
#define SET_MOVEABLE(C) (MARKWORD(C) = 2)
#define IS_REACHABLE(C) (MARKWORD(C) == 1)
#define IS_MOVEABLE(C) (MARKWORD(C) == 2)

static cell_t *mark_stack;

static void mark(cell_t **pp) {
	cell_t *p = *pp;
	if (IS_INDIRECT(p)) {
		p = AS_INDIRECT(p).actual;
	}
	if (!IS_REACHABLE(p)) {
		SET_REACHABLE(p);
		FORWARD(p) = mark_stack;
		mark_stack = p;
	}
}

static void adjust(cell_t **pp) {
	cell_t *p = *pp;
	if (IS_INDIRECT(p)) {
		p = AS_INDIRECT(p).actual;
	}
	if (IS_MOVEABLE(p)) {
		*pp = FORWARD(p);
	} else {
		*pp = p;
	}
}

void compact(struct context *ctx) {
	cell_t *p, *q;
	cell_t *mp = ctx->mp;
	cell_t *hp = ctx->hp;
	mark_stack = NULL;

	/* roots */
	walk_context(ctx, mark);

	/* mark */
	while (mark_stack != NULL) {
		p = mark_stack;
		mark_stack = FORWARD(p);
		walk_object(p, mark);
	}

	/* calculate */
	for (q = p = mp; p < hp; ++p) {
		if (MARKWORD(p)) {
			FORWARD(p) = q++;
			SET_MOVEABLE(p);
		}
	}
	ctx->hp = q;

	/* adjust */
	walk_context(ctx, adjust);
	for (p = mp; p < hp; ++p) {
		if (MARKWORD(p)) {
			walk_object(p, adjust);
		}
	}

	/* move */
	for (p = mp; p < hp; ++p) {
		if (MARKWORD(p)) {
			MARKWORD(p) = 0;
			q = FORWARD(p);
			if (q != p) {
				*q = *p;
			}
		}
	}
}
