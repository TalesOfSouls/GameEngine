#include "../TestFramework.h"
#include "../../utils/RandomUtils.h"

/**
 * Tests for RandomUtils.h
 *
 * The generators are seeded from the clock by rand_setup(), so the tests
 * either set the thread local state explicitly or only verify properties
 * (range, determinism after reseeding, permutation, ...).
 */

static void test_random_utils_state_determinism()
{
    _rng_state_32 = 123456789u;
    const uint32 first = rand_fast_32();
    const uint32 second = rand_fast_32();

    _rng_state_32 = 123456789u;
    TEST_EQUALS(rand_fast_32(), first);
    TEST_EQUALS(rand_fast_32(), second);
    TEST_NOT_EQUALS(first, second);

    _rng_state_64 = 987654321ULL;
    const uint64 first64 = rand_fast_64();
    const uint64 second64 = rand_fast_64();

    _rng_state_64 = 987654321ULL;
    TEST_EQUALS(rand_fast_64(), first64);
    TEST_EQUALS(rand_fast_64(), second64);
    TEST_NOT_EQUALS(first64, second64);

    uint32 state32 = 42;
    const uint32 value_a = rand_fast(&state32);
    const uint32 value_b = rand_fast(&state32);
    TEST_EQUALS(state32, value_b);
    TEST_NOT_EQUALS(value_a, value_b);

    uint64 state64 = 42;
    const uint64 value_a64 = rand_fast(&state64);
    const uint64 value_b64 = rand_fast(&state64);
    TEST_EQUALS(state64, value_b64);
    TEST_NOT_EQUALS(value_a64, value_b64);

    // A zero state is a fixed point of the generator
    _rng_state_32 = 0;
    TEST_EQUALS(rand_fast_32(), 0);
    TEST_EQUALS(rand_fast_32(), 0);

    _rng_state_64 = 0;
    TEST_EQUALS(rand_fast_64(), 0);
    TEST_EQUALS(rand_fast_64(), 0);
}

static void test_random_utils_range()
{
    uint32 state = 1;
    bool in_range_32 = true;

    for (int32 i = 0; i < 200; ++i) {
        const uint32 value = rand_fast(&state, 100);
        in_range_32 = in_range_32 && (value < 100);
    }
    TEST_TRUE(in_range_32);
    TEST_EQUALS(rand_fast(&state, 0), 0);

    _rng_state_32 = 7;
    bool in_range_global = true;
    for (int32 i = 0; i < 200; ++i) {
        in_range_global = in_range_global && (rand_fast(100) < 100);
    }
    TEST_TRUE(in_range_global);
    TEST_EQUALS(rand_fast(0), 0);

    _rng_state_32 = 7;
    bool in_range_min_max = true;
    for (int32 i = 0; i < 200; ++i) {
        const uint32 value = rand_fast(10, 20);
        in_range_min_max = in_range_min_max && (value >= 10) && (value < 20);
    }
    TEST_TRUE(in_range_min_max);

    // Empty range returns the lower bound
    TEST_EQUALS(rand_fast(5, 5), 5);

    // Negative bounds are returned as two's complement uint32 values
    _rng_state_32 = 7;
    bool in_range_negative = true;
    for (int32 i = 0; i < 100; ++i) {
        const int32 value = (int32) rand_fast(-10, 10);
        in_range_negative = in_range_negative && (value >= -10) && (value < 10);
    }
    TEST_TRUE(in_range_negative);

    // Loose distribution sanity check (deterministic because the state is fixed)
    _rng_state_32 = 123;
    int32 low_count = 0;
    for (int32 i = 0; i < 1000; ++i) {
        if (rand_fast(100) < 50) {
            ++low_count;
        }
    }
    TEST_TRUE(low_count > 350 && low_count < 650);
}

static void test_random_utils_uniform01()
{
    _rng_state_64 = 0;
    TEST_EQUALS_WITH_DELTA(rand_uniform01(), 0.0, 0.0);

    _rng_state_64 = 1;
    bool in_range = true;
    f64 lowest = 1.0;
    f64 highest = 0.0;

    for (int32 i = 0; i < 200; ++i) {
        const f64 value = rand_uniform01();
        in_range = in_range && (value >= 0.0) && (value < 1.0);

        if (value < lowest) {
            lowest = value;
        }

        if (value > highest) {
            highest = value;
        }
    }

    TEST_TRUE(in_range);
    TEST_TRUE(lowest < 0.5);
    TEST_TRUE(highest > 0.5);
    TEST_TRUE(highest < 1.0);
}

static void test_random_utils_percent()
{
    _rng_state_32 = 0;
    TEST_EQUALS_WITH_DELTA(rand_fast_percent(), 0.0f, 0.0f);

    _rng_state_32 = 999;
    bool in_range = true;
    f32 lowest = 1.0f;
    f32 highest = 0.0f;

    for (int32 i = 0; i < 200; ++i) {
        const f32 value = rand_fast_percent();
        in_range = in_range && (value >= 0.0f) && (value <= 1.0f);

        if (value < lowest) {
            lowest = value;
        }

        if (value > highest) {
            highest = value;
        }
    }

    TEST_TRUE(in_range);
    TEST_TRUE(lowest < highest);
}

static void test_random_utils_unique_permutation()
{
    int32 values[32];
    for (int32 i = 0; i < 32; ++i) {
        values[i] = i;
    }

    srand(0);
    random_unique(values, 32);

    bool seen[32] = {};
    bool in_range = true;
    bool duplicate = false;

    for (int32 i = 0; i < 32; ++i) {
        const int32 value = values[i];
        in_range = in_range && (value >= 0) && (value < 32);

        if (value >= 0 && value < 32) {
            duplicate = duplicate || seen[value];
            seen[value] = true;
        }
    }

    int32 seen_count = 0;
    for (int32 i = 0; i < 32; ++i) {
        if (seen[i]) {
            ++seen_count;
        }
    }

    TEST_TRUE(in_range);
    TEST_FALSE(duplicate);
    TEST_EQUALS(seen_count, 32);

    int32 single[1] = {7};
    srand(0);
    random_unique(single, 1);
    TEST_EQUALS(single[0], 7);

    int32 none[1] = {5};
    random_unique(none, 0);
    TEST_EQUALS(none[0], 5);
}

static void test_random_utils_weighted_index()
{
    srand(0);

    int32 single_weight[] = {5};
    bool single_ok = true;
    for (int32 i = 0; i < 50; ++i) {
        single_ok = single_ok && (random_weighted_index(single_weight, 1) == 0);
    }
    TEST_TRUE(single_ok);

    int32 weights[] = {5, 10, 20};
    bool in_range = true;
    for (int32 i = 0; i < 200; ++i) {
        const int32 index = random_weighted_index(weights, 3);
        in_range = in_range && (index >= 0) && (index < 3);
    }
    TEST_TRUE(in_range);
}

static void test_random_utils_string()
{
    char allowed[] = "ACGT";
    char out[33] = {};

    _rng_state_64 = 555;
    random_string(allowed, 4, out, 32);

    TEST_EQUALS((int32) strlen(out), 32);
    TEST_EQUALS(out[32], '\0');

    bool valid = true;
    for (int32 i = 0; i < 32; ++i) {
        valid = valid && (strchr(allowed, out[i]) != NULL);
    }
    TEST_TRUE(valid);

    char out_again[33] = {};
    _rng_state_64 = 555;
    random_string(allowed, 4, out_again, 32);
    TEST_TRUE(strcmp(out, out_again) == 0);

    char hex_allowed[] = "0123456789ABCDEF";
    char hex_out[65] = {};
    _rng_state_64 = 777;
    random_string(hex_allowed, 16, hex_out, 64);

    bool hex_valid = true;
    for (int32 i = 0; i < 64; ++i) {
        hex_valid = hex_valid && (strchr(hex_allowed, hex_out[i]) != NULL);
    }
    TEST_TRUE(hex_valid);
    TEST_EQUALS((int32) strlen(hex_out), 64);

    // A zero length output is still terminated
    char empty_out[4] = "xxx";
    random_string(allowed, 4, empty_out, 0);
    TEST_EQUALS(empty_out[0], '\0');
}

static void test_random_utils_str_random()
{
    char source[] = "abc";
    char destination[12] = {};

    _rng_state_32 = 999;
    str_random(destination, 12, source, 3);

    bool valid = true;
    for (int32 i = 0; i < 12; ++i) {
        valid = valid && (destination[i] == 'a' || destination[i] == 'b' || destination[i] == 'c');
    }
    TEST_TRUE(valid);

    char destination_again[12] = {};
    _rng_state_32 = 999;
    str_random(destination_again, 12, source, 3);
    TEST_TRUE(memcmp(destination, destination_again, 12) == 0);

    char single_source[5] = {};
    _rng_state_32 = 3;
    str_random(single_source, 5, source, 1);
    bool all_first = true;
    for (int32 i = 0; i < 5; ++i) {
        all_first = all_first && (single_source[i] == 'a');
    }
    TEST_TRUE(all_first);

    char untouched[4] = {'x', 'y', 'z', 'w'};
    _rng_state_32 = 5;
    str_random(untouched, 0, source, 3);
    TEST_EQUALS(untouched[0], 'x');
    TEST_EQUALS(untouched[3], 'w');
}

static void test_random_utils_setup()
{
    const uint64 before = time_index();
    rand_setup();
    const uint64 after = time_index();

    TEST_TRUE(_rng_state_64 >= before);
    TEST_TRUE(_rng_state_64 <= after);

    // Reseeding with the captured state reproduces the same first values
    const uint32 seed32 = _rng_state_32;
    const uint64 seed64 = _rng_state_64;

    const uint32 next32 = rand_fast_32();
    const uint64 next64 = rand_fast_64();

    _rng_state_32 = seed32;
    _rng_state_64 = seed64;

    TEST_EQUALS(rand_fast_32(), next32);
    TEST_EQUALS(rand_fast_64(), next64);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsRandomUtilsTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_random_utils_state_determinism);
    TEST_RUN(test_random_utils_range);
    TEST_RUN(test_random_utils_uniform01);
    TEST_RUN(test_random_utils_percent);
    TEST_RUN(test_random_utils_unique_permutation);
    TEST_RUN(test_random_utils_weighted_index);
    TEST_RUN(test_random_utils_string);
    TEST_RUN(test_random_utils_str_random);
    TEST_RUN(test_random_utils_setup);

    TEST_FINALIZE();

    return 0;
}
