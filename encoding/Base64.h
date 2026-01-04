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

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"
#include "Base64Definitions.h"

void base64_encode(const char* data, char* encoded_data, size_t data_length = 0) NO_EXCEPT
{
    if (!data_length) {
        // WARNING: This should only happen if the data is a char string
        // Binary data is not allowed since it often has '\0' characters
        data_length = str_length(data);
    }

    size_t i = 0;
    size_t j = 0;
    while (i + 3 <= data_length) {
        const uint32 triple = ((uint32) data[i] << 16) | ((uint32) data[i + 1] << 8) | data[i + 2];

        encoded_data[j++] = BASE64_CHARS[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = BASE64_CHARS[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = BASE64_CHARS[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = BASE64_CHARS[(triple >> 0 * 6) & 0x3F];

        i += 3;
    }

    if (i < data_length) {
        uint32 triple = ((uint32) data[i] << 16);

        encoded_data[j++] = BASE64_CHARS[(triple >> 18) & 0x3F];
        encoded_data[j++] = BASE64_CHARS[(triple >> 12) & 0x3F];

        if (i + 1 < data_length) {
            triple |= (uint32)data[i + 1] << 8;
            encoded_data[j++] = BASE64_CHARS[(triple >> 6) & 0x3F];
        } else {
            encoded_data[j++] = '=';
        }

        encoded_data[j++] = '=';
    }

    encoded_data[j] = '\0';
}

void base64_encode(const wchar_t* data, wchar_t* output, size_t data_length = 0) {
    if (!data_length) {
        // WARNING: This should only happen if the data is a char string
        // Binary data is not allowed since it often has '\0' characters
        data_length = str_length(data);
    }

    size_t i = 0;
    size_t j = 0;

    while (i + 3 <= data_length) {
        const uint32 triple = ((uint32) data[i] << 16) | ((uint32) data[i+1] << 8) | (uint32) data[i+2];

        output[j++] = BASE64_CHARS[(triple >> 18) & 0x3F];
        output[j++] = BASE64_CHARS[(triple >> 12) & 0x3F];
        output[j++] = BASE64_CHARS[(triple >> 6) & 0x3F];
        output[j++] = BASE64_CHARS[triple & 0x3F];

        i += 3;
    }

    if (i < data_length) {
        uint32 triple = ((uint32)data[i] << 16);
        if (i + 1 < data_length) {
            triple |= ((uint32)data[i+1] << 8);
        }

        output[j++] = BASE64_CHARS[(triple >> 18) & 0x3F];
        output[j++] = BASE64_CHARS[(triple >> 12) & 0x3F];
        output[j++] = (i + 1 < data_length) ? BASE64_CHARS[(triple >> 6) & 0x3F] : L'=';
        output[j++] = L'=';
    }

    output[j] = L'\0';
}

size_t base64_decode(const char* encoded_data, char* data, size_t encoded_length = 0) NO_EXCEPT
{
    if (!encoded_length) {
        encoded_length = str_length(encoded_data);
    }

    size_t output_length = base64_decoded_length(encoded_length);
    int32 padding = 0;

    if (encoded_data[encoded_length - 1] == '=') {
        --output_length;
        ++padding;

        if (encoded_data[encoded_length - 2] == '=') {
            --output_length;
            ++padding;
        }
    }

    size_t complete_blocks = encoded_length - padding;
    size_t i, j;

    for (i = 0, j = 0; i < complete_blocks; i += 4, j += 3) {
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

size_t base64_decode(const wchar_t* encoded_data, wchar_t* data, size_t encoded_length = 0) {
    if (encoded_length == 0) {
        encoded_length = str_length(encoded_data);
    }

    size_t output_length = base64_decoded_length(encoded_length);
    int padding = 0;

    if (encoded_length >= 1 && encoded_data[encoded_length - 1] == L'=') {
        padding++;
        output_length--;
        if (encoded_length >= 2 && encoded_data[encoded_length - 2] == L'=') {
            padding++;
            output_length--;
        }
    }

    size_t complete_blocks = encoded_length - padding;
    size_t i, j;

    for (i = 0, j = 0; i < complete_blocks; i += 4, j += 3) {
        const uint32 sextet_a = BASE64_LOOKUP[(byte)encoded_data[i]];
        const uint32 sextet_b = BASE64_LOOKUP[(byte)encoded_data[i + 1]];
        const uint32 sextet_c = BASE64_LOOKUP[(byte)encoded_data[i + 2]];
        const uint32 sextet_d = BASE64_LOOKUP[(byte)encoded_data[i + 3]];

        const uint32 triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6) | sextet_d;

        data[j] = (wchar_t)((triple >> 16) & 0xFF);
        if (j + 1 < output_length) {
            data[j + 1] = (wchar_t)((triple >> 8) & 0xFF);
        }

        if (j + 2 < output_length) {
            data[j + 2] = (wchar_t)(triple & 0xFF);
        }
    }

    if (padding > 0) {
        const uint32 sextet_a = BASE64_LOOKUP[(byte)encoded_data[i]];
        const uint32 sextet_b = BASE64_LOOKUP[(byte)encoded_data[i + 1]];
        const uint32 sextet_c = (padding > 1) ? 0 : BASE64_LOOKUP[(byte)encoded_data[i + 2]];

        const uint32 triple = (sextet_a << 18) | (sextet_b << 12) | (sextet_c << 6);

        if (padding == 1) {
            data[j + 1] = (wchar_t)((triple >> 8) & 0xFF);
        }

        data[j] = (wchar_t)((triple >> 16) & 0xFF);
        if (padding == 2) {
            data[j + 1] = 0; // last byte is not used
        }
    }

    return output_length;
}

#ifdef __aarch64__
    #include "Base64SimdArm.h"
#else
    #include "Base64SimdX86.h"
#endif

#endif