/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_HASH_MAP_HASH_H
#define COMS_STDLIB_HASH_MAP_HASH_H

#include "Stdlib.h"
#include "HashMapT.h"

/**
 * Hash helpers shared by HashMapT and ThrdHashMapT
 */

// Murmur3 fmix32, makes sure all bits of the hash depend on all key characters
FORCE_INLINE
uint32 hashmap_hash_mix(uint32 h) NO_EXCEPT
{
    h ^= h >> 16;
    h *= 0x85EBCA6BU;
    h ^= h >> 13;
    h *= 0xC2B2AE35U;
    h ^= h >> 16;

    return h;
}

/**
 * Hashes the key and calculates its length in one pass (djb2 + fmix32)
 *
 * If the key is longer than HASH_MAP_MAX_KEY_LENGTH - 1 only the last HASH_MAP_MAX_KEY_LENGTH - 1 characters are used
 * (this is also what gets stored in the entry, which makes it possible to look up/remove an entry by its stored key)
 *
 * @param key    In: key, Out: start of the stored key part
 * @param length Out: length of the stored key part (without \0)
 */
FORCE_INLINE
uint32 hashmap_str_hash(const char** key, int32* length) NO_EXCEPT
{
    const char* str = *key;
    const char* pos = str;
    uint32 hash = 5381;

    while (*pos) {
        hash = ((hash << 5) + hash) + (byte) *pos;
        ++pos;
    }

    int32 len = (int32) (pos - str);
    if (len > HASH_MAP_MAX_KEY_LENGTH - 1) {
        // Rare case: Only the tail of the key is stored -> only the tail is hashed
        str = pos - (HASH_MAP_MAX_KEY_LENGTH - 1);
        len = HASH_MAP_MAX_KEY_LENGTH - 1;

        hash = 5381;
        for (int32 i = 0; i < len; ++i) {
            hash = ((hash << 5) + hash) + (byte) str[i];
        }
    }

    *key = str;
    *length = len;

    return hashmap_hash_mix(hash);
}

// Hash for non-string keys (hashes the key bytes, NOT a null terminated string)
template <typename K>
FORCE_INLINE
uint32 hashmap_int_hash(const K& key) NO_EXCEPT
{
    uint64 h = 0;
    if CONSTEXPR (sizeof(K) <= sizeof(uint64)) {
        memcpy(&h, &key, sizeof(K));
    } else {
        // FNV-1a over the key bytes
        // WARNING: struct keys must not contain uninitialized padding bytes
        const byte* data = (const byte *) &key;
        h = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < sizeof(K); ++i) {
            h = (h ^ data[i]) * 0x100000001B3ULL;
        }
    }

    // Murmur3 fmix64
    h ^= h >> 33;
    h *= 0xFF51AFD7ED558CCDULL;
    h ^= h >> 33;
    h *= 0xC4CEB9FE1A85EC53ULL;
    h ^= h >> 33;

    return (uint32) h;
}

// Maps a hash to [0, count) for ANY count (not only powers of 2), uses the high bits of the hash
FORCE_INLINE
uint32 hashmap_hash_range(uint32 hash, uint32 count) NO_EXCEPT
{
    return (uint32) (((uint64) hash * count) >> 32);
}

#endif
