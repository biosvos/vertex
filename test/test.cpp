#include <cstdlib>
#include <doctest/doctest.h>

#include "vertex.h"

void *produce(void __attribute__((unused)) *data, void *pp) {
    int *i = static_cast<int *>(pp);

    if (*i > 10000000) {
        return VERTEX_QUIT;
    }

    int *tmp = static_cast<int *>(malloc(sizeof(int)));
    *tmp = *i;
    *i = *i + 1;

    return tmp;
}

void *consumer(void *data, void __attribute__((unused)) *pp) {
    int *a = static_cast<int *>(data);
    free(a);

    return nullptr;
}

TEST_CASE("simple") {
    int i = 0;
    struct vertex *one = vertices(0, produce, &i);
    struct vertex *two = vertices(1, consumer, nullptr);

    edge(one, two);

    vertex_start(one);

    vertex_stop(one);

    CHECK(i == 10000001);
}