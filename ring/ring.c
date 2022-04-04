#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>

#include "ring.h"

int ring_init(struct ring *r)
{
	memset(r, 0, sizeof(struct ring));

	return 0;
}

void ring_deinit(struct ring __attribute__((unused)) *r)
{
}

int ticket_push(struct ring *r)
{
	int ret = atomic_fetch_add(&r->usage, 1);
	if (ret > RING_LIMIT-1) {
		atomic_fetch_sub(&r->usage, 1);
		return -1;
	}

	do {
		ret = r->rear;
	} while (!__sync_bool_compare_and_swap(&r->rear, ret, (ret+1)&(RING_LIMIT-1)));

	return ret;
}

int ticket_pop(struct ring *r)
{
	int ret = atomic_fetch_sub(&r->usage, 1);
	if (ret < 1) {
		atomic_fetch_add(&r->usage, 1);
		return -1;
	}

	do {
		ret = r->front;
	} while (!__sync_bool_compare_and_swap(&r->front, ret, (ret+1)&(RING_LIMIT-1)));

	return ret;
}

int ring_push(struct ring *r, void *data)
{
	int idx = ticket_push(r);
	if (idx < 0) {
		return -1;
	}

	while (r->data[idx]) {
		sched_yield();
	}
	r->data[idx] = data;

	return 0;
}

void *ring_pop(struct ring *r)
{
	int idx = ticket_pop(r);
	void *data = NULL;

	if (idx < 0) {
		return NULL;
	}

	while (r->data[idx] == NULL) { 
		sched_yield();
	}
	data = r->data[idx];
//	while (data == NULL) {
//		data = r->data[idx];
//	}

	r->data[idx] = NULL;

	return data;
}
