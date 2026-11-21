#pragma once
#ifndef COMS_UI_LANGUAGE_H
#define COMS_UI_LANGUAGE_H

#include "../stdlib/Stdlib.h"

// Version of the text format (.langtxt)
#define LANGUAGE_VERSION 3

#define LANGUAGE_DEFAULT_CODE L"en_US"

/**
 * TEXT FORMAT: .langtxt, version LANGUAGE_VERSION
 *
 *      version 2
 *
 *      // Comment (only full line comments)
 *      menu_quit: Quit                     <- "key: value"
 *      status_prefix: "Status: "           <- quotes keep leading/trailing spaces
 *      net_info: Ping: %n\nLoss: %n        <- \n = new line, \t = tab, \" = quote, \\ = backslash
 *
 *      key     everything before the first ':', max. LANGUAGE_KEY_MAX_LENGTH - 1 = 63 characters
 *      value   trailing whitespace is trimmed unless the value is quoted
 *      a key that is defined twice overwrites the first value (warning)
 *      lines without ':' are ignored, except the optional first "version N" line
 *
 * Patterns like %n/%s are not interpreted here, they are just part of the string.
 *
 * BINARY FORMAT: .langbin / ASSET_TYPE_LANGUAGE payload, version LANGUAGE_VERSION
 *
 * The file is exactly the memory footprint, so it can be used in place without copying or parsing:
 *
 *      offset          size        type                field
 *      0               16          LanguageHeader      scalar header
 *      16              16 * S      LanguageSlot[S]     hash map with open addressing
 *      16 + 16*S       ...         char[]              "key\0value\0" strings
 *      total           size        -                   aligned up to sizeof(size_t)
 *
 *      S = slot_count = power of 2, at least 2 * count, minimum 8
 *
 * LanguageHeader (16 bytes)
 *
 *      0   4   int32   version         LANGUAGE_VERSION
 *      4   4   int32   count           amount of strings
 *      8   4   int32   slot_count      power of 2, >= 2 * count
 *      12  4   int32   size            total size incl. this header
 *
 * LanguageSlot (16 bytes)
 *
 *      0   4   uint32  hash            FNV-1a (32 bit) of the key, 0 = empty slot
 *      4   4   int32   key             offset of the key in the image
 *      8   4   int32   value           offset of the value in the image
 *      12  4   int32   value_length    length of the value in bytes excl. \0
 *
 * Lookup: bucket = hash & (slot_count - 1), linear probing (i + 1) & mask, stop at
 * the first empty slot, the key is compared with its length (memcmp + \0 check). Since the image always has
 * empty slots the probe always terminates. The strings are NUL terminated and
 * contiguous, keys and values can be interleaved.
 *
 * The layout keeps the language of the file and a global language separately.
 *
 * WARNING: the memory is written with memcpy (no endian conversion), it is only
 * portable between hosts with the same endianness and sizeof(size_t).
 */
struct LanguageHeader {
    int32 version;

    // Amount of strings
    int32 count;

    // Size of the slot array, power of 2 (at least twice the amount of strings)
    int32 slot_count;

    // Size of the memory incl. this header
    int32 size;
};

struct LanguageSlot {
    // 0 = empty slot
    uint32 hash;

    // Offsets in Language::data
    int32 key;
    int32 value;

    // Length of the value in bytes excl. \0 -> language_get_string() doesn't need a strlen
    int32 value_length;
};

/**
 * Key of a string incl. its hash and length
 *
 * String literals should use language_key("...") -> the hash + length are calculated at compile time,
 * the lookup doesn't need to hash the key or calculate its length.
 */
struct LanguageKey {
    uint32 hash;

    // Length in bytes excl. \0
    int32 length;

    const char* str;
};

struct Language {
    // At least 4 byte aligned
    // The data is NOT owned by the Language (e.g. memory of a theme, buffer memory, asset)
    byte* data;
    int32 size;

    // Amount of strings
    int32 count;
};

#endif
