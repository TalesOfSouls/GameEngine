/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENCODING_BASE64_SIMD_ARM_H
#define COMS_ENCODING_BASE64_SIMD_ARM_H

#include "../stdlib/Types.h"
#include "../stdlib/Simd.h"
#include "../utils/StringUtils.h"
#include "Base64Definitions.h"

#ifdef __ARM_FEATURE_SVE
    #include <arm_sve.h>
#endif

#ifdef __ARM_NEON
    #include <arm_neon.h>
#endif

void base64_encode_simd(const byte* data, char* encoded_data, size_t data_length = 0, int32 steps = 16) {
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

        const uint8x16_t base64_table = vld1q_u8((const byte*)BASE64_CHARS);
        const uint8x16_t mask_3F = vdupq_n_u8(0x3F);

        for (; i + 11 < data_length; i += 12) {
            uint8x16_t in = vld1q_u8(data + i);

            uint32x4_t triple0 = vreinterpretq_u32_u8(vshrq_n_u8(in, 2));
            uint32x4_t triple1 = vreinterpretq_u32_u8(vshrq_n_u8(vextq_u8(in, in, 1), 4));
            uint32x4_t triple2 = vreinterpretq_u32_u8(vshrq_n_u8(vextq_u8(in, in, 2), 6));

            uint32x4_t combined = vorrq_u32(
                vshlq_n_u32(triple0, 16),
                vorrq_u32(
                    vshlq_n_u32(triple1, 8),
                    triple2));

            uint32x4_t idx0 = vandq_u32(vshrq_n_u32(combined, 18), 0x3F);
            uint32x4_t idx1 = vandq_u32(vshrq_n_u32(combined, 12), 0x3F);
            uint32x4_t idx2 = vandq_u32(vshrq_n_u32(combined, 6), 0x3F);
            uint32x4_t idx3 = vandq_u32(combined, 0x3F);

            uint8x16_t enc0 = vqtbl1q_u8(base64_table, vreinterpretq_u8_u32(idx0));
            uint8x16_t enc1 = vqtbl1q_u8(base64_table, vreinterpretq_u8_u32(idx1));
            uint8x16_t enc2 = vqtbl1q_u8(base64_table, vreinterpretq_u8_u32(idx2));
            uint8x16_t enc3 = vqtbl1q_u8(base64_table, vreinterpretq_u8_u32(idx3));

            uint8x16x2_t zip01 = vzipq_u8(enc0, enc1);
            uint8x16x2_t zip23 = vzipq_u8(enc2, enc3);
            uint8x16_t encoded = vcombine_u8(vget_low_u8(zip01.val[0]), vget_low_u8(zip23.val[0]));

            vst1q_u8((uint8_t*)(encoded_data + (i/3*4)), encoded);
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

size_t base64_decode_simd(const char* encoded_data, byte* data, size_t encoded_length = 0, int32 steps = 16) {
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

        const uint8x16_t mask_3F = vdupq_n_u8(0x3F);
        const uint8x16_t shuffle_mask = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2
        };

        for (; i + 15 < complete_blocks * 4; i += 16, j += 12) {

            uint8x16_t in = vld1q_u8((const byte*)(encoded_data + i));

            byte chars[16];
            vst1q_u8(chars, in);

            uint32 sextets[4];
            for (int k = 0; k < 4; ++k) {
                sextets[k] =
                    (BASE64_LOOKUP[chars[k*4 + 0]] << 18) |
                    (BASE64_LOOKUP[chars[k*4 + 1]] << 12) |
                    (BASE64_LOOKUP[chars[k*4 + 2]] << 6) |
                    (BASE64_LOOKUP[chars[k*4 + 3]]);
            }

            uint8x16_t bytes0 = vreinterpretq_u8_u32(vshrq_n_u32(vld1q_u32(sextets), 16));
            uint8x16_t bytes1 = vreinterpretq_u8_u32(vshrq_n_u32(vld1q_u32(sextets), 8));
            uint8x16_t bytes2 = vreinterpretq_u8_u32(vld1q_u32(sextets));

            bytes0 = vandq_u8(bytes0, vdupq_n_u8(0xFF));
            bytes1 = vandq_u8(bytes1, vdupq_n_u8(0xFF));
            bytes2 = vandq_u8(bytes2, vdupq_n_u8(0xFF));


            uint8x16_t packed = vqtbl1q_u8(vcombine_u8(
                vget_low_u8(bytes0), vget_low_u8(bytes1)), shuffle_mask);

            vst1q_u8(data + j, packed);

            vst1_u8(data + j + 8, vget_low_u8(bytes2));
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