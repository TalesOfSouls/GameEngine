/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HASH_SHA1_SIMD_ARM_H
#define COMS_HASH_SHA1_SIMD_ARM_H

#include <immintrin.h>
#include "../stdlib/Types.h"
#include "Sha1Definitions.h"

#ifdef __ARM_FEATURE_SVE
    #include <arm_sve.h>

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

        svuint32_t sv_k3 = svdup_n_u32(3);
        svuint32_t sv_k8 = svdup_n_u32(8);
        svuint32_t sv_k14 = svdup_n_u32(14);
        svuint32_t sv_k16 = svdup_n_u32(16);

        for (int32 i = 16; i < 80; i += steps) {
            svbool_t pg = svwhilelt_b32(i, 80);

            // Calculate indices
            svuint32_t idx_3 = svsub_n_u32_z(pg, svindex_u32(i, 1), 3);
            svuint32_t idx_8 = svsub_n_u32_z(pg, svindex_u32(i, 1), 8);
            svuint32_t idx_14 = svsub_n_u32_z(pg, svindex_u32(i, 1), 14);
            svuint32_t idx_16 = svsub_n_u32_z(pg, svindex_u32(i, 1), 16);

            // Gather values
            svuint32_t v_3 = svld1_gather_index_u32(pg, w, idx_3);
            svuint32_t v_8 = svld1_gather_index_u32(pg, w, idx_8);
            svuint32_t v_14 = svld1_gather_index_u32(pg, w, idx_14);
            svuint32_t v_16 = svld1_gather_index_u32(pg, w, idx_16);

            // Compute w[i] = (w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]) <<< 1
            svuint32_t v = sveor_u32_z(pg, v_3, v_8);
            v = sveor_u32_z(pg, v, v_14);
            v = sveor_u32_z(pg, v, v_16);
            v = svorr_u32_z(pg, svlsl_n_u32_z(pg, v, 1), svlsr_n_u32_z(pg, v, 31));

            svst1_u32(pg, &w[i], v);
        }

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
#elif defined (__ARM_NEON)
    #include <arm_neon.h>

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

        for (int32 i = 16; i < 80; i += 4) {
            uint32x4_t v = veorq_u32(
                vld1q_u32(&w[i-3]),
                vld1q_u32(&w[i-8])
            );
            v = veorq_u32(v, vld1q_u32(&w[i-14]));
            v = veorq_u32(v, vld1q_u32(&w[i-16]));

            // Rotate left by 1
            uint32x4_t v_rot = vorrq_u32(
                vshlq_n_u32(v, 1),
                vshrq_n_u32(v, 31)
            );

            vst1q_u32(&w[i], v_rot);
        }

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

#endif