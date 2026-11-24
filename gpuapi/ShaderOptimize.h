/**
 * Shader source optimization (platform + gpu api independent)
 *
 * Used by the archive builder for the shader files of the asset archives, no gpu api headers are needed
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_GPUAPI_SHADER_OPTIMIZE_H
#define COMS_GPUAPI_SHADER_OPTIMIZE_H

#include "../stdlib/Stdlib.h"
#include "../utils/StringUtils.h"

// Only useful when creating shader files, not for optimizing shaders at runtime
// This only optimizes the file size of the shader on the hard drive:
// removes comments + unnecessary whitespace (preprocessor directives keep their own line)
//
// @return int32 Length of the output (without the null terminator, same as strlen)
CONSTEXPR int32 shader_program_optimize(const char* __restrict input, char* __restrict output) NO_EXCEPT
{
    const char* read_ptr = input;
    char* write_ptr = output;
    bool in_string = false;

    while (*read_ptr) {
        str_skip_empty(&read_ptr);

        // Preprocessor directives (e.g. #define) must start a new line, even after ; { ( ,
        if (write_ptr != output
            && *(write_ptr - 1) != '\n'
            && (*read_ptr == '#'
                || (*(write_ptr - 1) != ';' && *(write_ptr - 1) != '{'
                    && *(write_ptr - 1) != '('
                    && *(write_ptr - 1) != ','
                )
            )
        ) {
            *write_ptr++ = '\n';
        }

        // Handle single-line comments (//)
        if (*read_ptr == '/' && *(read_ptr + 1) == '/' && !in_string) {
            str_move_to(&read_ptr, '\n');

            continue;
        }

        // Handle multi-line comments (/* */)
        if (*read_ptr == '/' && *(read_ptr + 1) == '*' && !in_string) {
            // Go to end of comment
            while (*read_ptr && (*read_ptr != '*' || *(read_ptr + 1) != '/')) {
                ++read_ptr;
            }

            if (*read_ptr == '*' && *(read_ptr + 1) == '/') {
                read_ptr += 2;
            }

            continue;
        }

        // Handle strings to avoid removing content within them
        if (*read_ptr == '"') {
            in_string = !in_string;
        }

        // Copy valid characters to write_ptr
        while (*read_ptr && !is_eol(read_ptr) && *read_ptr != '"'
            && !(*read_ptr == '/' && (*(read_ptr + 1) == '/' || *(read_ptr + 1) == '*'))
        ) {
            if (!in_string
                && (*read_ptr == '*' || *read_ptr == '/' || *read_ptr == '=' || *read_ptr == '+' || *read_ptr == '-' || *read_ptr == '%'
                    || *read_ptr == '(' || *read_ptr == ')'
                    || *read_ptr == '{' || *read_ptr == '}'
                    || *read_ptr == ',' || *read_ptr == '?' || *read_ptr == ':' || *read_ptr == ';'
                    || *read_ptr == '&' || *read_ptr == '|'
                    || *read_ptr == '>' || *read_ptr == '<'
                )
            ) {
                if (write_ptr != output && (is_whitespace(*(write_ptr - 1)) || *(write_ptr - 1) == '\n')) {
                    --write_ptr;
                }

                *write_ptr++ = *read_ptr++;

                if (*read_ptr && is_whitespace(*read_ptr)) {
                    ++read_ptr;
                }
            } else {
                *write_ptr++ = *read_ptr++;
            }
        }
    }

    *write_ptr = '\0';

    return (int32) (write_ptr - output);
}

#endif
