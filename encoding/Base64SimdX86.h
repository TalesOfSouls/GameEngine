/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENCODING_BASE64_SIMD_X86_H
#define COMS_ENCODING_BASE64_SIMD_X86_H

#include <immintrin.h>
#include "../stdlib/Types.h"
#include "../stdlib/Simd.h"
#include "../utils/StringUtils.h"
#include "Base64Definitions.h"

void base64_encode_simd(const byte* data, char* encoded_data, size_t data_length = 0, int32 steps = 16) {
    if (!data_length) {
        // WARNING: This should only happen if the data is a char string
        // Binary data is not allowed since it often has '\0' characters
        data_length = str_length((const char *) data);
    }

    steps = intrin_validate_steps(data, steps);
    steps = intrin_validate_steps((const byte*) encoded_data, steps);

    size_t i = 0;

    #ifdef __AVX512F__
    if (steps >= 16) {
        const __m512i mask0 = _mm512_set1_epi32(0x00FC0000);
        const __m512i mask1 = _mm512_set1_epi32(0x0003F000);
        const __m512i mask2 = _mm512_set1_epi32(0x00000FC0);
        const __m512i mask3 = _mm512_set1_epi32(0x0000003F);

        const __m512i shift0 = _mm512_set1_epi32(18);
        const __m512i shift1 = _mm512_set1_epi32(12);
        const __m512i shift2 = _mm512_set1_epi32(6);
        const __m512i shift3 = _mm512_set1_epi32(0);

        const __m512i shuffle_mask = _mm512_set_epi8(
            5,4,6,5,7,6,8,7,9,8,10,9,11,10,12,11,
            1,0,2,1,3,2,4,3,5,4,6,5,7,6,8,7,
            5,4,6,5,7,6,8,7,9,8,10,9,11,10,12,11,
            1,0,2,1,3,2,4,3,5,4,6,5,7,6,8,7
        );

        const __m512i permute_mask = _mm512_set_epi32(15,13,11,9,7,5,3,1,14,12,10,8,6,4,2,0);

        for (; i + 47 < data_length; i += 48) {
            __m512i in = _mm512_loadu_si512((const __m512i*)(data + i));

            __m512i shuffled = _mm512_shuffle_epi8(in, shuffle_mask);

            __m512i permuted = _mm512_permutexvar_epi32(permute_mask, shuffled);

            __m512i indices0 = _mm512_srlv_epi32(permuted, shift0);
            __m512i indices1 = _mm512_srlv_epi32(permuted, shift1);
            __m512i indices2 = _mm512_srlv_epi32(permuted, shift2);
            __m512i indices3 = _mm512_srlv_epi32(permuted, shift3);

            indices0 = _mm512_and_si512(indices0, mask0);
            indices1 = _mm512_and_si512(indices1, mask1);
            indices2 = _mm512_and_si512(indices2, mask2);
            indices3 = _mm512_and_si512(indices3, mask3);

            __m512i indices = _mm512_or_si512(
                _mm512_or_si512(indices0, indices1),
                _mm512_or_si512(indices2, indices3)
            );

            alignas(64) uint32 idx[16];
            _mm512_store_si512((__m512i*)idx, indices);

            for (int32 k = 0; k < 16; ++k) {
                encoded_data[i/3*4 + k*4 + 0] = BASE64_CHARS[(idx[k] >> 18) & 0x3F];
                encoded_data[i/3*4 + k*4 + 1] = BASE64_CHARS[(idx[k] >> 12) & 0x3F];
                encoded_data[i/3*4 + k*4 + 2] = BASE64_CHARS[(idx[k] >> 6) & 0x3F];
                encoded_data[i/3*4 + k*4 + 3] = BASE64_CHARS[(idx[k] >> 0) & 0x3F];
            }
        }

        steps = 1;
    } else
    #endif

    #ifdef __AVX2__
    if (steps >= 8) {
        const __m256i mask0 = _mm256_set1_epi32(0x00FC0000);
        const __m256i mask1 = _mm256_set1_epi32(0x0003F000);
        const __m256i mask2 = _mm256_set1_epi32(0x00000FC0);
        const __m256i mask3 = _mm256_set1_epi32(0x0000003F);

        const __m256i shift0 = _mm256_set1_epi32(18);
        const __m256i shift1 = _mm256_set1_epi32(12);
        const __m256i shift2 = _mm256_set1_epi32(6);
        const __m256i shift3 = _mm256_set1_epi32(0);

        const __m256i shuffle_mask = _mm256_set_epi8(
            5,4,6,5,7,6,8,7,9,8,10,9,11,10,12,11,
            1,0,2,1,3,2,4,3,5,4,6,5,7,6,8,7
        );

        const __m256i permute_mask = _mm256_set_epi32(7,5,3,1,6,4,2,0);

        for (; i + 23 < data_length; i += 24) {
            __m256i in = _mm256_loadu_si256((const __m256i*)(data + i));

            __m256i shuffled = _mm256_shuffle_epi8(in, shuffle_mask);

            __m256i permuted = _mm256_permutevar8x32_epi32(shuffled, permute_mask);

            __m256i indices0 = _mm256_srlv_epi32(permuted, shift0);
            __m256i indices1 = _mm256_srlv_epi32(permuted, shift1);
            __m256i indices2 = _mm256_srlv_epi32(permuted, shift2);
            __m256i indices3 = _mm256_srlv_epi32(permuted, shift3);

            indices0 = _mm256_and_si256(indices0, mask0);
            indices1 = _mm256_and_si256(indices1, mask1);
            indices2 = _mm256_and_si256(indices2, mask2);
            indices3 = _mm256_and_si256(indices3, mask3);

            __m256i indices = _mm256_or_si256(
                _mm256_or_si256(indices0, indices1),
                _mm256_or_si256(indices2, indices3)
            );

            alignas(32) uint32 idx[8];
            _mm256_store_si256((__m256i*)idx, indices);

            for (int32 k = 0; k < 8; ++k) {
                encoded_data[i/3*4 + k*4 + 0] = BASE64_CHARS[(idx[k] >> 18) & 0x3F];
                encoded_data[i/3*4 + k*4 + 1] = BASE64_CHARS[(idx[k] >> 12) & 0x3F];
                encoded_data[i/3*4 + k*4 + 2] = BASE64_CHARS[(idx[k] >> 6) & 0x3F];
                encoded_data[i/3*4 + k*4 + 3] = BASE64_CHARS[(idx[k] >> 0) & 0x3F];
            }
        }

        steps = 1;
    } else
    #endif

    #ifdef __SSE4_2__
    if (steps >= 4) {
        const __m128i mask0 = _mm_set1_epi32(0x00FC0000);
        const __m128i mask1 = _mm_set1_epi32(0x0003F000);
        const __m128i mask2 = _mm_set1_epi32(0x00000FC0);
        const __m128i mask3 = _mm_set1_epi32(0x0000003F);

        const __m128i shift0 = _mm_set1_epi32(18);
        const __m128i shift1 = _mm_set1_epi32(12);
        const __m128i shift2 = _mm_set1_epi32(6);
        const __m128i shift3 = _mm_set1_epi32(0);

        const __m128i shuffle_mask = _mm_set_epi8(
            5,4,6,5,7,6,8,7,9,8,10,9,11,10,12,11
        );

        for (; i + 11 < data_length; i += 12) {
            __m128i in = _mm_loadu_si128((const __m128i*)(data + i));

            __m128i shuffled = _mm_shuffle_epi8(in, shuffle_mask);

            __m128i indices0 = _mm_srlv_epi32(shuffled, shift0);
            __m128i indices1 = _mm_srlv_epi32(shuffled, shift1);
            __m128i indices2 = _mm_srlv_epi32(shuffled, shift2);
            __m128i indices3 = _mm_srlv_epi32(shuffled, shift3);

            indices0 = _mm_and_si128(indices0, mask0);
            indices1 = _mm_and_si128(indices1, mask1);
            indices2 = _mm_and_si128(indices2, mask2);
            indices3 = _mm_and_si128(indices3, mask3);

            __m128i indices = _mm_or_si128(_mm_or_si128(indices0, indices1), _mm_or_si128(indices2, indices3));

            alignas(16) uint32 idx[4];
            _mm_store_si128((__m128i*)idx, indices);

            for (int32 k = 0; k < 4; ++k) {
                encoded_data[i/3*4 + k*4 + 0] = BASE64_CHARS[(idx[k] >> 18) & 0x3F];
                encoded_data[i/3*4 + k*4 + 1] = BASE64_CHARS[(idx[k] >> 12) & 0x3F];
                encoded_data[i/3*4 + k*4 + 2] = BASE64_CHARS[(idx[k] >> 6) & 0x3F];
                encoded_data[i/3*4 + k*4 + 3] = BASE64_CHARS[(idx[k] >> 0) & 0x3F];
            }
        }
    }
    #endif

    for (; i + 3 <= data_length; i += 3) {
        uint32 triple = ((uint32)data[i] << 16) | ((uint32)data[i + 1] << 8) | data[i + 2];

        encoded_data[i/3*4 + 0] = BASE64_CHARS[(triple >> 18) & 0x3F];
        encoded_data[i/3*4 + 1] = BASE64_CHARS[(triple >> 12) & 0x3F];
        encoded_data[i/3*4 + 2] = BASE64_CHARS[(triple >> 6) & 0x3F];
        encoded_data[i/3*4 + 3] = BASE64_CHARS[triple & 0x3F];
    }

    if (i < data_length) {
        uint32 triple = ((uint32)data[i] << 16);
        if (i + 1 < data_length) {
            triple |= ((uint32)data[i + 1] << 8);
        }

        size_t j = i/3*4;
        encoded_data[j++] = BASE64_CHARS[(triple >> 18) & 0x3F];
        encoded_data[j++] = BASE64_CHARS[(triple >> 12) & 0x3F];
        encoded_data[j++] = (i + 1 < data_length) ? BASE64_CHARS[(triple >> 6) & 0x3F] : '=';
        encoded_data[j] = '=';
    }

    encoded_data[base64_encoded_length(data_length)] = '\0';
}

size_t base64_decode_simd(const char* encoded_data, byte* data, size_t encoded_length, int32 steps = 16) {
    if (!encoded_length) {
        encoded_length = str_length(encoded_data);
    }

    size_t padding = 0;
    if (encoded_data[encoded_length - 1] == '=') {
        ++padding;

        if (encoded_data[encoded_length - 2] == '=') {
            ++padding;
        }
    }

    size_t output_length = (encoded_length / 4) * 3 - padding;
    size_t complete_blocks = (encoded_length - padding) / 4;

    steps = intrin_validate_steps((const byte*)encoded_data, steps);
    steps = intrin_validate_steps(data, steps);

    size_t i = 0;
    size_t j = 0;

    #ifdef __AVX512F__
    if (steps >= 16) {
        steps = 16;

        const __m512i mask6bits = _mm512_set1_epi32(0x3F);
        const __m512i shuffle_mask = _mm512_set_epi8(
            -1,-1,-1,-1,-1,-1,11,10,9,8,7,6,5,4,3,2,
            -1,-1,-1,-1,-1,-1,11,10,9,8,7,6,5,4,3,2,
            -1,-1,-1,-1,-1,-1,11,10,9,8,7,6,5,4,3,2,
            -1,-1,-1,-1,-1,-1,11,10,9,8,7,6,5,4,3,2
        );

        const __m512i permute_mask = _mm512_set_epi32(0,0,0,0,0,0,0,0,14,12,10,8,6,4,2,0);

        for (; i + 63 < complete_blocks * 4; i += 64, j += 48) {
            __m512i in = _mm512_loadu_si512((const __m512i*)(encoded_data + i));

            alignas(64) byte chars[64];
            _mm512_store_si512((__m512i*)chars, in);

            alignas(64) uint32 sextets[16];
            for (int32 k = 0; k < 16; ++k) {
                sextets[k] =
                    (BASE64_LOOKUP[(byte)chars[k*4 + 0]] << 18) |
                    (BASE64_LOOKUP[(byte)chars[k*4 + 1]] << 12) |
                    (BASE64_LOOKUP[(byte)chars[k*4 + 2]] << 6) |
                    (BASE64_LOOKUP[(byte)chars[k*4 + 3]]);
            }

            __m512i sextet_vec = _mm512_load_si512((const __m512i*)sextets);

            __m512i bytes0 = _mm512_srli_epi32(sextet_vec, 16);
            __m512i bytes1 = _mm512_srli_epi32(sextet_vec, 8);
            __m512i bytes2 = sextet_vec;

            bytes0 = _mm512_and_si512(bytes0, _mm512_set1_epi32(0xFF));
            bytes1 = _mm512_and_si512(bytes1, _mm512_set1_epi32(0xFF));
            bytes2 = _mm512_and_si512(bytes2, _mm512_set1_epi32(0xFF));

            __m512i packed = _mm512_shuffle_epi8(_mm512_packus_epi32(bytes0, bytes1), shuffle_mask);
            packed = _mm512_permutexvar_epi32(permute_mask, packed);

            _mm512_storeu_si512((__m512i*)(data + j), packed);
        }

        steps = 1;
    }
    #endif

    #ifdef __AVX2__
    if (steps >= 8) {
        steps = 8;

        const __m256i mask6bits = _mm256_set1_epi32(0x3F);
        const __m256i shuffle_mask = _mm256_set_epi8(
            -1,-1,-1,-1,-1,-1,11,10,9,8,7,6,5,4,3,2,
            -1,-1,-1,-1,-1,-1,11,10,9,8,7,6,5,4,3,2
        );

        const __m256i permute_mask = _mm256_set_epi32(0,0,0,0,6,4,2,0);

        for (; i + 31 < complete_blocks * 4; i += 32, j += 24) {
            __m256i in = _mm256_loadu_si256((const __m256i*)(encoded_data + i));

            alignas(32) byte chars[32];
            _mm256_store_si256((__m256i*)chars, in);

            alignas(32) uint32 sextets[8];
            for (int32 k = 0; k < 8; ++k) {
                sextets[k] =
                    (BASE64_LOOKUP[(byte)chars[k*4 + 0]] << 18) |
                    (BASE64_LOOKUP[(byte)chars[k*4 + 1]] << 12) |
                    (BASE64_LOOKUP[(byte)chars[k*4 + 2]] << 6) |
                    (BASE64_LOOKUP[(byte)chars[k*4 + 3]]);
            }

            __m256i sextet_vec = _mm256_load_si256((const __m256i*)sextets);

            __m256i bytes0 = _mm256_srli_epi32(sextet_vec, 16);
            __m256i bytes1 = _mm256_srli_epi32(sextet_vec, 8);
            __m256i bytes2 = sextet_vec;

            bytes0 = _mm256_and_si256(bytes0, _mm256_set1_epi32(0xFF));
            bytes1 = _mm256_and_si256(bytes1, _mm256_set1_epi32(0xFF));
            bytes2 = _mm256_and_si256(bytes2, _mm256_set1_epi32(0xFF));

            __m256i packed = _mm256_shuffle_epi8(_mm256_packus_epi32(bytes0, bytes1), shuffle_mask);
            packed = _mm256_permutevar8x32_epi32(packed, permute_mask);

            _mm256_storeu_si256((__m256i*)(data + j), packed);
        }

        steps = 1;
    }
    #endif

    #ifdef __SSE4_2__
    if (steps >= 4) {
        steps = 4;

        const __m128i mask6bits = _mm_set1_epi32(0x3F);
        const __m128i shuffle_mask = _mm_set_epi8(
            -1,-1,-1,-1,-1,-1,11,10,9,8,7,6,5,4,3,2
        );

        for (; i + 15 < complete_blocks * 4; i += 16, j += 12) {
            __m128i in = _mm_loadu_si128((const __m128i*)(encoded_data + i));

            alignas(16) byte chars[16];
            _mm_store_si128((__m128i*)chars, in);

            alignas(16) uint32 sextets[4];
            for (int32 k = 0; k < 4; ++k) {
                sextets[k] =
                    (BASE64_LOOKUP[(byte)chars[k*4 + 0]] << 18) |
                    (BASE64_LOOKUP[(byte)chars[k*4 + 1]] << 12) |
                    (BASE64_LOOKUP[(byte)chars[k*4 + 2]] << 6) |
                    (BASE64_LOOKUP[(byte)chars[k*4 + 3]]);
            }

            __m128i sextet_vec = _mm_load_si128((const __m128i*)sextets);

            __m128i bytes0 = _mm_srli_epi32(sextet_vec, 16);
            __m128i bytes1 = _mm_srli_epi32(sextet_vec, 8);
            __m128i bytes2 = sextet_vec;

            bytes0 = _mm_and_si128(bytes0, _mm_set1_epi32(0xFF));
            bytes1 = _mm_and_si128(bytes1, _mm_set1_epi32(0xFF));
            bytes2 = _mm_and_si128(bytes2, _mm_set1_epi32(0xFF));

            __m128i packed = _mm_shuffle_epi8(_mm_packus_epi32(bytes0, bytes1), shuffle_mask);

            _mm_storeu_si128((__m128i*)(data + j), packed);
        }
    }
    #endif

    for (; i < complete_blocks * 4; i += 4, j += 3) {
        uint32 sextet_a = BASE64_LOOKUP[(byte) encoded_data[i]];
        uint32 sextet_b = BASE64_LOOKUP[(byte) encoded_data[i + 1]];
        uint32 sextet_c = BASE64_LOOKUP[(byte) encoded_data[i + 2]];
        uint32 sextet_d = BASE64_LOOKUP[(byte) encoded_data[i + 3]];

        uint32 triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        data[j] = (triple >> 16) & 0xFF;
        data[j + 1] = (triple >> 8) & 0xFF;
        data[j + 2] = triple & 0xFF;
    }

    if (padding > 0) {
        uint32 sextet_a = BASE64_LOOKUP[(byte) encoded_data[i]];
        uint32 sextet_b = BASE64_LOOKUP[(byte) encoded_data[i + 1]];
        uint32 sextet_c = (padding > 1) ? 0 : BASE64_LOOKUP[(byte) encoded_data[i + 2]];

        uint32 triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6);

        data[j] = (triple >> 16) & 0xFF;
        if (padding == 1) {
            data[j + 1] = (triple >> 8) & 0xFF;
        }
    }

    return output_length;
}

#endif