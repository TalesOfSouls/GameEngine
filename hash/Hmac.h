/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_HASH_HMAC_H
#define COMS_HASH_HMAC_H

#include "../stdlib/Stdlib.h"
#include "../utils/ConstantTime.h"
#include "Sha256.h"

/**
 * HMAC-SHA256 (RFC 2104)
 *
 * Used to sign authentication tokens and server-to-server messages.
 * The streaming variant allows signing a packet header and payload without
 * concatenating them into a temporary buffer first.
 */

#define HMAC_SHA256_DIGEST_SIZE SHA256_DIGEST_SIZE
#define HMAC_SHA256_BLOCK_SIZE SHA256_BLOCK_SIZE

struct HmacSha256Context {
    Sha256Context inner;
    Sha256Context outer;
};

/**
 * Initialize an HMAC context
 *
 * Keys longer than the block size are hashed first as required by RFC 2104.
 *
 * @param HmacSha256Context* ctx      Context to initialize
 * @param const byte*        key      Secret key
 * @param size_t             key_size Key size in bytes
 */
inline
void hmac_sha256_init(
    HmacSha256Context* const ctx,
    const byte* const __restrict key,
    size_t key_size
) NO_EXCEPT
{
    byte key_block[HMAC_SHA256_BLOCK_SIZE];
    memset(key_block, 0, sizeof(key_block));

    if (key_size > HMAC_SHA256_BLOCK_SIZE) {
        sha256_hash(key, key_size, key_block);
    } else if (key_size) {
        memcpy(key_block, key, key_size);
    }

    byte ipad[HMAC_SHA256_BLOCK_SIZE];
    byte opad[HMAC_SHA256_BLOCK_SIZE];

    for (int32 i = 0; i < HMAC_SHA256_BLOCK_SIZE; ++i) {
        ipad[i] = (byte) (key_block[i] ^ 0x36);
        opad[i] = (byte) (key_block[i] ^ 0x5C);
    }

    sha256_init(&ctx->inner);
    sha256_update(&ctx->inner, ipad, sizeof(ipad));

    sha256_init(&ctx->outer);
    sha256_update(&ctx->outer, opad, sizeof(opad));

    ct_memzero(key_block, sizeof(key_block));
    ct_memzero(ipad, sizeof(ipad));
    ct_memzero(opad, sizeof(opad));
}

/**
 * Add data to the HMAC
 *
 * @param HmacSha256Context* ctx    Context
 * @param const void*        data   Data
 * @param size_t             length Amount of bytes
 */
inline
void hmac_sha256_update(HmacSha256Context* const ctx, const void* const data, size_t length) NO_EXCEPT
{
    sha256_update(&ctx->inner, (const byte *) data, length);
}

/**
 * Finalize the HMAC
 *
 * @param HmacSha256Context* ctx    Context
 * @param byte*              digest Output buffer (HMAC_SHA256_DIGEST_SIZE bytes)
 */
inline
void hmac_sha256_final(HmacSha256Context* const ctx, byte* const digest) NO_EXCEPT
{
    byte inner_digest[SHA256_DIGEST_SIZE];
    sha256_final(&ctx->inner, inner_digest);

    sha256_update(&ctx->outer, inner_digest, sizeof(inner_digest));
    sha256_final(&ctx->outer, digest);

    ct_memzero(inner_digest, sizeof(inner_digest));
}

/**
 * Compute an HMAC-SHA256 in one call
 *
 * @param const byte* key      Secret key
 * @param size_t      key_size Key size in bytes
 * @param const void* data     Data to sign
 * @param size_t      length   Data size in bytes
 * @param byte*       digest   Output buffer (HMAC_SHA256_DIGEST_SIZE bytes)
 */
inline
void hmac_sha256(
    const byte* const __restrict key,
    size_t key_size,
    const void* const __restrict data,
    size_t length,
    byte* const __restrict digest
) NO_EXCEPT
{
    HmacSha256Context ctx;
    hmac_sha256_init(&ctx, key, key_size);
    hmac_sha256_update(&ctx, data, length);
    hmac_sha256_final(&ctx, digest);
}

/**
 * Verify an HMAC-SHA256 in constant time
 *
 * @param const byte* key      Secret key
 * @param size_t      key_size Key size in bytes
 * @param const void* data     Signed data
 * @param size_t      length   Data size in bytes
 * @param const byte* expected Expected signature
 *
 * @return bool True if the signature matches
 */
inline
bool hmac_sha256_verify(
    const byte* const __restrict key,
    size_t key_size,
    const void* const __restrict data,
    size_t length,
    const byte* const __restrict expected
) NO_EXCEPT
{
    byte digest[HMAC_SHA256_DIGEST_SIZE];
    hmac_sha256(key, key_size, data, length, digest);

    const bool valid = ct_memcmp(digest, expected, sizeof(digest));
    ct_memzero(digest, sizeof(digest));

    return valid;
}

#endif
