#include "../TestFramework.h"
#include "../../memory/PointerMemory.h"

/**
 * Tests for memory/PointerMemory.h
 *
 * This module doesn't own memory, it only aligns a caller provided pointer and
 * advances it. All tests therefore use a stack buffer with a known alignment.
 */
struct PointerTestElement {
    int32 id;
    f32 value;
};

static void test_pointer_memory_get_alignment() {
    alignas(64) byte buffer[256] = {0};

    // Intentionally misaligned start
    byte* data = buffer + 1;

    byte* first = memory_get(&data, 8);

    TEST_EQUALS((uintptr_t) first % 8, 0);
    TEST_TRUE(first >= buffer + 1);
    TEST_TRUE(first < buffer + 1 + 8);

    // The pointer is advanced by the requested size and not by the aligned size
    TEST_EQUALS(data, first + 8);

    // A subsequent request starts where the previous one ended (already aligned)
    byte* second = memory_get(&data, 3);
    TEST_EQUALS((uintptr_t) second % 8, 0);
    TEST_EQUALS(second, first + 8);
    TEST_EQUALS(data, second + 3);
}

static void test_pointer_memory_get_sequential() {
    alignas(64) byte buffer[256] = {0};
    byte* data = buffer;

    byte* a = memory_get(&data, 16);
    TEST_EQUALS(a, buffer);
    TEST_EQUALS(data, buffer + 16);

    byte* b = memory_get(&data, 8);
    TEST_EQUALS(b, buffer + 16);
    TEST_EQUALS(data, buffer + 24);

    // The alignment request determines the returned address, the size only the advance
    byte* c = memory_get(&data, 1);
    TEST_EQUALS(c, buffer + 24);
    TEST_EQUALS(data, buffer + 25);

    // 25 is not 8 byte aligned, so the next request rounds up
    byte* d = memory_get(&data, 4);
    TEST_EQUALS(d, buffer + 32);
    TEST_EQUALS(data, buffer + 36);

    // The returned ranges never overlap
    TEST_TRUE(a + 16 <= b);
    TEST_TRUE(b + 8 <= c);
    TEST_TRUE(c + 1 <= d);
}

static void test_pointer_memory_get_temp() {
    alignas(64) byte buffer[128] = {0};
    byte* data = buffer + 3;

    byte* temp1 = memory_get_temp(&data, 16);
    TEST_EQUALS(temp1, buffer + 8);

    // Temporary memory doesn't advance the pointer
    TEST_EQUALS(data, buffer + 3);

    byte* temp2 = memory_get_temp(&data, 16);
    TEST_EQUALS(temp2, temp1);
    TEST_EQUALS(data, buffer + 3);

    // The next real request returns the same memory and commits it
    byte* real = memory_get(&data, 16);
    TEST_EQUALS(real, temp1);
    TEST_EQUALS(data, temp1 + 16);
}

static void test_pointer_memory_resize() {
    alignas(64) byte buffer[64] = {0};
    byte* data = buffer;

    // The pointer memory can't resize, it always reports failure
    TEST_FALSE(memory_resize(&data, 1024));
    TEST_EQUALS(data, buffer);

    TEST_FALSE(memory_resize(&data, 0));
    TEST_EQUALS(data, buffer);

    TEST_FALSE(memory_resize((byte **) NULL, 0));
}

static void test_pointer_memory_alignment_variants() {
    const int32 alignments[] = {1, 2, 4, 8, 16, 32, 64};
    alignas(64) byte buffer[256] = {0};

    for (uint32 i = 0; i < ARRAY_COUNT(alignments); ++i) {
        byte* data = buffer + 1;
        byte* out = memory_get(&data, 3, alignments[i]);

        TEST_EQUALS((uintptr_t) out % alignments[i], 0);
        TEST_TRUE(out >= buffer + 1);
        TEST_TRUE(out <= buffer + 1 + alignments[i] - 1);
        TEST_EQUALS(data, out + 3);
    }

    // Already aligned memory stays untouched
    byte* aligned = memory_get(buffer, 7, 64);
    TEST_EQUALS(aligned, buffer);
}

static void test_pointer_memory_const_overloads() {
    alignas(64) byte buffer[64] = {0};

    // The value based overloads have no state, size is only there for the API symmetry
    TEST_EQUALS(memory_get(buffer + 5, 100), buffer + 8);
    TEST_EQUALS(memory_get(buffer + 5, 0), buffer + 8);
    TEST_EQUALS(memory_get(buffer + 5, 100, 16), buffer + 16);
    TEST_EQUALS(memory_get_temp(buffer + 5, 100), buffer + 8);

    // An aligned pointer is returned as is
    TEST_EQUALS(memory_get(buffer, 3), buffer);
    TEST_EQUALS(memory_get_temp(buffer, 3), buffer);
}

static void test_pointer_memory_zero_size() {
    alignas(64) byte buffer[64] = {0};
    byte* data = buffer + 1;

    // A zero sized request only aligns the pointer
    byte* aligned = memory_get(&data, 0);
    TEST_EQUALS(aligned, buffer + 8);
    TEST_EQUALS(data, aligned);

    // Requesting zero bytes from an aligned pointer doesn't move it either
    TEST_EQUALS(memory_get(&data, 0), buffer + 8);
    TEST_EQUALS(data, buffer + 8);
}

static void test_pointer_memory_round_trip() {
    alignas(64) byte buffer[256] = {0};
    byte* data = buffer;

    const PointerTestElement reference = {77, 2.5f};

    PointerTestElement* first = (PointerTestElement *) memory_get(
        &data,
        sizeof(PointerTestElement),
        alignof(PointerTestElement)
    );
    memcpy(first, &reference, sizeof(PointerTestElement));

    TEST_MEMORY_EQUALS(first, &reference, sizeof(PointerTestElement));
    TEST_EQUALS(first->id, reference.id);
    TEST_EQUALS_WITH_DELTA(first->value, reference.value, 0.0001);

    PointerTestElement* second = (PointerTestElement *) memory_get(
        &data,
        sizeof(PointerTestElement),
        alignof(PointerTestElement)
    );

    // The elements are distinct and don't overlap
    TEST_NOT_EQUALS(first, second);
    TEST_TRUE((byte *) second >= (byte *) first + sizeof(PointerTestElement));

    const PointerTestElement other = {5, 7.0f};
    memcpy(second, &other, sizeof(PointerTestElement));

    TEST_MEMORY_EQUALS(second, &other, sizeof(PointerTestElement));
    TEST_MEMORY_EQUALS(first, &reference, sizeof(PointerTestElement));
    TEST_EQUALS(first->id, 77);
    TEST_EQUALS(second->id, 5);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryPointerMemoryTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_pointer_memory_get_alignment);
    TEST_RUN(test_pointer_memory_get_sequential);
    TEST_RUN(test_pointer_memory_get_temp);
    TEST_RUN(test_pointer_memory_resize);
    TEST_RUN(test_pointer_memory_alignment_variants);
    TEST_RUN(test_pointer_memory_const_overloads);
    TEST_RUN(test_pointer_memory_zero_size);
    TEST_RUN(test_pointer_memory_round_trip);

    TEST_FINALIZE();

    return 0;
}
