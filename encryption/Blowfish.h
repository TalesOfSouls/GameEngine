/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ENCRYPTION_BLOWFISH_H
#define COMS_ENCRYPTION_BLOWFISH_H

#include "../stdlib/Stdlib.h"
#include "BlowfishConstants.h"

/**
 * Blowfish block cipher (Bruce Schneier, 1993)
 *
 * This is the plain Blowfish cipher. bcrypt uses the same round function but
 * with the expensive "EksBlowfish" key schedule which is implemented in
 * ../auth/Bcrypt.h on top of the primitives from this file.
 *
 * Both the cipher and the key schedule operate on 64 bit blocks which are
 * passed as two 32 bit words. All functions are endian independent since the
 * words are assembled from bytes explicitly.
 */

#define BLOWFISH_P_COUNT 18
#define BLOWFISH_S_COUNT 4
#define BLOWFISH_S_BOX_SIZE 256
#define BLOWFISH_BLOCK_SIZE 8

struct BlowfishContext {
    uint32 p[BLOWFISH_P_COUNT];
    uint32 s[BLOWFISH_S_COUNT][BLOWFISH_S_BOX_SIZE];
};

/**
 * Reset the context to the initial pi based state
 *
 * @param BlowfishContext* ctx Context to reset
 */
inline
void blowfish_init_state(BlowfishContext* const ctx) NO_EXCEPT
{
    for (int32 i = 0; i < BLOWFISH_P_COUNT; ++i) {
        ctx->p[i] = BLOWFISH_P_INIT[i];
    }

    for (int32 i = 0; i < BLOWFISH_S_COUNT; ++i) {
        for (int32 j = 0; j < BLOWFISH_S_BOX_SIZE; ++j) {
            ctx->s[i][j] = BLOWFISH_S_INIT[i][j];
        }
    }
}

/**
 * Blowfish round function
 *
 * @param const BlowfishContext* ctx Context
 * @param uint32                 x   Input word
 *
 * @return uint32 Transformed word
 */
FORCE_INLINE
uint32 blowfish_f(const BlowfishContext* const __restrict ctx, uint32 x) NO_EXCEPT
{
    return ((ctx->s[0][(x >> 24) & 0xFF] + ctx->s[1][(x >> 16) & 0xFF])
        ^ ctx->s[2][(x >> 8) & 0xFF]) + ctx->s[3][x & 0xFF];
}

/**
 * Encrypt a single block
 *
 * @param const BlowfishContext* ctx Context
 * @param uint32*                l   Left word
 * @param uint32*                r   Right word
 */
inline
void blowfish_encipher(const BlowfishContext* const __restrict ctx, uint32* const l, uint32* const r) NO_EXCEPT
{
    uint32 xl = *l ^ ctx->p[0];
    uint32 xr = *r;

    for (int32 n = 1; n <= 16; ++n) {
        if (n & 1) {
            xr ^= blowfish_f(ctx, xl) ^ ctx->p[n];
        } else {
            xl ^= blowfish_f(ctx, xr) ^ ctx->p[n];
        }
    }

    *l = xr ^ ctx->p[17];
    *r = xl;
}

/**
 * Decrypt a single block
 *
 * @param const BlowfishContext* ctx Context
 * @param uint32*                l   Left word
 * @param uint32*                r   Right word
 */
inline
void blowfish_decipher(const BlowfishContext* const __restrict ctx, uint32* const l, uint32* const r) NO_EXCEPT
{
    uint32 a = *r;
    uint32 b = *l ^ ctx->p[17];

    for (int32 n = 16; n >= 1; --n) {
        if (n & 1) {
            b ^= blowfish_f(ctx, a) ^ ctx->p[n];
        } else {
            a ^= blowfish_f(ctx, b) ^ ctx->p[n];
        }
    }

    *l = a ^ ctx->p[0];
    *r = b;
}

/**
 * Read the next 32 bit word from a byte stream
 *
 * The index wraps around which is how Blowfish/bcrypt cycle through keys and
 * salts that are shorter than the state they are expanded into.
 *
 * @param const byte* data   Byte stream
 * @param int32       length Stream length (must be > 0)
 * @param int32*      index  Current position, updated to the next position
 *
 * @return uint32 Next word (big endian)
 */
FORCE_INLINE
uint32 blowfish_stream2word(
    const byte* const __restrict data,
    int32 length,
    int32* const __restrict index
) NO_EXCEPT
{
    ASSERT_TRUE(length > 0);

    uint32 word = 0;

    for (int32 i = 0; i < 4; ++i) {
        word = (word << 8) | data[*index];
        *index = (*index + 1) % length;
    }

    return word;
}

/**
 * Standard Blowfish key schedule (without salt)
 *
 * @param BlowfishContext* ctx      Context
 * @param const byte*      key      Key
 * @param int32            key_size Key size in bytes
 */
inline
void blowfish_expand_key(
    BlowfishContext* const __restrict ctx,
    const byte* const __restrict key,
    int32 key_size
) NO_EXCEPT
{
    ASSERT_TRUE(key_size > 0);

    int32 key_index = 0;

    for (int32 i = 0; i < BLOWFISH_P_COUNT; ++i) {
        ctx->p[i] ^= blowfish_stream2word(key, key_size, &key_index);
    }

    uint32 l = 0;
    uint32 r = 0;

    for (int32 i = 0; i < BLOWFISH_P_COUNT; i += 2) {
        blowfish_encipher(ctx, &l, &r);

        ctx->p[i] = l;
        ctx->p[i + 1] = r;
    }

    for (int32 i = 0; i < BLOWFISH_S_COUNT; ++i) {
        for (int32 j = 0; j < BLOWFISH_S_BOX_SIZE; j += 2) {
            blowfish_encipher(ctx, &l, &r);

            ctx->s[i][j] = l;
            ctx->s[i][j + 1] = r;
        }
    }
}

/**
 * EksBlowfish key schedule (salted expansion)
 *
 * This is the "expensive key schedule" variant used by bcrypt. It is the same
 * as blowfish_expand_key but the salt is XORed into the running block before
 * every encryption.
 *
 * @param BlowfishContext* ctx      Context
 * @param const byte*      salt     Salt (may be NULL to skip salt mixing)
 * @param int32            salt_size Salt size in bytes
 * @param const byte*      key      Key
 * @param int32            key_size Key size in bytes
 */
inline
void blowfish_expand_state(
    BlowfishContext* const __restrict ctx,
    const byte* const __restrict salt, int32 salt_size,
    const byte* const __restrict key, int32 key_size
) NO_EXCEPT
{
    ASSERT_TRUE(key_size > 0);

    int32 key_index = 0;
    int32 salt_index = 0;

    for (int32 i = 0; i < BLOWFISH_P_COUNT; ++i) {
        ctx->p[i] ^= blowfish_stream2word(key, key_size, &key_index);
    }

    uint32 l = 0;
    uint32 r = 0;

    for (int32 i = 0; i < BLOWFISH_P_COUNT; i += 2) {
        if (salt) {
            l ^= blowfish_stream2word(salt, salt_size, &salt_index);
            r ^= blowfish_stream2word(salt, salt_size, &salt_index);
        }

        blowfish_encipher(ctx, &l, &r);

        ctx->p[i] = l;
        ctx->p[i + 1] = r;
    }

    for (int32 i = 0; i < BLOWFISH_S_COUNT; ++i) {
        for (int32 j = 0; j < BLOWFISH_S_BOX_SIZE; j += 2) {
            if (salt) {
                l ^= blowfish_stream2word(salt, salt_size, &salt_index);
                r ^= blowfish_stream2word(salt, salt_size, &salt_index);
            }

            blowfish_encipher(ctx, &l, &r);

            ctx->s[i][j] = l;
            ctx->s[i][j + 1] = r;
        }
    }
}

#endif
