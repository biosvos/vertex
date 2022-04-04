#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "vertex.h"

void *two(void *data, void __attribute__((unused)) *private) {
    int *a = data;
    free(a);

    return NULL;
}

void *produce(void __attribute__((unused)) *data, void *private) {
    int *i = private;
    int *tmp = NULL;

    if (*i > 10000000) {
        printf("%lu quit\n", pthread_self());
        return VERTEX_QUIT;
    }

    tmp = malloc(sizeof(int));
    *tmp = *i;

    *i = *i + 1;

    return tmp;
}

int main(void) {
    int i = 0;
    int j = 0;

    struct vertex *producer1 = vertices(0, produce, &i);
    struct vertex *producer2 = vertices(0, produce, &j);
    struct vertex *merge = vertices(20, two, NULL);
    printf("producer1 %p\n", (void *) producer1);
    printf("producer2 %p\n", (void *) producer2);
    printf("merge %p\n", (void *) merge);

    edge(producer1, merge);
    edge(producer2, merge);

    vertex_start(producer1);
    vertex_start(producer2);

    vertex_stop(producer1);
    vertex_stop(producer2);

    printf("%d %d\n", i, j);

    return 0;
}
