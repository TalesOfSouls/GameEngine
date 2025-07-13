/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENCRYPTION_XOR_H
#define COMS_ENCRYPTION_XOR_H

#include "../stdlib/Types.h"

constexpr inline
void encrypt_xor(char* input, char key) {
    for (int32 i = 0; input[i] != '\0'; i++) {
        input[i] ^= key;
    }
}

constexpr inline
void decrypt_xor(char* input, char key) {
    for (int32 i = 0; input[i] != '\0'; i++) {
        input[i] ^= key;
    }
}

#endif