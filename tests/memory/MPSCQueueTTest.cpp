#include "../TestFramework.h"
#include "../../memory/MPSCQueueT.cpp"

/**
 * Tests for memory/MPSCQueueT.cpp/.h
 *
 * Multi producer / single consumer ring buffer. Producers claim a slot with a CAS on
 * head and publish it through the per slot `slot_ready` flag, the consumer only advances
 * tail after it consumed a published slot. The ring keeps one slot empty, so a queue with
 * capacity N holds at most N - 1 elements.
 * All tests are single threaded, they verify the slot lifecycle (claim -> publish -> consume).
 */

struct MPSCQueueElement {
    int32 id;
    int32 value;
};

static void test_mpsc_queue_alloc() {
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 8);

    TEST_NOT_EQUALS(queue.memory, NULL);
    TEST_NOT_EQUALS(queue.slot_ready, NULL);
    TEST_EQUALS(queue.capacity, 4);
    TEST_EQUALS(queue.head.load(memory_order_relaxed), queue.memory);
    TEST_EQUALS(queue.tail.load(memory_order_relaxed), queue.memory);
    TEST_EQUALS(queue.head_cache, queue.memory);

    // No slot is published yet
    for (int32 i = 0; i < 4; ++i) {
        TEST_EQUALS(queue.slot_ready[i].load(memory_order_relaxed), 0);
    }

    TEST_TRUE(queue_is_empty(&queue));
    TEST_TRUE(queue_has_space(&queue));
    TEST_FALSE(queue_is_full(&queue));

    queue_free(&queue);
    TEST_EQUALS(queue.memory, NULL);
}

static void test_mpsc_queue_init_buffer() {
    // The slot ready flags live in the same buffer as the elements
    alignas(64) byte raw[512] = {0};
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_init(&queue, raw, 4);

    TEST_EQUALS(queue.memory, (MPSCQueueElement *) raw);
    TEST_EQUALS(queue.capacity, 4);
    TEST_TRUE(queue.slot_ready >= (atomic<byte> *) (queue.memory + queue.capacity));
    TEST_TRUE(queue_is_empty(&queue));

    const MPSCQueueElement element = {1, 10};
    TEST_NOT_EQUALS(queue_enqueue(&queue, element), NULL);
    TEST_FALSE(queue_is_empty(&queue));

    // BufferMemory variant carves the memory out of the buffer
    BufferMemory buffer = {0};
    buffer_alloc(&buffer, 1024, 1024);

    MPSCQueueT<MPSCQueueElement> second = {0};
    queue_init(&second, &buffer, 4);

    TEST_NOT_EQUALS(second.memory, NULL);
    TEST_EQUALS(second.capacity, 4);
    TEST_TRUE(queue_is_empty(&second));
    TEST_NOT_EQUALS(second.memory, queue.memory);

    buffer_free(&buffer);
}

static void test_mpsc_queue_enqueue_dequeue() {
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // Empty queue
    MPSCQueueElement out = {0};
    TEST_FALSE(queue_dequeue(&queue, &out));
    TEST_EQUALS(queue_dequeue_keep(&queue), NULL);

    const MPSCQueueElement a = {1, 10};
    const MPSCQueueElement b = {2, 20};
    const MPSCQueueElement c = {3, 30};

    TEST_EQUALS(queue_enqueue(&queue, &a), queue.memory);
    TEST_EQUALS(queue_enqueue(&queue, b), queue.memory + 1);
    TEST_EQUALS(queue_enqueue(&queue, c), queue.memory + 2);

    // Every enqueued slot is published
    TEST_EQUALS(queue.slot_ready[0].load(memory_order_acquire), 1);
    TEST_EQUALS(queue.slot_ready[2].load(memory_order_acquire), 1);

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

static void test_mpsc_queue_full() {
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // One slot stays empty -> 3 of 4 slots are usable
    for (int32 i = 0; i < 3; ++i) {
        const MPSCQueueElement element = {i, i * 10};
        TEST_NOT_EQUALS(queue_enqueue(&queue, element), NULL);
    }

    TEST_TRUE(queue_is_full(&queue));
    TEST_FALSE(queue_has_space(&queue));

    const MPSCQueueElement rejected = {99, 990};
    TEST_EQUALS(queue_enqueue(&queue, rejected), NULL);
    TEST_EQUALS(queue_reserve_slot(&queue), NULL);

    // Consuming a slot reopens it
    MPSCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 0);
    TEST_TRUE(queue_has_space(&queue));

    const MPSCQueueElement accepted = {3, 30};
    TEST_NOT_EQUALS(queue_enqueue(&queue, accepted), NULL);

    queue_free(&queue);
}

static void test_mpsc_queue_wrap() {
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    for (int32 i = 0; i < 10; ++i) {
        const MPSCQueueElement element = {i, i * 10};
        TEST_NOT_EQUALS(queue_enqueue(&queue, element), NULL);

        MPSCQueueElement out = {0};
        TEST_TRUE(queue_dequeue(&queue, &out));
        TEST_EQUALS(out.id, i);
        TEST_EQUALS(out.value, i * 10);
    }

    TEST_TRUE(queue_is_empty(&queue));
    TEST_TRUE(queue.head.load(memory_order_relaxed) >= queue.memory);
    TEST_TRUE(queue.head.load(memory_order_relaxed) < queue.memory + queue.capacity);

    queue_free(&queue);
}

static void test_mpsc_queue_publish() {
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // A claimed but unpublished slot is not visible to the consumer
    MPSCQueueElement* slot = queue_enqueue_start(&queue);
    TEST_EQUALS(slot, queue.memory);
    slot->id = 5;
    slot->value = 50;

    TEST_EQUALS(queue_dequeue_keep(&queue), NULL);
    TEST_FALSE(queue_is_empty(&queue));

    queue_enqueue_end(&queue, slot);
    TEST_EQUALS(queue.slot_ready[0].load(memory_order_acquire), 1);

    MPSCQueueElement* published = queue_dequeue_keep(&queue);
    TEST_EQUALS(published, slot);
    TEST_EQUALS(published->id, 5);
    TEST_EQUALS(published->value, 50);

    queue_dequeue_end(&queue);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_mpsc_queue_dequeue_keep() {
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    const MPSCQueueElement a = {1, 10};
    const MPSCQueueElement b = {2, 20};
    queue_enqueue(&queue, a);
    queue_enqueue(&queue, b);

    // keep doesn't consume the element
    MPSCQueueElement* kept = queue_dequeue_keep(&queue);
    TEST_EQUALS(kept, queue.memory);
    TEST_EQUALS(queue.tail.load(memory_order_relaxed), queue.memory);
    TEST_EQUALS(queue_dequeue_keep(&queue), kept);

    // The start variant returns the same slot as keep
    TEST_EQUALS(queue_dequeue_start(&queue), kept);

    MPSCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 1);

    TEST_EQUALS(queue_dequeue_keep(&queue), queue.memory + 1);
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 2);

    TEST_EQUALS(queue_dequeue_keep(&queue), NULL);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_mpsc_queue_wait() {
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, 4, 4);

    // The waiting variants only block while the queue is full/empty
    const MPSCQueueElement a = {7, 70};
    TEST_NOT_EQUALS(queue_enqueue_wait(&queue, a), NULL);

    MPSCQueueElement* slot = queue_reserve_slot_wait(&queue);
    TEST_NOT_EQUALS(slot, NULL);
    TEST_EQUALS(slot, queue.memory + 1);

    // The consumer has to publish... (the reserved slot belongs to the producer)
    slot->id = 8;
    slot->value = 80;
    queue_enqueue_end(&queue, slot);

    MPSCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 7);
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 8);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_mpsc_queue_round_trip() {
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, 8, 8);

    int32 produced = 0;
    int32 consumed = 0;

    while (consumed < 50) {
        while (produced < 50 && queue_has_space(&queue)) {
            const MPSCQueueElement element = {produced, produced * 3};
            TEST_NOT_EQUALS(queue_enqueue(&queue, element), NULL);
            ++produced;
        }

        MPSCQueueElement out = {0};
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

static void test_mpsc_queue_multi_producer_order() {
    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, 8, 8);

    // Two producers claim slots one after another, the consumer sees the claim order
    const MPSCQueueElement a = {1, 10};
    const MPSCQueueElement b = {2, 20};
    const MPSCQueueElement c = {3, 30};
    const MPSCQueueElement d = {4, 40};

    TEST_NOT_EQUALS(queue_enqueue(&queue, a), NULL);
    TEST_NOT_EQUALS(queue_enqueue(&queue, b), NULL);
    TEST_NOT_EQUALS(queue_enqueue(&queue, c), NULL);

    MPSCQueueElement out = {0};
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 1);

    // A second producer joins while the queue is partially drained
    TEST_NOT_EQUALS(queue_enqueue(&queue, d), NULL);

    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 2);
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 3);
    TEST_TRUE(queue_dequeue(&queue, &out));
    TEST_EQUALS(out.id, 4);
    TEST_TRUE(queue_is_empty(&queue));

    queue_free(&queue);
}

static void test_mpsc_queue_arena() {
    MemoryArena* root = mem_arena_alloc(8192, 8192);

    MPSCQueueT<MPSCQueueElement> queue = {0};
    queue_alloc(&queue, root, 4, 4);

    TEST_NOT_EQUALS(queue.memory, NULL);
    TEST_EQUALS(queue.capacity, 4);
    TEST_TRUE(queue_is_empty(&queue));
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    const MPSCQueueElement element = {1, 10};
    TEST_NOT_EQUALS(queue_enqueue(&queue, element), NULL);

    MPSCQueueElement out = {0};
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
    #define main MemoryMPSCQueueTTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_mpsc_queue_alloc);
    TEST_RUN(test_mpsc_queue_init_buffer);
    TEST_RUN(test_mpsc_queue_enqueue_dequeue);
    TEST_RUN(test_mpsc_queue_full);
    TEST_RUN(test_mpsc_queue_wrap);
    TEST_RUN(test_mpsc_queue_publish);
    TEST_RUN(test_mpsc_queue_dequeue_keep);
    TEST_RUN(test_mpsc_queue_wait);
    TEST_RUN(test_mpsc_queue_round_trip);
    TEST_RUN(test_mpsc_queue_multi_producer_order);
    TEST_RUN(test_mpsc_queue_arena);

    TEST_FINALIZE();

    return 0;
}
