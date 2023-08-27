#include <stdio.h>
#include <stdlib.h>

#include "roflang.h"

int main(int argc, char **argv) {
	if (argc < 2) {
		return 0;
	}
	static cell_t nil;
	static cell_t mem[1024];
	static value_t stack[256];

	struct parser p = {
		.cur = argv[1],
		.top = mem,
		.names = NULL,
	};

	cell_t *binds = parse(&p);
	if (binds == NULL) {
		return 0;
	}
	printf("parsed\n");

	struct context ctx = {
		.ep = stack, .sp = stack + 256, .fp = stack + 256, .bp = stack + 256,
		.heap_mem = p.top, .free_mem = p.top, .meta_mem = mem + 1024,
		.binds = binds,
		.names = &nil,
	};
	cell_t *obj = interpret(&ctx);

	switch (TAG(obj)) {
	case TAG_INTEGER:
		printf("%d\n", AS_INTEGER(obj).value);
		break;
	case TAG_CLOSURE:
		printf("<closure>\n");
		break;
	default:
		printf("<obj>\n");
		break;
	}
	return 0;
}
