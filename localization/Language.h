#pragma once
#ifndef COMS_UI_LANGUAGE_H
#define COMS_UI_LANGUAGE_H

#include "../stdlib/Stdlib.h"

#define LANGUAGE_VERSION 2
#define LANGUAGE_DEFAULT_CODE L"en_US"

/**
 * Localized strings, key -> string
 *
 * Memory image (data), position independent (offsets relative to data) and identical to the binary file:
 *      LanguageHeader
 *      LanguageSlot[slot_count]    hash map with open addressing (linear probing), slot_count = power of 2
 *      "key\0string\0" ...         strings
 */
struct LanguageHeader {
    int32 version;

    // Amount of strings
    int32 count;

    // Size of the slot array, power of 2 (at least twice the amount of strings)
    int32 slot_count;

    // Size of the memory image incl. this header
    int32 size;
};

struct LanguageSlot {
    // 0 = empty slot
    uint32 hash;

    // Offsets in Language::data
    int32 key;
    int32 value;
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
