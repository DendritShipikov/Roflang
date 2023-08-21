#include <stdio.h>
#include <stdlib.h>

#include "roflang.h"

static cell_t *parse_expr(struct parser *p);
static cell_t *parse_lambda(struct parser *p);
static cell_t *parse_term(struct parser *p);
static cell_t *parse_factor(struct parser *p);
static cell_t *parse_item(struct parser *p);
static cell_t *parse_name(struct parser *p);
static cell_t *parse_number(struct parser *p);

static int is_space(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
static int is_alpha(int c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }
static int is_digit(int c) { return '0' <= c && c <= '9'; }

static char tokenize(struct parser *p) {
	while (is_space(*p->cur)) {
		++p->cur;
	}
	return *p->cur;
}

cell_t *parse(struct parser *p) {
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
		// TODO: memcheck 3
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
	if (c == '@') {
		return parse_lambda(p);
	}
	cell_t *expr = parse_term(p);
	if (expr == NULL) {
		return NULL;
	}
	for (;;) {
		c = tokenize(p);
		if (c != '+' && c != '-') {
			return expr;
		}
		++p->cur;
		cell_t *term = parse_term(p);
		if (term == NULL) {
			return NULL;
		}
		// TODO: memcheck 1
		cell_t *tmp = p->top++;
		make_binex(tmp, c == '+' ? EXT_ADD : EXT_SUB, expr, term);
		expr = tmp;
	}
}

cell_t *parse_lambda(struct parser *p) {
	char c = *++p->cur;
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
	// TODO: memcheck 1
	cell_t *expr = p->top++;
	make_lamex(expr, param, body);
	return expr;
}

cell_t *parse_term(struct parser *p) {
	cell_t *term = parse_factor(p);
	if (term == NULL) {
		return NULL;
	}
	for (;;) {
		char c = tokenize(p);
		if (c != '*') {
			return term;
		}
		++p->cur;
		cell_t *factor = parse_factor(p);
		if (factor == NULL) {
			return NULL;
		}
		// TODO: memcheck 1
		cell_t *tmp = p->top++;
		make_binex(tmp, EXT_MUL, term, factor);
		term = tmp;
	}
}

cell_t *parse_factor(struct parser *p) {
	tokenize(p);
	cell_t *factor = parse_item(p);
	if (factor == NULL) {
		return NULL;
	}
	for (;;) {
		char c = tokenize(p);
		if (!is_alpha(c) && c != '(' && !is_digit(c)) {
			return factor;
		}
		cell_t *item = parse_item(p);
		if (item == NULL) {
			return NULL;
		}
		// TODO: memcheck 1
		cell_t *tmp = p->top++;
		make_appex(tmp, factor, item);
		factor = tmp;
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
		// TODO: memcheck 1
		cell_t *item = p->top++;
		cell_t *name = parse_name(p);
		if (name == NULL) {
			return NULL;
		}
		make_varex(item, name);
		return item;
	}
	if (is_digit(c)) {
		cell_t *num = parse_number(p);
		if (num == NULL) {
			return NULL;
		}
		// TODO: memcheck 1
		cell_t *item = p->top++;
		make_litex(item, num);
		return item;
	}
	fprintf(stderr, "Error: wrong item\n");
	return NULL;
}

cell_t *parse_name(struct parser *p) {
	static cell_t nil = { .object = { .tag = TAG_NIL, .markword = 0 } };
	cell_t *name = &nil;
	cell_t *top = p->top;
	while (is_alpha(*p->cur)) {
		// TODO: memcheck 1
		cell_t *tmp = p->top++;
		make_symbol(tmp, *p->cur++, name);
		name = tmp;
	}
	for (cell_t *iter = p->names; iter != NULL; iter = FORWARD(iter)) {
		cell_t *x = name, *y = iter;
		while (IS_SYMBOL(x) && IS_SYMBOL(y)) {
			if (AS_SYMBOL(x).head != AS_SYMBOL(y).head) {
				break;
			}
			x = AS_SYMBOL(x).tail;
			y = AS_SYMBOL(y).tail;
		}
		if (!IS_SYMBOL(x) && !IS_SYMBOL(y)) {
			p->top = top;
			return iter;
		}
	}
	FORWARD(name) = p->names;
	p->names = name;
	return name;
}

cell_t *parse_number(struct parser *p) {
	int n = 0;
	while (is_digit(*p->cur)) {
		n = 10 * n + *p->cur++ - '0';
	}
	// TODO: memcheck 1
	cell_t *num = p->top++;
	make_integer(num, n);
	return num;
}
