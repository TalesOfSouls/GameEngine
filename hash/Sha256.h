/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_HASH_SHA256_H
#define COMS_HASH_SHA256_H

#include "../stdlib/Stdlib.h"
#include "Sha256Constants.h"

/**
 * SHA-256 (FIPS 180-4)
 *
 * Used for authentication tokens, HMAC signatures and integrity checks.
 * The implementation is portable (no SIMD requirement) so servers and clients
 * on every architecture produce identical results.
 */

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

struct Sha256Context {
    uint32 state[8];
    uint64 count; // amount of processed bytes
    byte buffer[SHA256_BLOCK_SIZE];
};

/**
 * Rotate a 32 bit value to the right
 */
// @performance SWAP_ENDIAN_32 based rotation is faster on some architectures (ror instruction)
FORCE_INLINE
uint32 sha256_rotr(uint32 value, int32 bits) NO_EXCEPT
{
    return (value >> bits) | (value << (32 - bits));
}

FORCE_INLINE
uint32 sha256_ch(uint32 x, uint32 y, uint32 z) NO_EXCEPT
{
    return (x & y) ^ (~x & z);
}

FORCE_INLINE
uint32 sha256_maj(uint32 x, uint32 y, uint32 z) NO_EXCEPT
{
    return (x & y) ^ (x & z) ^ (y & z);
}

FORCE_INLINE
uint32 sha256_big_sigma0(uint32 x) NO_EXCEPT
{
    return sha256_rotr(x, 2) ^ sha256_rotr(x, 13) ^ sha256_rotr(x, 22);
}

FORCE_INLINE
uint32 sha256_big_sigma1(uint32 x) NO_EXCEPT
{
    return sha256_rotr(x, 6) ^ sha256_rotr(x, 11) ^ sha256_rotr(x, 25);
}

FORCE_INLINE
uint32 sha256_small_sigma0(uint32 x) NO_EXCEPT
{
    return sha256_rotr(x, 7) ^ sha256_rotr(x, 18) ^ (x >> 3);
}

FORCE_INLINE
uint32 sha256_small_sigma1(uint32 x) NO_EXCEPT
{
    return sha256_rotr(x, 17) ^ sha256_rotr(x, 19) ^ (x >> 10);
}

/**
 * Process a single 64 byte block
 *
 * @param Sha256Context* ctx  Hash context
 * @param const byte*    data Block to process
 */
static inline
void sha256_transform(Sha256Context* const ctx, const byte* const data) NO_EXCEPT
{
    uint32 w[64];

    for (int32 i = 0; i < 16; ++i) {
        w[i] = ((uint32) data[i * 4] << 24)
            | ((uint32) data[i * 4 + 1] << 16)
            | ((uint32) data[i * 4 + 2] << 8)
            | ((uint32) data[i * 4 + 3]);
    }

    for (int32 i = 16; i < 64; ++i) {
        w[i] = sha256_small_sigma1(w[i - 2])
            + w[i - 7]
            + sha256_small_sigma0(w[i - 15])
            + w[i - 16];
    }

    uint32 a = ctx->state[0];
    uint32 b = ctx->state[1];
    uint32 c = ctx->state[2];
    uint32 d = ctx->state[3];
    uint32 e = ctx->state[4];
    uint32 f = ctx->state[5];
    uint32 g = ctx->state[6];
    uint32 h = ctx->state[7];

    for (int32 i = 0; i < 64; ++i) {
        const uint32 temp1 = h + sha256_big_sigma1(e) + sha256_ch(e, f, g) + SHA256_K[i] + w[i];
        const uint32 temp2 = sha256_big_sigma0(a) + sha256_maj(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

/**
 * Initialize a hash context
 *
 * @param Sha256Context* ctx Hash context
 */
static inline
void sha256_init(Sha256Context* const ctx) NO_EXCEPT
{
    for (int32 i = 0; i < 8; ++i) {
        ctx->state[i] = SHA256_H_INIT[i];
    }

    ctx->count = 0;
}

/**
 * Add data to the hash
 *
 * @param Sha256Context* ctx    Hash context
 * @param const byte*    data   Data to add
 * @param size_t         length Amount of bytes to add
 */
static inline
void sha256_update(Sha256Context* const ctx, const byte* const data, size_t length) NO_EXCEPT
{
    size_t index = (size_t) (ctx->count & (SHA256_BLOCK_SIZE - 1));
    ctx->count += length;

    size_t offset = 0;

    if (index) {
        const size_t available = SHA256_BLOCK_SIZE - index;

        if (length < available) {
            memcpy(ctx->buffer + index, data, length);
            return;
        }

        memcpy(ctx->buffer + index, data, available);
        sha256_transform(ctx, ctx->buffer);

        offset = available;
    }

    for (; offset + SHA256_BLOCK_SIZE <= length; offset += SHA256_BLOCK_SIZE) {
        sha256_transform(ctx, data + offset);
    }

    if (offset < length) {
        memcpy(ctx->buffer, data + offset, length - offset);
    }
}

/**
 * Finalize the hash
 *
 * @param Sha256Context* ctx    Hash context
 * @param byte*          digest Output buffer (SHA256_DIGEST_SIZE bytes)
 */
static inline
void sha256_final(Sha256Context* const ctx, byte* const digest) NO_EXCEPT
{
    const uint64 bit_count = ctx->count * 8;
    size_t index = (size_t) (ctx->count & (SHA256_BLOCK_SIZE - 1));

    const byte first_padding = 0x80;
    sha256_update(ctx, &first_padding, 1);

    index = (size_t) (ctx->count & (SHA256_BLOCK_SIZE - 1));

    if (index > 56) {
        const byte zeros[SHA256_BLOCK_SIZE] = {0};
        sha256_update(ctx, zeros, SHA256_BLOCK_SIZE - index);

        index = 0;
    }

    // Pad up to the length field
    const byte zeros[SHA256_BLOCK_SIZE] = {0};
    sha256_update(ctx, zeros, 56 - index);

    byte length_bytes[8];
    for (int32 i = 0; i < 8; ++i) {
        length_bytes[i] = (byte) (bit_count >> ((7 - i) * 8));
    }

    sha256_update(ctx, length_bytes, sizeof(length_bytes));

    for (int32 i = 0; i < 8; ++i) {
        digest[i * 4] = (byte) (ctx->state[i] >> 24);
        digest[i * 4 + 1] = (byte) (ctx->state[i] >> 16);
        digest[i * 4 + 2] = (byte) (ctx->state[i] >> 8);
        digest[i * 4 + 3] = (byte) (ctx->state[i]);
    }
}

/**
 * Hash a memory buffer in one call
 *
 * @param const byte* data   Data to hash
 * @param size_t      length Amount of bytes to hash
 * @param byte*       digest Output buffer (SHA256_DIGEST_SIZE bytes)
 */
inline
void sha256_hash(const byte* const data, size_t length, byte* const digest) NO_EXCEPT
{
    Sha256Context ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, length);
    sha256_final(&ctx, digest);
}

#endif
