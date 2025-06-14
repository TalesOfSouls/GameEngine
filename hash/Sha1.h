/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HASH_SHA1_H
#define COMS_HASH_SHA1_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../stdlib/Types.h"
#include "../stdlib/Simd.h"
#include "Sha1Definitions.h"

#ifdef __SSE4_2__
    #include "Sha1SimdX86.h"
#elif defined(__ARM_FEATURE_SVE) || defined(__ARM_NEON)
    #include "Sha1SimdArm.h"
#else
    static
    void sha1_transform(SHA1_CTX *ctx, const byte data[64], [[mayb_unused]] int32 steps) {
        uint32 a, b, c, d, e, temp;
        uint32 w[80];

        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32)data[i * 4 + 0] << 24) |
                ((uint32)data[i * 4 + 1] << 16) |
                ((uint32)data[i * 4 + 2] <<  8) |
                ((uint32)data[i * 4 + 3]);
        }

        for (int i = 16; i < 80; i++) {
            temp = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
            w[i] = (temp << 1) | (temp >> 31);
        }

        a = ctx->state[0];
        b = ctx->state[1];
        c = ctx->state[2];
        d = ctx->state[3];
        e = ctx->state[4];

        // Main compression loop - unrolled for better performance
        // Round 1 (0-19)
        for (int i = 0; i < 20; i++) {
            temp = SHA1_ROTL32(a, 5) + SHA1_Ch(b, c, d) + e + K1 + w[i];
            e = d;
            d = c;
            c = SHA1_ROTL32(b, 30);
            b = a;
            a = temp;
        }

        // Round 2 (20-39)
        for (int i = 20; i < 40; i++) {
            temp = SHA1_ROTL32(a, 5) + SHA1_Parity(b, c, d) + e + K2 + w[i];
            e = d;
            d = c;
            c = SHA1_ROTL32(b, 30);
            b = a;
            a = temp;
        }

        // Round 3 (40-59)
        for (int i = 40; i < 60; i++) {
            temp = SHA1_ROTL32(a, 5) + SHA1_Maj(b, c, d) + e + K3 + w[i];
            e = d;
            d = c;
            c = SHA1_ROTL32(b, 30);
            b = a;
            a = temp;
        }

        // Round 4 (60-79)
        for (int i = 60; i < 80; i++) {
            temp = SHA1_ROTL32(a, 5) + SHA1_Parity(b, c, d) + e + K4 + w[i];
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

void sha1_init(SHA1_CTX* ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count = 0;
}

static
void sha1_update(SHA1_CTX* ctx, const byte* data, size_t len, int32 steps) {
    size_t i, index, part_len;

    index = (size_t)((ctx->count >> 3) & 0x3F);
    ctx->count += len << 3;
    part_len = 64 - index;

    if (len >= part_len) {
        memcpy(&ctx->buffer[index], data, part_len);
        sha1_transform(ctx, ctx->buffer, steps);

        for (i = part_len; i + 63 < len; i += 64) {
            sha1_transform(ctx, &data[i], steps);
        }

        index = 0;
    } else {
        i = 0;
    }

    memcpy(&ctx->buffer[index], &data[i], len - i);
}

static
void sha1_final(SHA1_CTX* ctx, byte digest[20], int32 steps) {
    byte bits[8];
    uint32 index, pad_len;

    for (int32 i = 0; i < 8; i++) {
        bits[i] = (byte) ((ctx->count >> ((7 - i) * 8)) & 0xFF);
    }

    index = (uint32)((ctx->count >> 3) & 0x3F);
    pad_len = (index < 56) ? (56 - index) : (120 - index);
    sha1_update(ctx, (const byte*) "\x80", 1, steps);
    while (pad_len-- > 1) {
        sha1_update(ctx, (const byte*) "\0", 1, steps);
    }

    sha1_update(ctx, bits, 8, steps);

    for (int32 i = 0; i < 5; i++) {
        digest[i*4+0] = (byte) ((ctx->state[i] >> 24) & 0xFF);
        digest[i*4+1] = (byte) ((ctx->state[i] >> 16) & 0xFF);
        digest[i*4+2] = (byte) ((ctx->state[i] >>  8) & 0xFF);
        digest[i*4+3] = (byte) ( ctx->state[i]        & 0xFF);
    }

    // WARNING: We are not doing this since no sensitive data should use sha1 anyways
    // Normally this would be done to reduce side channel attacks
    // memset(ctx, 0, sizeof(*ctx));
}

inline
void sha1_hash(const byte* data, size_t len, byte digest[20], int32 steps = 16) {
    SHA1_CTX ctx;
    sha1_init(&ctx);

    steps = intrin_validate_steps(data, steps);

    sha1_update(&ctx, data, len, steps);
    sha1_final(&ctx, digest, steps);
}

#endif