/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_HASH_SHA1_DEFINITIONS_H
#define COMS_HASH_SHA1_DEFINITIONS_H

#include "../stdlib/Types.h"

#define SHA1_ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define SHA1_Ch(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define SHA1_Parity(x, y, z) ((x) ^ (y) ^ (z))
#define SHA1_Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define K1 0x5A827999
#define K2 0x6ED9EBA1
#define K3 0x8F1BBCDC
#define K4 0xCA62C1D6

struct SHA1_CTX {
    alignas(64) byte buffer[64];
    uint64 count;
    uint32 state[5];
};

#endif