#ifndef VERTEX_H_T3DBWYBC
#define VERTEX_H_T3DBWYBC

#include <sys/types.h>
#include "stdatomic.h"
#include "ring/ring.h"

struct vertex_input {
	//struct lfq_ctx input;
	struct ring input;
	atomic_int put_penalty;
	atomic_int get_penalty;
};

struct vertex {
	struct vertex *outvtx;
	struct vertex_input *v_input;
	void *(*process)(void *item, void *private);
	void *private;
	atomic_int ref_cnt; // out 기준?
	void *state;

	// thread_start 가 수행된 횟수
	atomic_int nr_running; 

	int nr_thread;
	pthread_t tid[];
};

#define VERTEX_QUIT ((void *)-1)

#define vertex(cb, pr) vertices(1, cb, pr)
struct vertex *vertices(int nr, void *(*func)(void *, void *), void *private);

void edge(struct vertex *from, struct vertex *to);

void vertex_put(struct vertex *vtx, void *data);
void vertex_stop(struct vertex *vtx);
void vertex_start(struct vertex *vtx);

#endif /* end of include guard: VERTEX_H_T3DBWYBC */
