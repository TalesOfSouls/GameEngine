/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ENCODING_BASE64_SIMD_ARM_H
#define COMS_ENCODING_BASE64_SIMD_ARM_H

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "Base64Definitions.h"

#ifdef __ARM_FEATURE_SVE
    #include <arm_sve.h>
#endif

#ifdef __ARM_NEON
    #include <arm_neon.h>
#endif

void base64_encode_simd(const byte* __restrict data, char* __restrict encoded_data, size_t data_length = 0, int32 steps = 16) NO_EXCEPT
{
    if (!data_length) {
        // WARNING: This should only happen if the data is a char string
        // Binary data is not allowed since it often has '\0' characters
        data_length = str_length((const char *) data);
    }

    steps = intrin_validate_steps(data, steps);
    steps = intrin_validate_steps((const byte*) encoded_data, steps);

    size_t i = 0;

    #ifdef __ARM_FEATURE_SVE
    if (steps >= 8) {
        const size_t sve_vec_bytes = steps;
        const size_t triple_per_vec = sve_vec_bytes / 3;

        for (; i + (triple_per_vec * 3 - 1) < data_length; i += triple_per_vec * 3) {
            svuint8_t in = svld1_u8(svptrue_b8(), data + i);

            svuint32_t triple0 = svreinterpret_u32(svld1_u8(svptrue_b8(), data + i));
            svuint32_t triple1 = svreinterpret_u32(svld1_u8(svptrue_b8(), data + i + 4));
            svuint32_t triple2 = svreinterpret_u32(svld1_u8(svptrue_b8(), data + i + 8));

            svuint32_t combined = svorr_u32_x(svptrue_b32(),
                svlsl_n_u32_x(svptrue_b32(), triple0, 16),
                svorr_u32_x(svptrue_b32(),
                    svlsl_n_u32_x(svptrue_b32(), triple1, 8),
                    triple2));

            svuint32_t idx0 = svand_u32_x(svptrue_b32(), svlsr_n_u32_x(svptrue_b32(), combined, 18), 0x3F);
            svuint32_t idx1 = svand_u32_x(svptrue_b32(), svlsr_n_u32_x(svptrue_b32(), combined, 12), 0x3F);
            svuint32_t idx2 = svand_u32_x(svptrue_b32(), svlsr_n_u32_x(svptrue_b32(), combined, 6), 0x3F);
            svuint32_t idx3 = svand_u32_x(svptrue_b32(), combined, 0x3F);

            svuint8_t chars0 = svld1_u8(svptrue_b8(), (const byte*)BASE64_CHARS);
            svuint8_t enc0 = svtbl_u8(chars0, svreinterpret_u8_u32(idx0));
            svuint8_t enc1 = svtbl_u8(chars0, svreinterpret_u8_u32(idx1));
            svuint8_t enc2 = svtbl_u8(chars0, svreinterpret_u8_u32(idx2));
            svuint8_t enc3 = svtbl_u8(chars0, svreinterpret_u8_u32(idx3));

            svuint8_t encoded = svzip1_u8(svzip1_u8(enc0, enc1), svzip1_u8(enc2, enc3));

            svst1_u8(svptrue_b8(), (uint8_t*)(encoded_data + (i/3*4)), encoded);
        }

        steps = 1;
    }
    #endif

    #ifdef __ARM_NEON
    if (steps >= 4) {
        steps = 4;

        // 48 input bytes -> 64 characters per iteration
        //      vld3q de-interleaves the bytes of the triples (in.val[0] = 1st byte of 16 triples, ...)
        //      vqtbl4q looks up all 64 characters (4 * 16 byte table), vst4q interleaves the 4 characters again
        uint8x16x4_t table;
        table.val[0] = vld1q_u8((const uint8 *) BASE64_CHARS);
        table.val[1] = vld1q_u8((const uint8 *) BASE64_CHARS + 16);
        table.val[2] = vld1q_u8((const uint8 *) BASE64_CHARS + 32);
        table.val[3] = vld1q_u8((const uint8 *) BASE64_CHARS + 48);

        const uint8x16_t mask_3F = vdupq_n_u8(0x3F);

        for (; i + 48 <= data_length; i += 48) {
            const uint8x16x3_t in = vld3q_u8(data + i);

            const uint8x16_t idx0 = vshrq_n_u8(in.val[0], 2);
            const uint8x16_t idx1 = vandq_u8(vorrq_u8(vshlq_n_u8(in.val[0], 4), vshrq_n_u8(in.val[1], 4)), mask_3F);
            const uint8x16_t idx2 = vandq_u8(vorrq_u8(vshlq_n_u8(in.val[1], 2), vshrq_n_u8(in.val[2], 6)), mask_3F);
            const uint8x16_t idx3 = vandq_u8(in.val[2], mask_3F);

            uint8x16x4_t out;
            out.val[0] = vqtbl4q_u8(table, idx0);
            out.val[1] = vqtbl4q_u8(table, idx1);
            out.val[2] = vqtbl4q_u8(table, idx2);
            out.val[3] = vqtbl4q_u8(table, idx3);

            vst4q_u8((uint8 *) (encoded_data + (i / 3 * 4)), out);
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

size_t base64_decode_simd(const char* __restrict encoded_data, byte* __restrict data, size_t encoded_length = 0, int32 steps = 16) NO_EXCEPT
{
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

    steps = intrin_validate_steps((const byte*) encoded_data, steps);
    steps = intrin_validate_steps(data, steps);

    size_t i = 0;
    size_t j = 0;

    #ifdef __ARM_FEATURE_SVE
    if (steps >= 8) {
        const size_t sve_vec_bytes = steps;
        const size_t quad_per_vec = sve_vec_bytes / 4;

        for (; i + (quad_per_vec * 4 - 1) < complete_blocks * 4; i += quad_per_vec * 4, j += quad_per_vec * 3) {

            svuint8_t in = svld1_u8(svptrue_b8(), (const byte*)(encoded_data + i));

            byte chars[sve_vec_bytes];
            svst1_u8(svptrue_b8(), chars, in);

            uint32 sextets[quad_per_vec];
            for (size_t k = 0; k < quad_per_vec; ++k) {
                sextets[k] =
                    (BASE64_LOOKUP[chars[k*4 + 0]] << 18) |
                    (BASE64_LOOKUP[chars[k*4 + 1]] << 12) |
                    (BASE64_LOOKUP[chars[k*4 + 2]] << 6) |
                    (BASE64_LOOKUP[chars[k*4 + 3]]);
            }

            byte output_bytes[quad_per_vec * 3];
            for (size_t k = 0; k < quad_per_vec; ++k) {
                output_bytes[k*3 + 0] = (sextets[k] >> 16) & 0xFF;
                output_bytes[k*3 + 1] = (sextets[k] >> 8) & 0xFF;
                output_bytes[k*3 + 2] = sextets[k] & 0xFF;
            }


            svst1_u8(svptrue_b8(), data + j, svld1_u8(svptrue_b8(), output_bytes));
        }

        steps = 1;
    }
    #endif

    #ifdef __ARM_NEON
    if (steps >= 4) {
        steps = 4;

        // 64 characters -> 48 bytes per iteration
        //      vld4q de-interleaves the characters of the quads, the lookup table has 128 entries (ASCII):
        //      vqtbl4q = entries 0 - 63, vqtbx4q = entries 64 - 127 (index - 64, keeps the first result if out of range)
        uint8x16x4_t lookup_lo;
        uint8x16x4_t lookup_hi;
        for (int32 k = 0; k < 4; ++k) {
            lookup_lo.val[k] = vld1q_u8((const uint8 *) BASE64_LOOKUP + 16 * k);
            lookup_hi.val[k] = vld1q_u8((const uint8 *) BASE64_LOOKUP + 64 + 16 * k);
        }

        const uint8x16_t offset_64 = vdupq_n_u8(64);

        for (; i + 64 <= complete_blocks * 4; i += 64, j += 48) {
            const uint8x16x4_t in = vld4q_u8((const uint8 *) (encoded_data + i));

            const uint8x16_t s0 = vqtbx4q_u8(vqtbl4q_u8(lookup_lo, in.val[0]), lookup_hi, vsubq_u8(in.val[0], offset_64));
            const uint8x16_t s1 = vqtbx4q_u8(vqtbl4q_u8(lookup_lo, in.val[1]), lookup_hi, vsubq_u8(in.val[1], offset_64));
            const uint8x16_t s2 = vqtbx4q_u8(vqtbl4q_u8(lookup_lo, in.val[2]), lookup_hi, vsubq_u8(in.val[2], offset_64));
            const uint8x16_t s3 = vqtbx4q_u8(vqtbl4q_u8(lookup_lo, in.val[3]), lookup_hi, vsubq_u8(in.val[3], offset_64));

            uint8x16x3_t out;
            out.val[0] = vorrq_u8(vshlq_n_u8(s0, 2), vshrq_n_u8(s1, 4));
            out.val[1] = vorrq_u8(vshlq_n_u8(s1, 4), vshrq_n_u8(s2, 2));
            out.val[2] = vorrq_u8(vshlq_n_u8(s2, 6), s3);

            vst3q_u8(data + j, out);
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