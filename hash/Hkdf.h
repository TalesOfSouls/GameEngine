/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_HASH_HKDF_H
#define COMS_HASH_HKDF_H

#include "../stdlib/Stdlib.h"
#include "../utils/ConstantTime.h"
#include "Hmac.h"

/**
 * HKDF-SHA256 (RFC 5869)
 *
 * Derives keys from a shared secret, e.g. the X25519 result of the network
 * handshake. extract() condenses the input into a pseudo random key, expand()
 * creates any amount of key material (max. 255 * 32 bytes) from it.
 */

/**
 * @param const byte* salt          Salt (may be NULL -> 32 zero bytes)
 * @param size_t      salt_size     Salt size
 * @param const byte* ikm           Input key material
 * @param size_t      ikm_size      Input key material size
 * @param byte*       prk           Output pseudo random key (32 bytes)
 */
inline
void hkdf_sha256_extract(
    const byte* const salt, size_t salt_size,
    const byte* const ikm, size_t ikm_size,
    byte* const prk
) NO_EXCEPT
{
    static const byte zero_salt[HMAC_SHA256_DIGEST_SIZE] = {0};

    if (!salt || !salt_size) {
        hmac_sha256(zero_salt, sizeof(zero_salt), ikm, ikm_size, prk);
    } else {
        hmac_sha256(salt, salt_size, ikm, ikm_size, prk);
    }
}

/**
 * @param const byte* prk           Pseudo random key (32 bytes)
 * @param const byte* info          Context information (may be NULL)
 * @param size_t      info_size     Info size
 * @param byte*       out           Output key material
 * @param size_t      out_size      Output size (max. 255 * 32 bytes)
 *
 * @return bool False if out_size is too large
 */
inline
bool hkdf_sha256_expand(
    const byte* const prk,
    const byte* const info, size_t info_size,
    byte* const out, size_t out_size
) NO_EXCEPT
{
    if (out_size > 255 * HMAC_SHA256_DIGEST_SIZE) {
        return false;
    }

    byte block[HMAC_SHA256_DIGEST_SIZE];
    size_t pos = 0;

    for (byte counter = 1; pos < out_size; ++counter) {
        HmacSha256Context ctx;
        hmac_sha256_init(&ctx, prk, HMAC_SHA256_DIGEST_SIZE);

        // T(i) = HMAC(PRK, T(i - 1) | info | i)
        if (counter > 1) {
            hmac_sha256_update(&ctx, block, sizeof(block));
        }

        if (info && info_size) {
            hmac_sha256_update(&ctx, info, info_size);
        }

        hmac_sha256_update(&ctx, &counter, 1);
        hmac_sha256_final(&ctx, block);

        const size_t chunk = out_size - pos < sizeof(block) ? out_size - pos : sizeof(block);
        memcpy(out + pos, block, chunk);
        pos += chunk;
    }

    ct_memzero(block, sizeof(block));

    return true;
}

/**
 * extract() + expand() in one call
 */
inline
bool hkdf_sha256(
    const byte* const salt, size_t salt_size,
    const byte* const ikm, size_t ikm_size,
    const byte* const info, size_t info_size,
    byte* const out, size_t out_size
) NO_EXCEPT
{
    byte prk[HMAC_SHA256_DIGEST_SIZE];
    hkdf_sha256_extract(salt, salt_size, ikm, ikm_size, prk);

    const bool result = hkdf_sha256_expand(prk, info, info_size, out, out_size);
    ct_memzero(prk, sizeof(prk));

    return result;
}

#endif
