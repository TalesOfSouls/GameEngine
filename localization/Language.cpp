#pragma once
#ifndef COMS_UI_LANGUAGE_C
#define COMS_UI_LANGUAGE_C

#include "../stdlib/Stdlib.h"
#include "../memory/RingMemory.cpp"
#include "../system/FileUtils.cpp"
#include "../utils/StringUtils.h"
#include "Language.h"

/**
 * Format:
 * // Comment (only full line comments)
 * menu_quit: Quit                     <- "key: string"
 * status_prefix: "Status: "           <- quotes keep leading/trailing spaces
 * net_info: Ping: %l\nLoss: %l        <- \n = new line, \t = tab, \" = quote, \\ = backslash
 */
#define LANGUAGE_KEY_MAX_LENGTH 64

/**
 * FNV-1a (32 bit), 0 is reserved for empty slots
 */
inline CONSTEXPR
uint32 language_hash(const char* key) NO_EXCEPT
{
    uint32 hash = 2166136261U;
    while (*key) {
        hash ^= (uint32) (byte) *key++;
        hash *= 16777619U;
    }

    return hash ? hash : 1;
}

FORCE_INLINE
const char* language_get(const Language* const lang, const char* const key) NO_EXCEPT
{
    const uint32 hash = language_hash(key);

    const LanguageHeader* const header = (const LanguageHeader *) lang->data;
    const LanguageSlot* const slots = (const LanguageSlot *) (header + 1);
    const uint32 mask = (uint32) header->slot_count - 1;

    // Terminates since there are always empty slots (slot_count >= 2 * count)
    for (uint32 i = hash & mask; ; i = (i + 1) & mask) {
        const LanguageSlot* const slot = &slots[i];
        if (!slot->hash) {
            return NULL;
        }

        if (slot->hash == hash && strcmp((const char *) (lang->data + slot->key), key) == 0) {
            return (const char *) (lang->data + slot->value);
        }
    }
}

FORCE_INLINE
int32 language_slot_count(int32 count) NO_EXCEPT
{
    int32 slot_count = 8;
    while (slot_count < count * 2) {
        slot_count <<= 1;
    }

    return slot_count;
}

/**
 * Upper bound of the memory image size for a language text
 */
inline
int32 language_txt_memory_size(const char* const text, int32 length) NO_EXCEPT
{
    int32 lines = 1;
    for (int32 i = 0; i < length; ++i) {
        lines += text[i] == '\n';
    }

    return (int32) sizeof(LanguageHeader)
        + language_slot_count(lines) * (int32) sizeof(LanguageSlot)
        + length + 2 * lines + (int32) sizeof(size_t);
}

/**
 * Parses a language text into lang->data
 *
 * @param capacity Size of lang->data (language_txt_memory_size() is always enough)
 *
 * @return Size of the memory image or -1 if lang->data is too small
 */
inline
int32 language_from_txt(
    Language* const __restrict lang,
    const char* __restrict text, int32 length,
    int32 capacity
) NO_EXCEPT
{
    // Count the amount of entries to calculate the slot size
    int32 entry_count = 0;
    for (int32 pos = 0; pos < length; ) {
        int32 i = pos;
        while (i < length && (text[i] == ' ' || text[i] == '\t')) {
            ++i;
        }

        const bool comment = i + 1 < length && text[i] == '/' && text[i + 1] == '/';

        bool has_colon = false;
        while (pos < length && text[pos] != '\n') {
            has_colon |= text[pos] == ':';
            ++pos;
        }

        entry_count += has_colon && !comment;
        ++pos;
    }

    const int32 slot_count = language_slot_count(entry_count);
    const int32 strings_offset = (int32) sizeof(LanguageHeader) + slot_count * (int32) sizeof(LanguageSlot);

    // The strings are never longer than the text (+ 2 null terminators per entry + alignment of the size)
    const int32 needed = strings_offset + length + 2 * entry_count + (int32) sizeof(size_t);
    if (needed > capacity) {
        // No assert, the language files can be created by users (e.g. translations)
        LOG_1("[ERROR] Language data too small: %d B needed", {DATA_TYPE_INT32, (int32 *) &needed});

        return -1;
    }

    memset(lang->data, 0, strings_offset);

    LanguageHeader* const header = (LanguageHeader *) lang->data;
    LanguageSlot* const slots = (LanguageSlot *) (header + 1);
    const uint32 mask = (uint32) slot_count - 1;

    char* out = (char *) (lang->data + strings_offset);
    int32 count = 0;
    int32 version = 0;
    int32 line_number = 0;

    for (int32 pos = 0; pos < length; ) {
        ++line_number;

        int32 line_start = pos;
        while (pos < length && text[pos] != '\n') {
            ++pos;
        }

        int32 line_end = pos;
        ++pos;

        while (line_start < line_end && (text[line_start] == ' ' || text[line_start] == '\t')) {
            ++line_start;
        }

        while (line_end > line_start && str_is_space(text[line_end - 1])) {
            --line_end;
        }

        // Empty line or comment
        if (line_start >= line_end
            || (line_start + 1 < line_end && text[line_start] == '/' && text[line_start + 1] == '/')
        ) {
            continue;
        }

        int32 colon = line_start;
        while (colon < line_end && text[colon] != ':') {
            ++colon;
        }

        if (colon >= line_end) {
            if (!count && line_end - line_start > 8 && strncmp(text + line_start, "version ", 8) == 0) {
                version = (int32) str_to_int(text + line_start + 8);
            } else {
                LOG_1("[WARNING] Language: invalid line %d", {DATA_TYPE_INT32, &line_number});
            }

            continue;
        }

        int32 key_end = colon;
        while (key_end > line_start && str_is_space(text[key_end - 1])) {
            --key_end;
        }

        const int32 key_length = key_end - line_start;
        if (key_length <= 0 || key_length >= LANGUAGE_KEY_MAX_LENGTH) {
            LOG_1("[WARNING] Language: invalid key in line %d", {DATA_TYPE_INT32, &line_number});
            ASSERT_TRUE(key_length > 0 && key_length < LANGUAGE_KEY_MAX_LENGTH);

            continue;
        }

        int32 value_start = colon + 1;
        while (value_start < line_end && (text[value_start] == ' ' || text[value_start] == '\t')) {
            ++value_start;
        }

        int32 value_end = line_end;

        // Quotes keep leading/trailing spaces
        if (value_end - value_start >= 2 && text[value_start] == '"' && text[value_end - 1] == '"') {
            ++value_start;
            --value_end;
        }

        // Key
        char* const key = out;
        memcpy(out, text + line_start, key_length);
        out += key_length;
        *out++ = '\0';

        // String (escape sequences)
        char* const value = out;
        for (int32 i = value_start; i < value_end; ++i) {
            if (text[i] == '\\' && i + 1 < value_end) {
                ++i;
                *out++ = text[i] == 'n' ? '\n' : (text[i] == 't' ? '\t' : text[i]);
            } else {
                *out++ = text[i];
            }
        }

        *out++ = '\0';

        // Insert (an existing key is overwritten)
        const uint32 hash = language_hash(key);
        for (uint32 i = hash & mask; ; i = (i + 1) & mask) {
            LanguageSlot* const slot = &slots[i];

            if (!slot->hash) {
                slot->hash = hash;
                slot->key = (int32) ((byte *) key - lang->data);
                slot->value = (int32) ((byte *) value - lang->data);
                ++count;

                break;
            }

            if (slot->hash == hash && strcmp((const char *) (lang->data + slot->key), key) == 0) {
                LOG_1("[WARNING] Language: duplicate key %s in line %d", {DATA_TYPE_CHAR_STR, (void *) key}, {DATA_TYPE_INT32, &line_number});
                slot->value = (int32) ((byte *) value - lang->data);

                break;
            }
        }
    }

    if (version != LANGUAGE_VERSION) {
        LOG_1("[WARNING] Language file version %d is not supported", {DATA_TYPE_INT32, &version});
    }

    header->version = LANGUAGE_VERSION;
    header->count = count;
    header->slot_count = slot_count;
    header->size = (int32) ALIGN_UP((int32) ((byte *) out - lang->data), (int32) sizeof(size_t));

    lang->size = header->size;
    lang->count = count;

    return lang->size;
}

/**
 * Loads a language text file (e.g. used by the asset builder)
 *
 * @param capacity Size of lang->data
 *
 * @return Size of the memory image or -1 if the file doesn't exist or lang->data is too small
 *
 * @todo remove this shouldn't be here it all it does is load the text file which should not be in here
 */
template <typename C, typename M>
int32 language_from_file_txt(
    Language* const __restrict lang,
    const C* __restrict path,
    M* const __restrict mem,
    int32 capacity = MAX_INT32
) NO_EXCEPT
{
    if (!file_exists(path)) {
        return -1;
    }

    FileBody file = {0};
    file_read(path, &file, mem);
    if (!file.content) {
        return -1;
    }

    return language_from_txt(lang, (const char *) file.content, (int32) file.size, capacity);
}

int32 language_from_data(
    const byte* __restrict data,
    Language* __restrict lang
) NO_EXCEPT
{
    if (!data) {
        LOG_1("[WARNING] No language data provided to load");
        return -1;
    }

    LOG_3("[INFO] Load language");

    const LanguageHeader* const header = (const LanguageHeader *) data;
    if (header->version != LANGUAGE_VERSION || header->size < (int32) sizeof(LanguageHeader)) {
        LOG_1("[WARNING] Incompatible language version %d", {DATA_TYPE_INT32, (int32 *) &header->version});
        return -1;
    }

    memcpy(lang->data, data, header->size);
    lang->size = header->size;
    lang->count = header->count;

    return lang->size;
}

int32 language_to_data(
    const Language* __restrict lang,
    byte* __restrict data
) NO_EXCEPT
{
    memcpy(data, lang->data, lang->size);

    return lang->size;
}

#endif
