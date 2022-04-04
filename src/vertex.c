#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ring.h"
#include "vertex.h"

static int vertex_ref_inc(struct vertex *vtx) {
    int r = 0;
    if (vtx == NULL) {
        return -1;
    }

    r = atomic_fetch_add(&vtx->ref_cnt, 1);
    return r + 1;
}

static int vertex_ref_dec(struct vertex *vtx) {
    int r = 0;
    if (vtx == NULL) {
        return -1;
    }

    r = atomic_fetch_sub(&vtx->ref_cnt, 1);
    return r - 1;
}

void nsleep(uint64_t ns) {
    struct timespec t = {0};
    t.tv_sec = ns / 1000000000UL;
    t.tv_nsec = ns % 1000000000UL;
    nanosleep(&t, NULL);
}

#define _1MS (1000000)

static void *vertex_get(struct vertex_input *v_input) {
    void *data = NULL;
    int pass = 1;
    int r = 0;

    while ((data = ring_pop(&v_input->input)) == NULL) { // 데이터 없음
        atomic_fetch_add(&v_input->put_penalty, 1);
        r = atomic_load(&v_input->get_penalty);
        if (r <= 0) {
            nsleep(_1MS);
        } else {
            nsleep(_1MS / r);
        }

        sched_yield();
        pass = 0;
    }

    if (pass) {
        r = atomic_fetch_sub(&v_input->put_penalty, 1);
        if (r < 1) {
            r = atomic_fetch_sub(&v_input->put_penalty, r - 1);
        }
    }

    return data;
}

void vertex_put(struct vertex *vtx, void *data) {
    struct vertex_input *share = NULL;
    int pass = 1;
    int r = 0;

    assert(data);

    share = vtx->v_input;

    while (ring_push(&share->input, data) < 0) {
        atomic_fetch_add(&share->get_penalty, 1);
        r = atomic_load(&share->put_penalty);
        if (r <= 0) {
            nsleep(_1MS);
        } else {
            nsleep(_1MS / r);
        }
        sched_yield();
        pass = 0;
    }

    if (pass) {
        r = atomic_fetch_sub(&share->get_penalty, 1);
        if (r < 1) {
            r = atomic_fetch_sub(&share->get_penalty, r - 1);
        }
    }
}

static inline uint64_t as_nanoseconds(struct timespec *ts) {
    return ts->tv_sec * (uint64_t) 1000000000L + ts->tv_nsec;
}

static void ____vertex_stop(struct vertex *vtx) {
    int i = 0;
    for (i = 0; i < vtx->nr_thread; ++i) {
        vertex_put(vtx, VERTEX_QUIT);
    }
}

static void *thread(void *data) {
    struct vertex *vtx = data;
    struct vertex *out = vtx->outvtx;
    struct vertex_input *v_input = vtx->v_input;

    void *item = NULL;
    int ref = 0;

    // for statistics
    uint64_t start, end;
    struct timespec ts;
    uint64_t total_consume_time = 0, nr_consume = 0;

    vertex_ref_inc(out);

    while (1) {
        if (v_input) {
            item = vertex_get(v_input);
            assert(item);
            if (item == VERTEX_QUIT) {
                break;
            }
        }

        clock_gettime(CLOCK_REALTIME, &ts);
        start = as_nanoseconds(&ts);
        if (v_input) {
            item = vtx->process(item, vtx->context);
        } else {
            item = vtx->process(vtx->state, vtx->context);
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        end = as_nanoseconds(&ts);
        nr_consume += 1;
        total_consume_time += end - start;

        if (item == VERTEX_QUIT) {
            break;
        }

        if (out && item) {
            vertex_put(out, item);
        }
    }

    ref = vertex_ref_dec(out);
    if (ref == 0) {
        ____vertex_stop(out);
    }

    // for statistics
    printf("vertex: %p count: %" PRIu64 " total time: %" PRIu64 ".%.9" PRIu64
           "\n", (void *) vtx, nr_consume, (total_consume_time) / 1000000000UL, (total_consume_time) % 1000000000UL);
    return NULL;
}

static struct vertex_input *vertex_input_setup() {
    struct vertex_input *v_input = NULL;

    v_input = malloc(sizeof(struct vertex_input));
    if (v_input == NULL) {
        return NULL;
    }
    memset(v_input, 0, sizeof(struct vertex_input));

    atomic_init(&v_input->put_penalty, 0);
    atomic_init(&v_input->get_penalty, 0);

    return v_input;
}

struct vertex *vertices(int nr, void *(*func)(void *, void *), void *context) {
    struct vertex *vtx = NULL;
    size_t memsize = 0;
    struct vertex_input *v_input = NULL;

    assert(func);

    if (nr > 0) {
        v_input = vertex_input_setup();
        if (v_input == NULL) {
            return NULL;
        }
    } else {
        nr = 1;
    }

    memsize = sizeof(struct vertex) + sizeof(pthread_t) * nr;

    vtx = malloc(memsize);
    if (vtx == NULL) {
        if (v_input) {
            free(v_input);
        }
        return NULL;
    }
    memset(vtx, 0, memsize);

    vtx->nr_thread = nr;
    vtx->v_input = v_input;
    vtx->context = context;
    vtx->process = func;
    atomic_init(&vtx->ref_cnt, 0);

    return vtx;
}

void edge(struct vertex *from, struct vertex *to) {
    assert(to->v_input);
    assert(from->tid[0] == 0);
    assert(to->tid[0] == 0);
    from->outvtx = to;
}

/**
 * @brief Thread 종료
 *
 * @param vtx
 * @return free 여부
 * 	1: free
 * 	0: free하면 안됨
 */
static int thread_stop(struct vertex *vtx) {
    int cnt = vtx->nr_thread;
    int i = 0;
    int r = 0;

    if (atomic_fetch_sub(&vtx->nr_running, 1) != 1) {
        return 0;
    }

    for (i = 0; i < cnt; ++i) {
        if (vtx->tid[i] == 0) {
            break;
        }
        r = pthread_join(vtx->tid[i], NULL);
        assert(r == 0);
    }

    return 1;
}

static void thread_start(struct vertex *vtx) {
    int cnt = vtx->nr_thread;
    int i = 0;
    int r = 0;

    if (atomic_fetch_add(&vtx->nr_running, 1) != 0) { // already running
        return;
    }

    for (i = 0; i < cnt; ++i) {
        r = pthread_create(&vtx->tid[i], NULL, thread, vtx);
        assert(r == 0);
    }
}

void vertex_start(struct vertex *vtx) {
    struct vertex *cur = vtx;

    // outvtx에는 ring 버퍼에 넣는 것이므로,
    // stack을 이용해 마지막 vtx부터 반대로 실행 시킬 필요 없음
    while (cur) {
        thread_start(cur);
        cur = cur->outvtx;
    }
}

void vertex_stop(struct vertex *vtx) {
    int i = 0;
    struct vertex *next = NULL;
    int is_free = 0;

    // 첫 vtx에 VERTEX_QUIT 전송 혹은 state 변경
    if (vtx->v_input) {
        for (i = 0; i < vtx->nr_thread; ++i) {
            vertex_put(vtx, VERTEX_QUIT);
        }
    } else {
        vtx->state = VERTEX_QUIT;
    }

    // join을 통한 thread 자원 소거
    while (vtx) {
        next = vtx->outvtx;
        is_free = thread_stop(vtx);

        if (is_free) {
            if (vtx->v_input) {
                printf("vertex: %p put penalty %d get penalty %d\n", (void *) vtx,
                       atomic_load(&vtx->v_input->put_penalty), atomic_load(&vtx->v_input->get_penalty));
                free(vtx->v_input);
            }

            free(vtx);
        }

        vtx = next;
    }
}
