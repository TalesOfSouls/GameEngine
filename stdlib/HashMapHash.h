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

// Unaligned loads (memcpy compiles to a single mov)
FORCE_INLINE
uint64 hashmap_read64(const char* p) NO_EXCEPT
{
    uint64 v;
    memcpy(&v, p, sizeof(v));

    return v;
}

FORCE_INLINE
uint64 hashmap_read32(const char* p) NO_EXCEPT
{
    uint32 v;
    memcpy(&v, p, sizeof(v));

    return v;
}

FORCE_INLINE
uint64 hashmap_word_mix(uint64 h, uint64 w) NO_EXCEPT
{
    h = (h ^ w) * 0x9E3779B97F4A7C15ULL;

    return h ^ (h >> 32);
}

/**
 * Length of a \0 terminated string
 *
 * Faster than strlen for short keys since it has no call overhead and no alignment prologue.
 * Only aligned 16 byte blocks are loaded -> a load never crosses a page boundary,
 * the bytes before/after the string in the same block are read but ignored
 */
FORCE_INLINE
int32 hashmap_str_length(const char* str) NO_EXCEPT
{
    #ifdef __aarch64__
        return (int32) strlen(str);
    #else
        const char* block = (const char *) ((uintptr_t) str & ~(uintptr_t) 15);
        const __m128i zero = _mm_setzero_si128();

        uint32 mask = (uint32) _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((const __m128i *) block), zero))
            >> (uint32) (str - block);

        if (mask) {
            return compiler_find_first_bit_r2l(mask);
        }

        for (;;) {
            block += 16;
            mask = (uint32) _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128((const __m128i *) block), zero));

            if (mask) {
                return (int32) (block - str) + compiler_find_first_bit_r2l(mask);
            }
        }
    #endif
}

/**
 * If the key is longer than HASH_MAP_MAX_KEY_LENGTH - 1 only the last HASH_MAP_MAX_KEY_LENGTH - 1 characters are used
 * (this is also what gets stored in the entry, which makes it possible to look up/remove an entry by its stored key)
 *
 * @param const char**  key     In: key, Out: start of the stored key part
 * @param int32*        length  In: key length, Out: length of the stored key part (without \0)
 */
FORCE_INLINE
void hashmap_str_key(const char** key, int32* length) NO_EXCEPT
{
    if (*length > HASH_MAP_MAX_KEY_LENGTH - 1) {
        *key += *length - (HASH_MAP_MAX_KEY_LENGTH - 1);
        *length = HASH_MAP_MAX_KEY_LENGTH - 1;
    }
}

/**
 * Hashes a key with known length 8 bytes at a time (no byte loop, no \0 search)
 *
 * Keys <= 8 characters are loaded as 1 word (overlapping loads for 4..8 characters)
 * Longer keys are loaded word by word, the last word overlaps with the previous one
 * The length is part of the seed -> overlapping loads can't create collisions between different lengths
 *
 * @param const char*   key     Stored key part (@see hashmap_str_key)
 * @param int32         length  Length of the stored key part (without \0)
 */
FORCE_INLINE
uint32 hashmap_str_hash(const char* key, int32 length) NO_EXCEPT
{
    uint64 h = 0xA0761D6478BD642FULL ^ (uint64) length;

    if (length <= 8) {
        uint64 w;
        if (length >= 4) {
            w = hashmap_read32(key) | (hashmap_read32(key + length - 4) << 32);
        } else if (length > 0) {
            w = ((uint64) (byte) key[0] << 16)
                | ((uint64) (byte) key[length >> 1] << 8)
                | (uint64) (byte) key[length - 1];
        } else {
            w = 0;
        }

        h = hashmap_word_mix(h, w);
    } else {
        for (int32 i = 0; i + 8 < length; i += 8) {
            h = hashmap_word_mix(h, hashmap_read64(key + i));
        }

        h = hashmap_word_mix(h, hashmap_read64(key + length - 8));
    }

    // Murmur3 fmix64
    h ^= h >> 33;
    h *= 0xFF51AFD7ED558CCDULL;
    h ^= h >> 33;
    h *= 0xC4CEB9FE1A85EC53ULL;
    h ^= h >> 33;

    return (uint32) h;
}

/**
 * Measures + hashes a \0 terminated key
 *
 * @param const char**  key     In: key, Out: start of the stored key part
 * @param int32*        length  Out: length of the stored key part (without \0)
 */
FORCE_INLINE
uint32 hashmap_str_hash(const char** key, int32* length) NO_EXCEPT
{
    *length = hashmap_str_length(*key);
    hashmap_str_key(key, length);

    return hashmap_str_hash(*key, *length);
}

/**
 * Compares a stored key (\0 terminated, HASH_MAP_MAX_KEY_LENGTH bytes) with a key of known length
 * Compares 8 bytes at a time, the \0 check of the stored key rejects longer stored keys
 *
 * @param const char*   stored  Stored key of the entry
 * @param const char*   key     Stored key part (@see hashmap_str_key)
 * @param int32         length  Length of the stored key part (without \0)
 */
FORCE_INLINE
bool hashmap_str_equals(const char* stored, const char* key, int32 length) NO_EXCEPT
{
    if (stored[length] != '\0') {
        return false;
    }

    if (length <= 8) {
        if (length >= 4) {
            return hashmap_read32(stored) == hashmap_read32(key)
                && hashmap_read32(stored + length - 4) == hashmap_read32(key + length - 4);
        }

        for (int32 i = 0; i < length; ++i) {
            if (stored[i] != key[i]) {
                return false;
            }
        }

        return true;
    }

    for (int32 i = 0; i + 8 < length; i += 8) {
        if (hashmap_read64(stored + i) != hashmap_read64(key + i)) {
            return false;
        }
    }

    return hashmap_read64(stored + length - 8) == hashmap_read64(key + length - 8);
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
