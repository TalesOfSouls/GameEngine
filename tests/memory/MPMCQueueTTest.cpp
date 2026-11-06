#include "../TestFramework.h"
#include "../../memory/MPMCQueueT.cpp"

/**
 * Tests for memory/MPMCQueueT.cpp/.h
 *
 * Bounded multi producer / multi consumer queue (Vyukov style). Every slot carries a
 * sequence number, a producer may claim a slot when sequence == position and a consumer
 * may claim it when sequence == position + 1. Unlike the SPSC/MPSC queues the ring does
 * not sacrifice a slot, so a queue with capacity N holds N elements.
 * The capacity has to be a power of two.
 * All tests are single threaded, they verify the slot lifecycle (claim -> publish -> consume).
 */

struct MPMCQueueElement {
    int32 id;
    int32 value;
};

static void test_mpmc_queue_alloc() {
    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_alloc(&queue, 8, 8);

    TEST_NOT_EQUALS(queue.memory, NULL);
    TEST_NOT_EQUALS(queue.sequence, NULL);
    TEST_EQUALS(queue.capacity, 8);
    TEST_EQUALS(queue.mask, 7);
    TEST_EQUALS(queue.head.load(memory_order_relaxed), 0);
    TEST_EQUALS(queue.tail.load(memory_order_relaxed), 0);

    // Every slot starts empty: sequence[i] == i
    for (int32 i = 0; i < 8; ++i) {
        TEST_EQUALS(queue.sequence[i].load(memory_order_relaxed), i);
    }

    TEST_TRUE(queue_is_empty(&queue));
    TEST_FALSE(queue_is_full(&queue));

    queue_free(&queue);
    TEST_EQUALS(queue.memory, NULL);
}

static void test_mpmc_queue_init_buffer() {
    // The sequence array follows the element memory
    alignas(64) byte raw[512] = {0};
    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_init(&queue, raw, 8);

    TEST_EQUALS(queue.memory, (MPMCQueueElement *) raw);
    TEST_EQUALS(queue.capacity, 8);
    TEST_TRUE(queue.sequence >= (atomic<size_t> *) (queue.memory + queue.capacity));
    TEST_TRUE(queue_is_empty(&queue));

    const MPMCQueueElement element = {1, 10};
    TEST_TRUE(queue_enqueue(&queue, element));
    TEST_FALSE(queue_is_empty(&queue));

    // BufferMemory variant carves the memory out of the buffer
    BufferMemory buffer = {0};
    buffer_alloc(&buffer, 2048, 2048);

    MPMCQueueT<MPMCQueueElement> second = {0};
    queue_init(&second, &buffer, 8);

    TEST_NOT_EQUALS(second.memory, NULL);
    TEST_EQUALS(second.capacity, 8);
    TEST_TRUE(queue_is_empty(&second));
    TEST_NOT_EQUALS(second.memory, queue.memory);

    buffer_free(&buffer);
}

static void test_mpmc_queue_enqueue_dequeue() {
    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // Empty queue
    MPMCQueueElement out = {0};
    TEST_FALSE(queue_dequeue(&queue, &out));
    TEST_EQUALS(queue_dequeue_start(&queue), NULL);

    const MPMCQueueElement a = {1, 10};
    const MPMCQueueElement b = {2, 20};
    const MPMCQueueElement c = {3, 30};

    TEST_TRUE(queue_enqueue(&queue, &a));
    TEST_TRUE(queue_enqueue(&queue, b));
    TEST_TRUE(queue_enqueue(&queue, c));

    // The sequence numbers move one position ahead per enqueue
    TEST_EQUALS(queue.sequence[0].load(memory_order_relaxed), 1);
    TEST_EQUALS(queue.sequence[2].load(memory_order_relaxed), 3);
    TEST_EQUALS(queue.head.load(memory_order_relaxed), 3);

    // FIFO order
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 1);
    TEST_EQUALS(out.value, 10);

    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 2);

    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 3);

    TEST_FALSE(queue_dequeue(&queue, &out));
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_mpmc_queue_full() {
    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // The whole ring is usable, no slot is sacrificed
    for (int32 i = 0; i < 4; ++i) {
        const MPMCQueueElement element = {i, i * 10};
        TEST_TRUE(queue_enqueue(&queue, element));
    }

    TEST_TRUE(queue_is_full(&queue));
    TEST_FALSE(queue_is_empty(&queue));

    const MPMCQueueElement rejected = {99, 990};
    TEST_FALSE(queue_enqueue(&queue, rejected));
    TEST_EQUALS(queue_enqueue_start(&queue), NULL);

    // Emptying the ring makes it writable again
    MPMCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 0);
    TEST_FALSE(queue_is_full(&queue));

    const MPMCQueueElement accepted = {4, 40};
    TEST_TRUE(queue_enqueue(&queue, accepted));

    queue_free(&queue);
}

static void test_mpmc_queue_wrap() {
    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    for (int32 i = 0; i < 10; ++i) {
        const MPMCQueueElement element = {i, i * 10};
        TEST_TRUE(queue_enqueue(&queue, element));

        MPMCQueueElement out = {0};
        TEST_TRUE(queue_dequeue(&queue, &out));
        TEST_EQUALS(out.id, i);
        TEST_EQUALS(out.value, i * 10);
    }

    TEST_TRUE(queue_is_empty(&queue));

    // Wrap around while the queue is partially filled
    const MPMCQueueElement a = {1, 10};
    const MPMCQueueElement b = {2, 20};
    const MPMCQueueElement c = {3, 30};

    TEST_TRUE(queue_enqueue(&queue, a));
    TEST_TRUE(queue_enqueue(&queue, b));
    TEST_TRUE(queue_enqueue(&queue, c));

    MPMCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 1);

    const MPMCQueueElement d = {4, 40};
    TEST_TRUE(queue_enqueue(&queue, d));

    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 2);
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 3);

    queue_free(&queue);
}

static void test_mpmc_queue_start_end() {
    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // Producer side: claim a slot, write in place, publish
    MPMCQueueElement* slot = queue_enqueue_start(&queue);
    TEST_EQUALS(slot, queue.memory);

    // While claimed, the slot is not visible to a consumer
    TEST_EQUALS(queue_enqueue_position(&queue, slot), 0);
    slot->id = 7;
    slot->value = 70;
    TEST_EQUALS(queue_dequeue_start(&queue), NULL);

    queue_enqueue_end(&queue, slot);
    TEST_EQUALS(queue.sequence[0].load(memory_order_relaxed), 1);

    MPMCQueueElement* consumer_slot = queue_dequeue_start(&queue);
    TEST_EQUALS(consumer_slot, slot);
    TEST_EQUALS(consumer_slot->id, 7);
    TEST_EQUALS(consumer_slot->value, 70);

    queue_dequeue_end(&queue, consumer_slot);

    // The slot is empty for the next lap now
    TEST_EQUALS(queue.sequence[0].load(memory_order_relaxed), (size_t) queue.capacity);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_mpmc_queue_reuse_slot() {
    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_alloc(&queue, 2, 2);

    // Fill, drain and fill again: the same slots are reused with a higher sequence
    for (int32 lap = 0; lap < 3; ++lap) {
        const MPMCQueueElement a = {lap * 2, lap * 20};
        const MPMCQueueElement b = {lap * 2 + 1, lap * 20 + 10};

        TEST_TRUE(queue_enqueue(&queue, a));
        TEST_TRUE(queue_enqueue(&queue, b));
        TEST_TRUE(queue_is_full(&queue));

        MPMCQueueElement out = {0};
        TEST_TRUE(queue_dequeue(&queue, &out));
        TEST_EQUALS(out.id, lap * 2);
        TEST_TRUE(queue_dequeue(&queue, &out));
        TEST_EQUALS(out.id, lap * 2 + 1);
        TEST_TRUE(queue_is_empty(&queue));
    }

    TEST_EQUALS(queue.head.load(memory_order_relaxed), 6);
    TEST_EQUALS(queue.tail.load(memory_order_relaxed), 6);
    TEST_EQUALS(queue.sequence[0].load(memory_order_relaxed), (size_t) (0 + 3 * 2));
    TEST_EQUALS(queue.sequence[1].load(memory_order_relaxed), (size_t) (1 + 3 * 2));

    queue_free(&queue);
}

static void test_mpmc_queue_round_trip() {
    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_alloc(&queue, 8, 8);

    int32 produced = 0;
    int32 consumed = 0;

    while (consumed < 50) {
        while (produced < 50 && !queue_is_full(&queue)) {
            const MPMCQueueElement element = {produced, produced * 3};
            TEST_TRUE(queue_enqueue(&queue, element));
            ++produced;
        }

        MPMCQueueElement out = {0};
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

static void test_mpmc_queue_wait() {
    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // The waiting variants only block while the queue is full/empty
    const MPMCQueueElement a = {1, 10};
    queue_enqueue_wait(&queue, a);
    MPMCQueueElement out = {0};
    queue_dequeue_wait(&queue, &out);
    TEST_EQUALS(out.id, 1);
    TEST_TRUE(queue_is_empty(&queue));

    MPMCQueueElement* slot = queue_enqueue_start_wait(&queue);

    // The position counter keeps increasing, so the next free slot is slot 1
    TEST_EQUALS(slot, queue.memory + 1);
    slot->id = 2;
    slot->value = 20;
    queue_enqueue_end(&queue, slot);

    MPMCQueueElement* consumer_slot = queue_dequeue_start_wait(&queue);
    TEST_EQUALS(consumer_slot->id, 2);
    queue_dequeue_end(&queue, consumer_slot);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_mpmc_queue_arena() {
    MemoryArena* root = mem_arena_alloc(8192, 8192);

    MPMCQueueT<MPMCQueueElement> queue = {0};
    queue_alloc(&queue, root, 8, 8);

    TEST_NOT_EQUALS(queue.memory, NULL);
    TEST_EQUALS(queue.capacity, 8);
    TEST_TRUE(queue_is_empty(&queue));
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    const MPMCQueueElement element = {1, 10};
    TEST_TRUE(queue_enqueue(&queue, element));

    MPMCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 1);

    queue_free(&queue, root);
    TEST_EQUALS(mem_arena_stats(root).count, 1);

    void* base = root->base;
    platform_aligned_free(&base);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryMPMCQueueTTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_mpmc_queue_alloc);
    TEST_RUN(test_mpmc_queue_init_buffer);
    TEST_RUN(test_mpmc_queue_enqueue_dequeue);
    TEST_RUN(test_mpmc_queue_full);
    TEST_RUN(test_mpmc_queue_wrap);
    TEST_RUN(test_mpmc_queue_start_end);
    TEST_RUN(test_mpmc_queue_reuse_slot);
    TEST_RUN(test_mpmc_queue_round_trip);
    TEST_RUN(test_mpmc_queue_wait);
    TEST_RUN(test_mpmc_queue_arena);

    TEST_FINALIZE();

    return 0;
}
