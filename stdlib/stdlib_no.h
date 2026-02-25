/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_NO_H
#define COMS_STDLIB_NO_H

#if defined(__GNUC__) || defined(__clang__)
    // Functions provided by the compiler even if "no" stdlib is available
    // In reality THIS is just the stdlib
    #define memmove __builtin_memmove
    #define memcpy __builtin_memcpy
    #define memset __builtin_memset
    #define memcmp __builtin_memcmp

    #define strlen __builtin_strlen
    #define wcslen __builtin_wcslen

    #define strcpy __builtin_strcpy
    #define wcscpy __builtin_wcscpy

    #define strcat __builtin_strcat
    #define wcscat __builtin_wcscat

    #define strstr __builtin_strstr
    #define strchr __builtin_strchr
    #define wcsstr __builtin_wcsstr

    #define strncpy __builtin_strncpy
    #define wcsncpy __builtin_wcsncpy

    #define strcmp __builtin_strcmp
    #define wcscmp __builtin_wcscmp
    #define strncmp __builtin_strncmp
    #define wcsncmp __builtin_wcsncmp

    #define toupper __builtin_toupper
    #define tolower __builtin_tolower

    #define isdigit __builtin_isdigit
    #define isalpha __builtin_isalpha
    #define isalnum __builtin_isalnum
#else
    #include "stdlib_internal.h"

    // These are user space stdlib functions
    #define memmove __internal_memmove
    #define memcpy __internal_memcpy
    #define memset __internal_memset
    #define memcmp __internal_memcmp

    #define strlen __internal_strlen
    #define wcslen __internal_wcslen

    #define strcpy __internal_strcpy
    #define wcscpy __internal_wcscpy

    #define strcat __internal_strcat
    #define wcscat __internal_wcscat

    #define strstr __internal_strstr
    #define strchar __internal_strchar
    #define wcsstr __internal_wcsstr

    #define strncpy __internal_strncpy
    #define wcsncpy __internal_wcsncpy

    #define strcmp __internal_strcmp
    #define wcscmp __internal_wcscmp
    #define strncmp __internal_strncmp
    #define wcsncmp __internal_wcsncmp

    #define toupper __internal_toupper
    #define tolower __internal_tolower

    #define isdigit __internal_isdigit
    #define isalpha __internal_isalpha
    #define isalnum __internal_isalnum
#endif

#endif