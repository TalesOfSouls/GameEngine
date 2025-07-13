/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HASH_GENERAL_H
#define COMS_HASH_GENERAL_H

#include "../stdlib/Types.h"

inline
uint64 hash_djb2(const char* key) noexcept
{
    uint64 hash = 5381;
    int32 c;

    while (((c = *key++))) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

inline
uint64 hash_sdbm(const byte* key) noexcept
{
    uint64 hash = 0;
    int32 c;

    while ((c = *key++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

inline
uint64 hash_lose_lose(const byte* key) noexcept
{
	uint64 hash = 0;
	int32 c;

	while ((c = *key++)) {
	    hash += c;
    }

	return hash;
}

inline
uint64 hash_polynomial_rolling(const char* str) noexcept
{
    const int32 p = 31;
    const int32 m = 1000000009;
    uint64 hash = 0;
    uint64 p_pow = 1;

    while (*str) {
        hash = (hash + (*str - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
        str++;
    }

    return hash;
}

inline
uint64 hash_fnv1a(const char* str) noexcept
{
    const uint64 FNV_OFFSET_BASIS = 14695981039346656037UL;
    const uint64 FNV_PRIME = 1099511628211UL;
    uint64 hash = FNV_OFFSET_BASIS;

    while (*str) {
        hash ^= (byte) *str;
        hash *= FNV_PRIME;
        str++;
    }

    return hash;
}

inline
uint32 hash_oat(const char* str) noexcept
{
    uint32 hash = 0;

    while(*str) {
        hash += *str++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

inline
uint32 hash_ejb(const char* str) noexcept
{
	const uint32 PRIME1 = 37;
	const uint32 PRIME2 = 1048583;
	uint32 hash = 0;

	while (*str) {
		hash = hash * PRIME1 ^ (*str++ - ' ');
	}

	return hash % PRIME2;
}

#define ROTL32(x, r) ((x) << (r)) | ((x) >> (32 - (r)))
inline
uint32 hash_murmur3_32(const byte* key, size_t len, uint32 seed = 0) noexcept
{
    uint32 h = seed;
    uint32 k;

    // Process 32-bit blocks (4 bytes at a time)
    for (size_t i = 0; i < len / 4; ++i) {
        // Get 4 bytes from the key
        k = *(uint32 *) (key + i * 4);

        // Mix the 4 bytes into the hash
        k *= 0xcc9e2d51;
        k = ROTL32(k, 15);
        k *= 0x1b873593;

        h ^= k;
        h = ROTL32(h, 13);
        h = h * 5 + 0xe6546b64;
    }

    // Handle the remaining bytes (less than 4 bytes)
    k = 0;
    switch (len & 3) {
        case 3:
            k ^= key[len / 4 * 4 + 2] << 16;
            [[fallthrough]];
        case 2:
            k ^= key[len / 4 * 4 + 1] << 8;
            [[fallthrough]];
        case 1:
            k ^= key[len / 4 * 4];
            k *= 0xcc9e2d51;
            k = ROTL32(k, 15);
            k *= 0x1b873593;
            h ^= k;
    }

    // Finalize the hash
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

#define ROTL64(x, r) ((x) << (r)) | ((x) >> (64 - (r)))
uint64 hash_murmur3_64(const void* key, size_t len, uint64 seed = 0) noexcept
{
    const uint64 c1 = 0x87c37b91114253d5ULL;
    const uint64 c2 = 0x4cf5ad432745937fULL;

    const byte* data = (const byte*) key;
    const size_t nblocks = len / 16;

    uint64 h1 = seed;
    uint64 h2 = seed;

    // Process 128-bit blocks (16 bytes at a time)
    for (size_t i = 0; i < nblocks; ++i) {
        uint64 k1 = *(uint64 *) (data + i * 16);
        uint64 k2 = *(uint64 *) (data + i * 16 + 8);

        k1 *= c1;
        k1 = ROTL64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1 = ROTL64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = ROTL64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2 = ROTL64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    // Handle the remaining bytes (less than 16 bytes)
    const byte* tail = data + nblocks * 16;
    uint64 k1 = 0;
    uint64 k2 = 0;

    switch (len & 15) {
        case 15:
            k2 ^= ((uint64) tail[14]) << 48;
            [[fallthrough]];
        case 14:
            k2 ^= ((uint64) tail[13]) << 40;
            [[fallthrough]];
        case 13:
            k2 ^= ((uint64) tail[12]) << 32;
            [[fallthrough]];
        case 12:
            k2 ^= ((uint64) tail[11]) << 24;
            [[fallthrough]];
        case 11:
            k2 ^= ((uint64) tail[10]) << 16;
            [[fallthrough]];
        case 10:
            k2 ^= ((uint64) tail[9]) << 8;
            [[fallthrough]];
        case  9:
            k2 ^= ((uint64) tail[8]) << 0;
            k2 *= c2;
            k2 = ROTL64(k2, 33);
            k2 *= c1;
            h2 ^= k2;
            [[fallthrough]];
        case  8:
            k1 ^= ((uint64) tail[7]) << 56;
            [[fallthrough]];
        case  7:
            k1 ^= ((uint64) tail[6]) << 48;
            [[fallthrough]];
        case  6:
            k1 ^= ((uint64) tail[5]) << 40;
            [[fallthrough]];
        case  5:
            k1 ^= ((uint64) tail[4]) << 32;
            [[fallthrough]];
        case  4:
            k1 ^= ((uint64) tail[3]) << 24;
            [[fallthrough]];
        case  3:
            k1 ^= ((uint64) tail[2]) << 16;
            [[fallthrough]];
        case  2:
            k1 ^= ((uint64) tail[1]) << 8;
            [[fallthrough]];
        case  1:
            k1 ^= ((uint64) tail[0]) << 0;
            k1 *= c1;
            k1 = ROTL64(k1, 31);
            k1 *= c2;
            h1 ^= k1;
    }

    // Finalize the hash
    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 ^= h1 >> 33;
    h1 *= 0xff51afd7ed558ccdULL;
    h1 ^= h1 >> 33;
    h1 *= 0xc4ceb9fe1a85ec53ULL;
    h1 ^= h1 >> 33;

    h2 ^= h2 >> 33;
    h2 *= 0xff51afd7ed558ccdULL;
    h2 ^= h2 >> 33;
    h2 *= 0xc4ceb9fe1a85ec53ULL;
    h2 ^= h2 >> 33;

    h1 += h2;
    // h2 += h1;

    return h1;
}

////////////////////////////////////
// Seeded hash functions
////////////////////////////////////

inline
uint64 hash_djb2_seeded(const char* key, int32 seed) noexcept
{
    uint64 hash = 5381;
    int32 c;

    while (((c = *key++))) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash ^ (seed + (seed << 6) + (seed >> 2));
}

inline
uint64 hash_sdbm_seeded(const char* key, int32 seed) noexcept
{
    uint64 hash = 0;
    int32 c;

    while ((c = *key++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash ^ (seed + (seed << 6) + (seed >> 2));
}

inline
uint64 hash_lose_lose_seeded(const char* key, int32 seed) noexcept
{
	uint64 hash = 0;
	int32 c;

	while ((c = *key++)) {
	    hash += c;
    }

	return hash ^ (seed + (seed << 6) + (seed >> 2));
}

inline
uint64 hash_polynomial_rolling_seeded(const char* str, int32 seed) noexcept
{
    const int32 p = 31;
    const int32 m = 1000000009;
    uint64 hash = 0;
    uint64 p_pow = 1;

    while (*str) {
        hash = (hash + (*str - 'a' + 1) * p_pow) % m;
        p_pow = (p_pow * p) % m;
        str++;
    }

    return hash ^ (seed + (seed << 6) + (seed >> 2));
}

inline
uint64 hash_fnv1a_seeded(const char* str, int32 seed) noexcept
{
    const uint64 FNV_OFFSET_BASIS = 14695981039346656037UL;
    const uint64 FNV_PRIME = 1099511628211UL;
    uint64 hash = FNV_OFFSET_BASIS;

    while (*str) {
        hash ^= (byte) *str;
        hash *= FNV_PRIME;
        str++;
    }

    return hash ^ (seed + (seed << 6) + (seed >> 2));
}

inline
uint64 hash_oat_seeded(const char* str, int32 seed) noexcept
{
    uint64 hash = 0;

    while(*str) {
        hash += *str++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash ^ (seed + (seed << 6) + (seed >> 2));;
}

inline
uint64 hash_ejb_seeded(const char* str, int32 seed) noexcept
{
	const uint64 PRIME1 = 37;
	const uint64 PRIME2 = 1048583;
	uint64 hash = 0;

	while (*str) {
		hash = hash * PRIME1 ^ (*str++ - ' ');
	}

	return (hash % PRIME2) ^ (seed + (seed << 6) + (seed >> 2));;
}

inline
uint32 intrin_hash(uint64 a, uint64 b = 0) noexcept
{
    uint8 seed[16] = {
        0xaa, 0x9b, 0xbd, 0xb8, 0xa1, 0x98, 0xac, 0x3f, 0x1f, 0x94, 0x07, 0xb3, 0x8c, 0x27, 0x93, 0x69,
    };

    __m128i hash = _mm_set_epi64x(a, b);
    hash = _mm_aesdec_si128(hash, _mm_load_si128((__m128i *) seed));
    hash = _mm_aesdec_si128(hash, _mm_load_si128((__m128i *) seed));

    return _mm_extract_epi32(hash, 0);
}

#endif