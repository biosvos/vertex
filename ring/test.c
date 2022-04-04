#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ring.h"

int main(void)
{
	struct ring *r;
	int i = 0;

	r = malloc(sizeof(struct ring));

	ring_init(r);

	int k = 0;

	printf("front rear usage\n");
	for (i = 0; i < 1024; ++i) {
		k = ring_push(r, &i);
		if (k < 0) {
			printf("%d %d\n", i, k);
		}
		printf("%d %d %d\n", r->front, r->rear, atomic_load(&r->usage));
	}

	int *x = NULL;
	for (i = 0; i < 10; ++i) {
		x = ring_pop(r);
		if (x == NULL) {
			printf("%d %d\n", i, k);
		}
	//	printf("%d %d %d\n", r->front, r->rear, atomic_load(&r->usage));
	}

	ring_deinit(r);
	return 0;
}
