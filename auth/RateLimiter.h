/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_AUTH_RATE_LIMITER_H
#define COMS_AUTH_RATE_LIMITER_H

#include "../stdlib/Stdlib.h"
#include "../hash/GeneralHash.h"

/**
 * Fixed window rate limiter
 *
 * Protects the authentication endpoints against brute force and flooding.
 * The limiter uses a caller provided bucket array (no allocations) and is
 * therefore safe to use in real-time server threads.
 *
 * The key is usually a hash of the client IP, the account id or a
 * combination of both. The window size and the limit are passed on every call
 * so one limiter can be used for different kinds of limits.
 *
 * WARNING: A fixed window allows a burst of up to 2x the limit at the window
 *          boundary. For authentication this is acceptable and much cheaper
 *          than a sliding window.
 */

struct RateLimitBucket {
    uint64 key;

    // Window start in unix seconds
    uint32 window_start;
    uint32 count;

    bool occupied;
};

struct RateLimiter {
    RateLimitBucket* buckets;

    // Always a power of two
    uint32 capacity;
    uint32 mask;

    // Randomizes the bucket distribution between server instances
    uint32 seed;
};

/**
 * Initialize a rate limiter
 *
 * @param RateLimiter*     limiter       Limiter
 * @param RateLimitBucket* buckets       Bucket memory
 * @param uint32           capacity      Amount of buckets (rounded down to a power of two)
 * @param uint32           seed          Hash seed
 */
inline
void rate_limiter_init(
    RateLimiter* const __restrict limiter,
    RateLimitBucket* const __restrict buckets,
    uint32 capacity,
    uint32 seed = 0
) NO_EXCEPT
{
    ASSERT_TRUE(limiter);
    ASSERT_TRUE(buckets);
    ASSERT_TRUE(capacity >= 2);

    uint32 power_of_two = 1;
    while ((power_of_two << 1) <= capacity) {
        power_of_two <<= 1;
    }

    limiter->buckets = buckets;
    limiter->capacity = power_of_two;
    limiter->mask = power_of_two - 1;
    limiter->seed = seed;

    memset(buckets, 0, sizeof(RateLimitBucket) * power_of_two);
}

/**
 * Find the bucket for a key
 *
 * @param RateLimiter* limiter Limiter
 * @param uint64       key     Key
 * @param uint32       now     Current unix time in seconds
 * @param uint32       window  Window size in seconds
 *
 * @return RateLimitBucket* Bucket or NULL if the table is full
 */
inline
RateLimitBucket* rate_limiter_bucket(
    RateLimiter* const __restrict limiter,
    uint64 key,
    uint32 now,
    uint32 window
) NO_EXCEPT
{
    const uint32 start = hash_downscale_64(key ^ limiter->seed) & limiter->mask;
    RateLimitBucket* stale = NULL;

    for (uint32 probe = 0; probe < limiter->capacity; ++probe) {
        RateLimitBucket* const bucket = &limiter->buckets[(start + probe) & limiter->mask];

        if (!bucket->occupied) {
            return bucket;
        }

        if (bucket->key == key) {
            return bucket;
        }

        const bool is_stale = (now - bucket->window_start) >= window;
        if (is_stale && !stale) {
            stale = bucket;
        }
    }

    // No free slot: recycle a stale entry if possible
    if (stale) {
        return stale;
    }

    return NULL;
}

/**
 * Consume one unit for a key
 *
 * @param RateLimiter* limiter Limiter
 * @param uint64       key     Key
 * @param uint32       limit   Maximum amount of units inside the window
 * @param uint32       now     Current unix time in seconds
 * @param uint32       window  Window size in seconds
 *
 * @return bool True if the request is allowed
 */
inline
bool rate_limiter_consume(
    RateLimiter* const __restrict limiter,
    uint64 key,
    uint32 limit,
    uint32 now,
    uint32 window = 60
) NO_EXCEPT
{
    if (!limit) {
        return false;
    }

    RateLimitBucket* const bucket = rate_limiter_bucket(limiter, key, now, window);
    if (!bucket) {
        // Fail closed: an attacker must not benefit from table pressure
        return false;
    }

    if (!bucket->occupied || bucket->key != key || (now - bucket->window_start) >= window) {
        bucket->occupied = true;
        bucket->key = key;
        bucket->window_start = now;
        bucket->count = 1;

        return true;
    }

    if (bucket->count >= limit) {
        return false;
    }

    ++bucket->count;

    return true;
}

/**
 * Amount of remaining units for a key
 *
 * @param RateLimiter* limiter Limiter
 * @param uint64       key     Key
 * @param uint32       limit   Maximum amount of units inside the window
 * @param uint32       now     Current unix time in seconds
 * @param uint32       window  Window size in seconds
 *
 * @return uint32 Remaining units
 */
inline
uint32 rate_limiter_remaining(
    RateLimiter* const __restrict limiter,
    uint64 key,
    uint32 limit,
    uint32 now,
    uint32 window = 60
) NO_EXCEPT
{
    RateLimitBucket* const bucket = rate_limiter_bucket(limiter, key, now, window);

    if (!bucket || !bucket->occupied || bucket->key != key || (now - bucket->window_start) >= window) {
        return limit;
    }

    return bucket->count >= limit ? 0 : limit - bucket->count;
}

/**
 * Reset the limit for a key (e.g. after a successful login)
 *
 * @param RateLimiter* limiter Limiter
 * @param uint64       key     Key
 */
inline
void rate_limiter_reset(RateLimiter* const __restrict limiter, uint64 key) NO_EXCEPT
{
    const uint32 start = hash_downscale_64(key ^ limiter->seed) & limiter->mask;

    for (uint32 probe = 0; probe < limiter->capacity; ++probe) {
        RateLimitBucket* const bucket = &limiter->buckets[(start + probe) & limiter->mask];

        if (!bucket->occupied) {
            return;
        }

        if (bucket->key == key) {
            bucket->count = 0;
            bucket->window_start = 0;

            return;
        }
    }
}

#endif
