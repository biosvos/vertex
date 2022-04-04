#ifndef RING_H_WXOABQJJ
#define RING_H_WXOABQJJ

#include "stdatomic.h"
#define RING_LIMIT 1024

struct ring {
	int front;
	int rear;
	atomic_int usage;
	void *data[RING_LIMIT];
};

int   ring_init(struct ring *r);
void  ring_deinit(struct ring *r);
int   ring_push(struct ring *r, void *data);
void *ring_pop(struct ring *r);
//#define RING_EMPTY(r) (atomic_load(&(r)->front) == atomic_load(&(r)->rear))
//#define RING_FULL(r) (atomic_load(&(r)->front) == (atomic_load(&(r)->rear)+1)%(r)->capacity)

#endif /* end of include guard: RING_H_WXOABQJJ */
