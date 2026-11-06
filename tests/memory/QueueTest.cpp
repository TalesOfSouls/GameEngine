#include "../TestFramework.h"
#include "../../memory/Queue.cpp"
#include "../../memory/QueueT.cpp"

struct QueueTestElement {
    int32 id;
    int32 value;
};

static void test_queue_alloc() {
    Queue mem = {0};
    queue_alloc(&mem, 10, 10, 20);

    // Every element is aligned according to the default alignment -> size >= constructor size
    // (10 + 1 empty slot) * ALIGN_UP(20, 8) = 264 -> aligned to the cache line size
    TEST_EQUALS(mem.size, 320);

    // The element size is aligned up as well
    TEST_EQUALS(mem.element_size, 24);

    queue_free(&mem);
    TEST_EQUALS(mem.size, 0);
    TEST_EQUALS(mem.memory, NULL);
}

static void test_queue_empty() {
    Queue queue = {0};
    queue_alloc(&queue, 8, 8, sizeof(QueueTestElement));

    TEST_TRUE(queue_is_empty(&queue));
    TEST_FALSE(queue_is_full(&queue));
    TEST_TRUE(queue_has_space(&queue));
    TEST_TRUE(queue_capacity(&queue) >= 9);

    QueueTestElement element = {0};
    TEST_FALSE(queue_dequeue(&queue, (byte *) &element));

    queue_free(&queue);
}

static void test_queue_enqueue_dequeue() {
    Queue queue = {0};
    queue_alloc(&queue, 8, 8, sizeof(QueueTestElement));

    QueueTestElement in = {1, 100};
    TEST_EQUALS(queue_enqueue(&queue, (const byte *) &in), queue.memory);
    TEST_FALSE(queue_is_empty(&queue));

    QueueTestElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, (byte *) &out));
    TEST_EQUALS(out.id, 1);
    TEST_EQUALS(out.value, 100);
    TEST_TRUE(queue_is_empty(&queue));

    // Empty again
    TEST_FALSE(queue_dequeue(&queue, (byte *) &out));

    queue_free(&queue);
}

static void test_queue_fifo_order() {
    Queue queue = {0};
    queue_alloc(&queue, 8, 8, sizeof(QueueTestElement));

    for (int32 i = 0; i < 5; ++i) {
        QueueTestElement in = {i, i * 10};
        queue_enqueue(&queue, (const byte *) &in);
    }

    for (int32 i = 0; i < 5; ++i) {
        QueueTestElement out = {0};
        TEST_TRUE(queue_dequeue(&queue, (byte *) &out));
        TEST_EQUALS(out.id, i);
        TEST_EQUALS(out.value, i * 10);
    }

    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_queue_full() {
    Queue queue = {0};
    queue_alloc(&queue, 8, 8, sizeof(QueueTestElement));

    // One slot always stays empty
    const uint64 max_elements = queue_capacity(&queue) - 1;

    for (uint64 i = 0; i < max_elements; ++i) {
        QueueTestElement in = {(int32) i, (int32) i};
        TEST_NOT_EQUALS(queue_enqueue_safe(&queue, (const byte *) &in), NULL);
    }

    TEST_TRUE(queue_is_full(&queue));
    TEST_FALSE(queue_has_space(&queue));

    // Doesn't overwrite existing elements
    QueueTestElement in = {999, 999};
    TEST_EQUALS(queue_enqueue_safe(&queue, (const byte *) &in), NULL);

    QueueTestElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, (byte *) &out));
    TEST_EQUALS(out.id, 0);
    TEST_TRUE(queue_has_space(&queue));

    queue_free(&queue);
}

// The head/tail pointers have to wrap around at the end of the buffer
static void test_queue_wrapping() {
    Queue queue = {0};
    queue_alloc(&queue, 8, 8, sizeof(QueueTestElement));

    const uint64 capacity = queue_capacity(&queue);

    // Move the head/tail close to the end of the buffer
    for (uint64 i = 0; i < capacity - 2; ++i) {
        QueueTestElement in = {(int32) i, (int32) i};
        queue_enqueue(&queue, (const byte *) &in);
    }

    for (uint64 i = 0; i < capacity - 2; ++i) {
        QueueTestElement out = {0};
        TEST_TRUE(queue_dequeue(&queue, (byte *) &out));
        TEST_EQUALS(out.id, (int32) i);
    }

    // Enqueue again -> the data has to wrap around
    for (int32 i = 0; i < 4; ++i) {
        QueueTestElement in = {100 + i, 0};
        TEST_NOT_EQUALS(queue_enqueue_safe(&queue, (const byte *) &in), NULL);
    }

    for (int32 i = 0; i < 4; ++i) {
        QueueTestElement out = {0};
        TEST_TRUE(queue_dequeue(&queue, (byte *) &out));
        TEST_EQUALS(out.id, 100 + i);
    }

    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_queue_enqueue_unique() {
    Queue queue = {0};
    queue_alloc(&queue, 8, 8, sizeof(QueueTestElement));

    QueueTestElement in = {1, 1};
    queue_enqueue_unique(&queue, (const byte *) &in);
    queue_enqueue_unique(&queue, (const byte *) &in);

    TEST_TRUE(queue_dequeue_keep(&queue) != NULL);

    QueueTestElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, (byte *) &out));
    TEST_EQUALS(out.id, 1);

    // The duplicate wasn't added
    TEST_FALSE(queue_dequeue(&queue, (byte *) &out));

    queue_free(&queue);
}

static void test_queue_dequeue_keep() {
    Queue queue = {0};
    queue_alloc(&queue, 8, 8, sizeof(QueueTestElement));

    TEST_EQUALS(queue_dequeue_keep(&queue), NULL);

    QueueTestElement in = {7, 70};
    queue_enqueue(&queue, (const byte *) &in);

    // The element is returned but not removed
    const byte* keep = queue_dequeue_keep(&queue);
    TEST_NOT_EQUALS(keep, NULL);
    TEST_EQUALS(((const QueueTestElement *) keep)->id, 7);
    TEST_FALSE(queue_is_empty(&queue));

    queue_dequeue_end(&queue);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_queue_set_empty() {
    Queue queue = {0};
    queue_alloc(&queue, 8, 8, sizeof(QueueTestElement));

    QueueTestElement in = {1, 1};
    queue_enqueue(&queue, (const byte *) &in);
    TEST_FALSE(queue_is_empty(&queue));

    queue_set_empty(&queue);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_queue_init() {
    byte buffer[1024];
    Queue queue = {0};
    queue_init(&queue, buffer, 8, sizeof(QueueTestElement));

    // The caller provides memory for exactly element_count slots, one stays empty
    TEST_EQUALS(queue_capacity(&queue), 8);
    TEST_TRUE(queue_is_empty(&queue));

    for (int32 i = 0; i < 7; ++i) {
        QueueTestElement in = {i, i};
        TEST_NOT_EQUALS(queue_enqueue_safe(&queue, (const byte *) &in), NULL);
    }

    QueueTestElement in = {7, 7};
    TEST_EQUALS(queue_enqueue_safe(&queue, (const byte *) &in), NULL);

    // Never frees memory that it doesn't own
    queue.size = 0;
    queue.memory = NULL;
}

static void test_queue_typed() {
    QueueT<QueueTestElement> queue = {0};
    queue_alloc(&queue, 8, 8, sizeof(size_t));

    TEST_TRUE(queue_is_empty(&queue));
    TEST_FALSE(queue_is_full(&queue));

    for (int32 i = 0; i < 5; ++i) {
        QueueTestElement in = {i, i * 2};
        queue_enqueue(&queue, &in);
    }

    for (int32 i = 0; i < 5; ++i) {
        QueueTestElement out = {0};
        TEST_TRUE(queue_dequeue(&queue, &out));
        TEST_EQUALS(out.id, i);
        TEST_EQUALS(out.value, i * 2);
    }

    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

#if PERFORMANCE_TEST
static void _my_queue(MAYBE_UNUSED volatile void* val) {
    Queue queue = {0};
    queue_alloc(&queue, 999, 999, sizeof(size_t));

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
    queue_alloc(&queue, 999, 999, 8);

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
    TEST_RUN(test_queue_empty);
    TEST_RUN(test_queue_enqueue_dequeue);
    TEST_RUN(test_queue_fifo_order);
    TEST_RUN(test_queue_full);
    TEST_RUN(test_queue_wrapping);
    TEST_RUN(test_queue_enqueue_unique);
    TEST_RUN(test_queue_dequeue_keep);
    TEST_RUN(test_queue_set_empty);
    TEST_RUN(test_queue_init);
    TEST_RUN(test_queue_typed);

    #if PERFORMANCE_TEST
        TEST_RUN(test_queue_performance);
    #endif

    TEST_FINALIZE();

    return 0;
}
