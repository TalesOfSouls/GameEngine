#include "../TestFramework.h"
#include "../../memory/Queue.h"
#include "../../memory/QueueT.h"

static void test_queue_alloc() {
    Queue mem = {0};
    queue_alloc(&mem, 10, 20);

    // Every element is aligned according to the default alignment -> size >= constructor size
    TEST_EQUALS(mem.size, 256);

    queue_free(&mem);
    TEST_EQUALS(mem.size, 0);
    TEST_EQUALS(mem.memory, NULL);
}

#if PERFORMANCE_TEST
static void _my_queue(MAYBE_UNUSED volatile void* val) {
    Queue queue = {0};
    queue_alloc(&queue, 999, sizeof(size_t));

    // First insert 333 elements
    for (size_t i = 0; i < 333; ++i) {
        queue_enqueue(&queue, (byte *) &i);
    }

    // Then insert/dequeue 333 elements
    size_t temp;
    size_t total = 0;
    for (size_t i = 0; i < 333; ++i) {
        queue_enqueue(&queue, (byte *) &i);
        queue_dequeue(&queue, (byte *) &temp);
        total += temp;
    }

    // Finally dequeue remaining 333 elements
    for (size_t i = 0; i < 333; ++i) {
        queue_dequeue(&queue, (byte *) &temp);
        total += temp;
    }

    TEST_TRUE(total);

    queue_free(&queue);
}

static void _my_queuet(MAYBE_UNUSED volatile void* val) {
    QueueT<size_t> queue = {0};
    queue_alloc(&queue, 999, 8);

    // First insert 333 elements
    for (size_t i = 0; i < 333; ++i) {
        queue_enqueue(&queue, &i);
    }

    // Then insert/dequeue 333 elements
    size_t temp;
    size_t total = 0;
    for (size_t i = 0; i < 333; ++i) {
        queue_enqueue(&queue, &i);
        queue_dequeue(&queue, &temp);
        total += temp;
    }

    // Finally dequeue remaining 333 elements
    for (size_t i = 0; i < 333; ++i) {
        queue_dequeue(&queue, &temp);
        total += temp;
    }

    TEST_TRUE(total);

    queue_free(&queue);
}

static void test_queue_performance() {
    COMPARE_FUNCTION_TEST_TIME(_my_queuet, _my_queue, -500.0);
}
#endif

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main QueueTest
#endif

int main() {
    TEST_INIT(25);

    TEST_RUN(test_queue_alloc);

    #if PERFORMANCE_TEST
        TEST_RUN(test_queue_performance);
    #endif

    TEST_FINALIZE();

    return 0;
}
