/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HASH_SHA1_SIMD_X86_H
#define COMS_HASH_SHA1_SIMD_X86_H

#include <immintrin.h>
#include "../stdlib/Types.h"
#include "Sha1Definitions.h"

static
void sha1_transform(SHA1_CTX* ctx, const byte data[64], int32 steps) {
    uint32 a, b, c, d, e, temp;
    alignas(64) uint32 w[80];

    // @question Does it make sense to also do SIMD here?
    for (int32 i = 0; i < 16; ++i) {
        w[i] = ((uint32) data[i * 4 + 0] << 24)
            | ((uint32) data[i * 4 + 1] << 16)
            | ((uint32) data[i * 4 + 2] <<  8)
            | ((uint32) data[i * 4 + 3]);
    }

    #ifdef __AVX512F__
    if (steps >= 16) {
        for (int i = 16; i < 80; i += 16) {
            __m512i v3 = _mm512_loadu_si512((__m512i*)&w[i-3]);
            __m512i v8 = _mm512_load_si512((__m512i*)&w[i-8]);
            __m512i v14 = _mm512_loadu_si512((__m512i*)&w[i-14]);
            __m512i v16 = _mm512_load_si512((__m512i*)&w[i-16]);

            __m512i v = _mm512_xor_si512(v3, v8);
            v = _mm512_xor_si512(v, v14);
            v = _mm512_xor_si512(v, v16);

            __m512i v_rot = _mm512_or_si512(
                _mm512_slli_epi32(v, 1),
                _mm512_srli_epi32(v, 31)
            );

            _mm512_store_si512((__m512i*)&w[i], v_rot);
        }

        steps = 1
    } else
    #endif

    #ifdef __AVX2__
    if (steps >= 8) {
        for (int i = 16; i < 80; i += 8) {
            __m256i v3 = _mm256_loadu_si256((__m256i*)&w[i-3]);
            __m256i v8 = _mm256_load_si256((__m256i*)&w[i-8]);
            __m256i v14 = _mm256_loadu_si256((__m256i*)&w[i-14]);
            __m256i v16 = _mm256_load_si256((__m256i*)&w[i-16]);

            __m256i v = _mm256_xor_si256(v3, v8);
            v = _mm256_xor_si256(v, v14);
            v = _mm256_xor_si256(v, v16);

            __m256i v_rot = _mm256_or_si256(
                _mm256_slli_epi32(v, 1),
                _mm256_srli_epi32(v, 31)
            );

            _mm256_store_si256((__m256i*)&w[i], v_rot);
        }

        steps = 1;
    } else
    #endif

    #ifdef __SSE4_2__
    if (steps >= 4) {
        for (int32 i = 16; i < 80; i += 4) {
            __m128i v = _mm_xor_si128(
                _mm_loadu_si128((__m128i*) &w[i-3]),
                _mm_load_si128((__m128i*) &w[i-8])
            );
            v = _mm_xor_si128(v, _mm_loadu_si128((__m128i*) &w[i-14]));
            v = _mm_xor_si128(v, _mm_load_si128((__m128i*) &w[i-16]));

            v = _mm_or_si128(_mm_slli_epi32(v, 1), _mm_srli_epi32(v, 31));

            _mm_store_si128((__m128i*) &w[i], v);
        }
    }
    #endif

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];

    for (int32 i = 0; i < 80; ++i) {
        if (i < 20) {
            temp = SHA1_ROTL32(a, 5) + SHA1_Ch(b, c, d) + e + K1 + w[i];
        } else if (i < 40) {
            temp = SHA1_ROTL32(a, 5) + SHA1_Parity(b, c, d) + e + K2 + w[i];
        } else if (i < 60) {
            temp = SHA1_ROTL32(a, 5) + SHA1_Maj(b, c, d) + e + K3 + w[i];
        } else {
            temp = SHA1_ROTL32(a, 5) + SHA1_Parity(b, c, d) + e + K4 + w[i];
        }

        e = d;
        d = c;
        c = SHA1_ROTL32(b, 30);
        b = a;
        a = temp;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

#endif