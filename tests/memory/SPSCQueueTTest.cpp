#include "../TestFramework.h"
#include "../../memory/SPSCQueueT.cpp"

/**
 * Tests for memory/SPSCQueueT.cpp/.h
 *
 * The queue is a single producer/single consumer ring buffer with a cache line padded
 * head/tail pair. The ring keeps one slot empty to distinguish empty from full, so a
 * queue with capacity N holds at most N - 1 elements.
 * All tests are single threaded, they only verify the data structure semantics.
 */
struct SPSCQueueElement {
    int32 id;
    int32 value;
};

static void test_spsc_queue_alloc() {
    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 8);

    TEST_NOT_EQUALS(queue.memory, NULL);
    TEST_EQUALS(queue.capacity, 4);
    TEST_EQUALS(queue.head.load(memory_order_relaxed), queue.memory);
    TEST_EQUALS(queue.tail.load(memory_order_relaxed), queue.memory);
    TEST_EQUALS(queue.head_cache, queue.memory);
    TEST_EQUALS(queue.tail_cache, queue.memory);

    TEST_TRUE(queue_is_empty(&queue));
    TEST_TRUE(queue_has_space(&queue));
    TEST_FALSE(queue_is_full(&queue));

    queue_free(&queue);
    TEST_EQUALS(queue.memory, NULL);
}

static void test_spsc_queue_init_buffer() {
    // Byte buffer variant
    alignas(64) byte raw[256] = {0};
    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_init(&queue, raw, 4);

    TEST_EQUALS(queue.memory, (SPSCQueueElement *) raw);
    TEST_EQUALS(queue.capacity, 4);
    TEST_TRUE(queue_is_empty(&queue));

    const SPSCQueueElement element = {1, 10};
    TEST_NOT_EQUALS(queue_enqueue(&queue, element), NULL);
    TEST_FALSE(queue_is_empty(&queue));

    // BufferMemory variant carves the memory out of the buffer
    BufferMemory buffer = {0};
    buffer_alloc(&buffer, 512, 512);

    SPSCQueueT<SPSCQueueElement> second = {0};
    queue_init(&second, &buffer, 4);

    TEST_NOT_EQUALS(second.memory, NULL);
    TEST_TRUE(second.memory >= (SPSCQueueElement *) buffer.memory);
    TEST_EQUALS(second.capacity, 4);
    TEST_TRUE(queue_is_empty(&second));

    // The queues don't share memory
    TEST_NOT_EQUALS(second.memory, queue.memory);

    buffer_free(&buffer);
}

static void test_spsc_queue_enqueue_dequeue() {
    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // Empty queue can't be dequeued
    SPSCQueueElement out = {0};
    TEST_FALSE(queue_dequeue(&queue, &out));

    const SPSCQueueElement a = {1, 10};
    const SPSCQueueElement b = {2, 20};
    const SPSCQueueElement c = {3, 30};

    TEST_EQUALS(queue_enqueue(&queue, a), queue.memory);
    TEST_EQUALS(queue_enqueue(&queue, &b), queue.memory + 1);
    TEST_EQUALS(queue_enqueue(&queue, c), queue.memory + 2);

    TEST_FALSE(queue_is_empty(&queue));
    TEST_EQUALS(queue.head.load(memory_order_relaxed), queue.memory + 3);

    // FIFO order
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 1);
    TEST_EQUALS(out.value, 10);

    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 2);
    TEST_EQUALS(out.value, 20);

    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 3);
    TEST_EQUALS(out.value, 30);

    TEST_FALSE(queue_dequeue(&queue, &out));
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_spsc_queue_full() {
    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // The ring keeps one slot free -> 3 of 4 slots are usable
    for (int32 i = 0; i < 3; ++i) {
        const SPSCQueueElement element = {i, i * 10};
        TEST_NOT_EQUALS(queue_enqueue(&queue, element), NULL);
    }

    TEST_TRUE(queue_is_full(&queue));
    TEST_FALSE(queue_has_space(&queue));
    TEST_FALSE(queue_is_empty(&queue));

    const SPSCQueueElement rejected = {99, 990};
    TEST_EQUALS(queue_enqueue(&queue, rejected), NULL);
    TEST_EQUALS(queue.head.load(memory_order_relaxed), queue.memory + 3);

    // Dequeueing reopens a slot
    SPSCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 0);
    TEST_TRUE(queue_has_space(&queue));
    TEST_FALSE(queue_is_full(&queue));

    const SPSCQueueElement accepted = {3, 30};
    TEST_NOT_EQUALS(queue_enqueue(&queue, accepted), NULL);

    queue_free(&queue);
}

static void test_spsc_queue_wrap() {
    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // The head/tail pointers are guaranteed to wrap around multiple times
    for (int32 i = 0; i < 10; ++i) {
        const SPSCQueueElement element = {i, i * 10};
        TEST_NOT_EQUALS(queue_enqueue(&queue, element), NULL);

        SPSCQueueElement out = {0};
        TEST_TRUE(queue_dequeue(&queue, &out));
        TEST_EQUALS(out.id, i);
        TEST_EQUALS(out.value, i * 10);
    }

    TEST_TRUE(queue_is_empty(&queue));

    // Wrapping is reflected in the pointers, they always stay inside the ring
    TEST_TRUE(queue.head.load(memory_order_relaxed) >= queue.memory);
    TEST_TRUE(queue.head.load(memory_order_relaxed) < queue.memory + queue.capacity);

    queue_free(&queue);
}

static void test_spsc_queue_start_end() {
    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // Producer side: reserve, write in place, publish
    SPSCQueueElement* slot = queue_enqueue_start(&queue);
    TEST_EQUALS(slot, queue.memory);
    slot->id = 7;
    slot->value = 70;
    queue_enqueue_end(&queue);

    TEST_EQUALS(queue.head.load(memory_order_relaxed), queue.memory + 1);
    TEST_FALSE(queue_is_empty(&queue));

    // The element is not visible before queue_enqueue_end()
    SPSCQueueElement* tail = queue_dequeue_start(&queue);
    TEST_EQUALS(tail, queue.memory);
    TEST_EQUALS(tail->id, 7);
    TEST_EQUALS(tail->value, 70);
    queue_dequeue_end(&queue);

    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_spsc_queue_dequeue_keep() {
    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    const SPSCQueueElement a = {1, 10};
    const SPSCQueueElement b = {2, 20};
    queue_enqueue(&queue, a);
    queue_enqueue(&queue, b);

    // keep doesn't remove the element
    SPSCQueueElement* kept = queue_dequeue_keep(&queue);
    TEST_EQUALS(kept, queue.memory);
    TEST_EQUALS(kept->id, 1);
    TEST_EQUALS(queue.tail.load(memory_order_relaxed), queue.memory);

    // A second keep returns the same element
    TEST_EQUALS(queue_dequeue_keep(&queue), kept);

    SPSCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 1);

    TEST_EQUALS(queue_dequeue_keep(&queue), queue.memory + 1);
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 2);

    TEST_EQUALS(queue_dequeue_keep(&queue), NULL);

    queue_free(&queue);
}

static void test_spsc_queue_capacity_one() {
    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_alloc(&queue, 1, 1);

    // A ring with one slot can't distinguish empty from full -> no element fits
    TEST_TRUE(queue_is_full(&queue));
    TEST_TRUE(queue_is_empty(&queue));

    const SPSCQueueElement element = {1, 10};
    TEST_EQUALS(queue_enqueue(&queue, element), NULL);

    SPSCQueueElement out = {0};
    TEST_FALSE(queue_dequeue(&queue, &out));

    queue_free(&queue);
}

static void test_spsc_queue_round_trip() {
    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_alloc(&queue, 8, 8);

    // Producer and consumer interleaved, verifies the fifo invariant over many wraps
    int32 produced = 0;
    int32 consumed = 0;

    while (consumed < 50) {
        while (produced < 50 && queue_has_space(&queue)) {
            const SPSCQueueElement element = {produced, produced * 3};
            TEST_NOT_EQUALS(queue_enqueue(&queue, element), NULL);
            ++produced;
        }

        SPSCQueueElement out = {0};
        if (queue_dequeue(&queue, &out)) {
            TEST_EQUALS(out.id, consumed);
            TEST_EQUALS(out.value, consumed * 3);
            ++consumed;
        }
    }

    TEST_EQUALS(produced, 50);
    TEST_EQUALS(consumed, 50);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_spsc_queue_arena() {
    MemoryArena* root = mem_arena_alloc(8192, 8192);

    SPSCQueueT<SPSCQueueElement> queue = {0};
    queue_alloc(&queue, root, 4, 4);

    TEST_NOT_EQUALS(queue.memory, NULL);
    TEST_EQUALS(queue.capacity, 4);
    TEST_TRUE(queue_is_empty(&queue));

    // The queue memory is the user memory of a new arena in the chain
    bool inside_chain = false;
    for (MemoryArena* mem = root; mem; mem = mem->next) {
        inside_chain = inside_chain || (byte *) queue.memory == mem->memory;
    }
    TEST_TRUE(inside_chain);
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    // The arena backed queue behaves like the platform backed one
    const SPSCQueueElement a = {1, 10};
    const SPSCQueueElement b = {2, 20};
    TEST_NOT_EQUALS(queue_enqueue(&queue, a), NULL);
    TEST_NOT_EQUALS(queue_enqueue(&queue, b), NULL);

    SPSCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 1);
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 2);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue, root);

    // The arena node is released with it
    TEST_EQUALS(mem_arena_stats(root).count, 1);

    // The root is still usable
    root->memory[0] = 9;
    TEST_EQUALS(root->memory[0], 9);

    void* base = root->base;
    platform_aligned_free(&base);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemorySPSCQueueTTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_spsc_queue_alloc);
    TEST_RUN(test_spsc_queue_init_buffer);
    TEST_RUN(test_spsc_queue_enqueue_dequeue);
    TEST_RUN(test_spsc_queue_full);
    TEST_RUN(test_spsc_queue_wrap);
    TEST_RUN(test_spsc_queue_start_end);
    TEST_RUN(test_spsc_queue_dequeue_keep);
    TEST_RUN(test_spsc_queue_capacity_one);
    TEST_RUN(test_spsc_queue_round_trip);
    TEST_RUN(test_spsc_queue_arena);

    TEST_FINALIZE();

    return 0;
}
