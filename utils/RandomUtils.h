/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_UTILS_RANDOM_H
#define COMS_UTILS_RANDOM_H

#include "../stdlib/Stdlib.h"
#include "../utils/Assert.h"
#include "../utils/TimeUtils.h"

thread_local uint32 _rng_state_32;
thread_local uint64 _rng_state_64;

FORCE_INLINE
void rand_setup() NO_EXCEPT
{
    _rng_state_32 = (int32) time_index();
    _rng_state_64 = time_index();
}

// PERFORMANCE: Approx. 4x faster than rand()
inline
uint32 rand_fast(uint32* state) NO_EXCEPT
{
    static const uint32 z = 0x9E3779B9;
    uint32 x = *state;

    x ^= ((x << 13) | (x >> 19)) ^ ((x << 5) | (x >> 27));
    x *= z;
    x ^= x >> 16;
    x *= z;
    x ^= x >> 15;

    *state = x;

    return x;
}

inline
uint32 rand_fast_32() NO_EXCEPT
{
    static const uint32 z = 0x9E3779B9;
    uint32 x = _rng_state_32;

    x ^= ((x << 13) | (x >> 19)) ^ ((x << 5) | (x >> 27));
    x *= z;
    x ^= x >> 16;
    x *= z;
    x ^= x >> 15;

    _rng_state_32 = x;

    return x;
}

FORCE_INLINE
f32 rand_fast_percent() NO_EXCEPT
{
    return (f32) rand_fast_32() / (f32) MAX_UINT32;
}

inline
uint64 rand_fast(uint64* state) NO_EXCEPT
{
    static const uint64 z = 0x9FB21C651E98DF25;
    uint64 x = *state;

    x ^= ((x << 49) | (x >> 15)) ^ ((x << 24) | (x >> 40));
    x *= z;
    x ^= x >> 35;
    x *= z;
    x ^= x >> 28;

    *state = x;

    return x;
}

inline
uint64 rand_fast_64() NO_EXCEPT
{
    static const uint64 z = 0x9FB21C651E98DF25;
    uint64 x = _rng_state_64;

    x ^= ((x << 49) | (x >> 15)) ^ ((x << 24) | (x >> 40));
    x *= z;
    x ^= x >> 35;
    x *= z;
    x ^= x >> 28;

    _rng_state_64 = x;

    return x;
}

FORCE_INLINE
uint32 rand_fast(uint32* state, int32 max) NO_EXCEPT
{
    return (uint32) (((uint64) rand_fast(state) * max) >> 32);
}

FORCE_INLINE
uint32 rand_fast(int32 max) NO_EXCEPT
{
    return (uint32) (((uint64) rand_fast_32() * max) >> 32);
}

FORCE_INLINE
uint32 rand_fast(int32 min, int32 max) NO_EXCEPT
{
    uint32 span = (uint32)(max - min);
    uint32 r = (uint32) (((uint64) rand_fast_32() * span) >> 32);
    return (uint32) (min + r);
}

FORCE_INLINE
f64 rand_uniform01() NO_EXCEPT
{
    /**
    * produce f64 in [0,1)
    * use upper 53 bits of randomness for IEEE f64
    */
    return (f64)(rand_fast_64() >> 11) / (f64)(1ULL << 53);
}

/**
 * Picks n random elements from end and stores them in begin.
 */
inline
void random_unique(int32* array, int32 size) NO_EXCEPT
{
    for (int32 i = size - 1; i > 0; --i) {
        const int32 j = rand() % (i + 1);

        const int32 temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

/**
 * Gets random index based value probability
 */
int32 random_weighted_index(const int32* arr, int32 array_count) NO_EXCEPT
{
    uint32 prob_sum = 0;
    for (int32 i = 0; i < array_count; ++i) {
        prob_sum += arr[i];
    }

    const uint32 random_prob = rand() % (prob_sum + 1);
    uint32 current_rarity = 0;
    int32 item_rarity = array_count - 1;
    for (int32 i = 0; i < array_count - 1; ++i) {
        current_rarity += arr[i];

        if (current_rarity < random_prob) {
            item_rarity = i;
            break;
        }
    }

    return item_rarity;
}

// WARNING: The allowed_chars string length needs to be of power 2 for performance reasons
//      Supporting any allowed_chars length is trivial but usually we prefer the performance improvement
void random_string(const char* allowed_chars, uint32 allowed_length, char* out, int32 out_length) NO_EXCEPT
{
    ASSERT_TRUE((allowed_length & 2) == 0);

    const uint32 mask = allowed_length - 1;

    //uint64 x = time_index();

    size_t i = 0;
    while (i < out_length) {
        const uint64 rand_val = rand_fast_64();

        for (int32 j = 0; j < 8 && i < out_length; ++j, ++i) {
            out[i] = allowed_chars[((rand_val >> (8 * j)) & 0xFF) & mask];
        }
    }

    out[out_length] = '\0';
}

#endif
