#include "../TestFramework.h"
#include "../../memory/BufferMemory.cpp"
#include "../../stdlib/ArrayVector.h"

static void test_array_vector_alloc() {
    ArrayVector<int32> vec = {0};
    array_vector_alloc(&vec, 10, 20);

    TEST_EQUALS(vec.capacity, 10);
    TEST_EQUALS(vec.max_capacity, 20);
    TEST_EQUALS(vec.count, 0);
    TEST_NOT_EQUALS(vec.elements, NULL);

    array_vector_free(&vec);
    TEST_EQUALS(vec.capacity, 0);
    TEST_EQUALS(vec.max_capacity, 0);
    TEST_EQUALS(vec.elements, NULL);
}

static void test_array_vector_init() {
    // +64 for the alignment of the buffer memory start
    byte buffer[1024 + 64] = {0};
    BufferMemory mem = {0};
    buffer_init(&mem, buffer, 1024);

    ArrayVector<int32> vec = {0};
    array_vector_init(&vec, &mem, 10);

    TEST_EQUALS(vec.capacity, 10);
    TEST_EQUALS(vec.max_capacity, 10);
    TEST_EQUALS(vec.count, 0);
    TEST_EQUALS(vec.elements, (int32 *) mem.memory);
    TEST_EQUALS(mem.head, mem.memory + 10 * sizeof(int32));
}

static void test_array_vector_fixed() {
    FixedArrayVector<int32, 4> fixed;
    ArrayVector<int32>& vec = fixed.view;

    TEST_EQUALS(vec.capacity, 4);
    TEST_EQUALS(vec.max_capacity, 4);
    TEST_EQUALS(vec.count, 0);
    TEST_EQUALS(vec.elements, fixed.elements_storage);

    array_vector_insert(&vec, 1);
    array_vector_insert(&vec, 2);

    TEST_EQUALS(vec.count, 2);
    TEST_EQUALS(array_vector_get(&vec, 0), 1);
    TEST_EQUALS(array_vector_get(&vec, 1), 2);
}

static void test_array_vector_insert() {
    ArrayVector<int32> vec = {0};
    array_vector_alloc(&vec, 10, 10);

    array_vector_insert(&vec, 1);
    TEST_EQUALS(vec.count, 1);
    TEST_EQUALS(vec.elements[0], 1);

    const int32 value = 2;
    array_vector_insert(&vec, &value);
    TEST_EQUALS(vec.count, 2);
    TEST_EQUALS(vec.elements[1], 2);

    const int32 values[] = {3, 4, 5};
    array_vector_insert(&vec, values, 3);
    TEST_EQUALS(vec.count, 5);
    TEST_EQUALS(vec.elements[2], 3);
    TEST_EQUALS(vec.elements[3], 4);
    TEST_EQUALS(vec.elements[4], 5);

    array_vector_free(&vec);
}

static void test_array_vector_insert_safe() {
    ArrayVector<int32> vec = {0};
    array_vector_alloc(&vec, 2, 2);

    TEST_TRUE(array_vector_insert_safe(&vec, 1));
    TEST_TRUE(array_vector_insert_safe(&vec, 2));

    // Full
    TEST_FALSE(array_vector_insert_safe(&vec, 3));
    TEST_EQUALS(vec.count, 2);

    // A bulk insert only succeeds if all elements fit
    array_vector_reset(&vec);

    const int32 values[] = {1, 2, 3};
    TEST_FALSE(array_vector_insert_safe(&vec, values, 3));
    TEST_EQUALS(vec.count, 0);

    TEST_TRUE(array_vector_insert_safe(&vec, values, 2));
    TEST_EQUALS(vec.count, 2);

    array_vector_free(&vec);
}

static void test_array_vector_reserve() {
    ArrayVector<int32> vec = {0};
    array_vector_alloc(&vec, 10, 10);

    int32& a = array_vector_reserve_one(&vec);
    TEST_EQUALS(vec.count, 1);
    a = 10;

    int32* b = array_vector_reserve(&vec, 2);
    TEST_EQUALS(vec.count, 3);
    b[0] = 20;
    b[1] = 30;

    TEST_EQUALS(array_vector_get(&vec, 0), 10);
    TEST_EQUALS(array_vector_get(&vec, 1), 20);
    TEST_EQUALS(array_vector_get(&vec, 2), 30);

    array_vector_free(&vec);
}

static void test_array_vector_has_value() {
    ArrayVector<int32> vec = {0};
    array_vector_alloc(&vec, 10, 10);

    const int32 values[] = {1, 2, 3, 4};
    array_vector_insert(&vec, values, 4);

    TEST_TRUE(array_vector_has_value(&vec, 1));
    TEST_TRUE(array_vector_has_value(&vec, 4));
    TEST_FALSE(array_vector_has_value(&vec, 5));

    array_vector_free(&vec);
}

static void test_array_vector_remove_value() {
    ArrayVector<int32> vec = {0};
    array_vector_alloc(&vec, 10, 10);

    const int32 values[] = {1, 2, 3, 2, 5};
    array_vector_insert(&vec, values, 5);

    array_vector_remove_value(&vec, 2);

    TEST_EQUALS(vec.count, 3);
    TEST_EQUALS(vec.elements[0], 1);
    TEST_EQUALS(vec.elements[1], 3);
    TEST_EQUALS(vec.elements[2], 5);
    TEST_FALSE(array_vector_has_value(&vec, 2));

    array_vector_free(&vec);
}

static void test_array_vector_remove_index() {
    ArrayVector<int32> vec = {0};
    array_vector_alloc(&vec, 10, 10);

    const int32 values[] = {1, 2, 3, 4};
    array_vector_insert(&vec, values, 4);

    array_vector_remove_index(&vec, 1);
    TEST_EQUALS(vec.count, 3);
    TEST_EQUALS(vec.elements[0], 1);
    TEST_EQUALS(vec.elements[1], 3);
    TEST_EQUALS(vec.elements[2], 4);

    array_vector_remove_index(&vec, 2);
    TEST_EQUALS(vec.count, 2);
    TEST_EQUALS(vec.elements[0], 1);
    TEST_EQUALS(vec.elements[1], 3);

    array_vector_free(&vec);
}

static void test_array_vector_iterate() {
    ArrayVector<int32> vec = {0};
    array_vector_alloc(&vec, 10, 10);

    const int32 values[] = {1, 2, 3, 4, 5};
    array_vector_insert(&vec, values, 5);

    int32 sum = 0;
    bool all_even_index = true;
    int32 index = 0;
    int32* element = NULL;
    ArrayVector<int32>* vec_ptr = &vec;

    array_vector_iterate_ptr_start(vec_ptr, element) {
        sum += *element;
        all_even_index = all_even_index && (*element == values[index]);
        ++index;
    } array_vector_iterate_end

    TEST_EQUALS(sum, 15);
    TEST_TRUE(all_even_index);
    TEST_EQUALS(index, 5);

    array_vector_free(&vec);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibArrayVectorTest
#endif

int main() {
    TEST_INIT(25);

    TEST_RUN(test_array_vector_alloc);
    TEST_RUN(test_array_vector_init);
    TEST_RUN(test_array_vector_fixed);
    TEST_RUN(test_array_vector_insert);
    TEST_RUN(test_array_vector_insert_safe);
    TEST_RUN(test_array_vector_reserve);
    TEST_RUN(test_array_vector_has_value);
    TEST_RUN(test_array_vector_remove_value);
    TEST_RUN(test_array_vector_remove_index);
    TEST_RUN(test_array_vector_iterate);

    TEST_FINALIZE();

    return 0;
}
