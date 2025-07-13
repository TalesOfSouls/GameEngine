/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENCRYPTION_CEASAR_H
#define COMS_ENCRYPTION_CEASAR_H

#include "../stdlib/Types.h"

constexpr inline
void encrypt_ceasar(char* input, int32 shift) {
    for (int32 i = 0; input[i] != '\0'; i++) {
        if (input[i] >= 'A' && input[i] <= 'Z') {
            input[i] = 'A' + (input[i] - 'A' + shift) % 26;
        } else if (input[i] >= 'a' && input[i] <= 'z') {
            input[i] = 'a' + (input[i] - 'a' + shift) % 26;
        }
    }
}

constexpr inline
void decrypt_ceasar(char* input, int32 shift) {
    for (int32 i = 0; input[i] != '\0'; i++) {
        if (input[i] >= 'A' && input[i] <= 'Z') {
            input[i] = 'A' + (input[i] - 'A' - shift + 26) % 26;
        } else if (input[i] >= 'a' && input[i] <= 'z') {
            input[i] = 'a' + (input[i] - 'a' - shift + 26) % 26;
        }
    }
}

#endif