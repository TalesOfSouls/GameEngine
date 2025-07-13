/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENCODING_BASE64_H
#define COMS_ENCODING_BASE64_H

#include "../stdlib/Types.h"
#include "../utils/StringUtils.h"
#include "Base64Definitions.h"

void base64_encode(const byte* data, char* encoded_data, size_t data_length = 0) {
    size_t output_length = 4 * ((data_length + 2) / 3);

    if (!data_length) {
        // WARNING: This should only happen if the data is a char string
        // Binary data is not allowed since it often has '\0' characters
        data_length = str_length((const char *) data);
    }

    size_t i = 0;
    size_t j = 0;
    while (i + 3 <= data_length) {
        uint32 triple = ((uint32) data[i] << 16) | ((uint32) data[i + 1] << 8) | data[i + 2];

        encoded_data[j++] = BASE64_CHARS[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = BASE64_CHARS[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = BASE64_CHARS[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = BASE64_CHARS[(triple >> 0 * 6) & 0x3F];

        i += 3;
    }

    if (i < data_length) {
        uint32 triple = ((uint32) data[i] << 16);
        if (i + 1 < data_length) {
            triple |= ((uint32) data[i + 1] << 8);
        }

        encoded_data[j++] = BASE64_CHARS[(triple >> 18) & 0x3F];
        encoded_data[j++] = BASE64_CHARS[(triple >> 12) & 0x3F];
        encoded_data[j] = (i + 1 < data_length) ? BASE64_CHARS[(triple >> 6) & 0x3F] : '=';
        encoded_data[j + 1] = '=';
    }

    encoded_data[output_length] = '\0';
}

size_t base64_decode(const char* encoded_data, byte* data, size_t encoded_length = 0) {
    if (!encoded_length) {
        encoded_length = str_length(encoded_data);
    }

    size_t output_length = encoded_length / 4 * 3;
    int32 padding = 0;

    if (encoded_data[encoded_length - 1] == '=') {
        --output_length;
        ++padding;

        if (encoded_data[encoded_length - 2] == '=') {
            --output_length;
            ++padding;
        }
    }

    size_t complete_blocks = (encoded_length - padding) / 4;
    size_t i, j;

    for (i = 0, j = 0; i < complete_blocks * 4; i += 4, j += 3) {
        uint32 sextet_a = BASE64_LOOKUP[(byte) encoded_data[i]];
        uint32 sextet_b = BASE64_LOOKUP[(byte) encoded_data[i + 1]];
        uint32 sextet_c = BASE64_LOOKUP[(byte) encoded_data[i + 2]];
        uint32 sextet_d = BASE64_LOOKUP[(byte) encoded_data[i + 3]];

        uint32 triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        data[j]     = (triple >> 16) & 0xFF;
        data[j + 1] = (triple >> 8) & 0xFF;
        data[j + 2] = triple & 0xFF;
    }

    if (padding > 0) {
        uint32 sextet_a = BASE64_LOOKUP[(byte) encoded_data[i]];
        uint32 sextet_b = BASE64_LOOKUP[(byte) encoded_data[i + 1]];
        uint32 sextet_c = (padding > 1) ? 0 : BASE64_LOOKUP[(byte) encoded_data[i + 2]];

        uint32 triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6);

        data[j + 1] = (triple >> 16) & 0xFF;
        if (padding == 1) {
            data[j + 2] = (triple >> 8) & 0xFF;
        }
    }

    return output_length;
}

#if __aarch64__
    #include "Base64SimdArm.h"
#else
    #include "Base64SimdX86.h"
#endif

#endif