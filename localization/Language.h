#ifndef COMS_UI_LANGUAGE_H
#define COMS_UI_LANGUAGE_H

#include "../stdlib/Types.h"
#include "../memory/RingMemory.h"
#include "../system/FileUtils.cpp"

#define LANGUAGE_VERSION 1

// Size is limited to 4GB
struct Language {
    // WARNING: the actual start of data is data -= sizeof(Language); see file loading below
    // The reason for this is we store the Language struct in the beginning of the data/file
    byte* data;

    int32 count;
    int32 size;
    char** lang;
};

void language_from_file_txt(
    Language* language,
    const char* path,
    RingMemory* ring
) {
    FileBody file = {};
    file_read(path, &file, ring);
    ASSERT_SIMPLE(file.size);

    // count elements
    language->count = 1;
    int32 len = 0;

    byte* data = file.content;

    while (data[len] != '\0') {
        if (data[len] == '\n' && data[len + 1] == '\n') {
            ++language->count;
            data[len] = '\0';
            ++len;
        }

        ++len;
    }

    language->size = len;
    language->lang = (char **) language->data;
    memcpy(language->data + language->count * sizeof(char *), data, len);

    // First element = 0
    char** pos = language->lang;
    *pos++ = (char *) data;

    for (int32 i = 1; i < len - 1; ++i) {
        if (data[i] == '\0') {
            // We have to move by 2 since every text element is separated by 2 \n
            // 1 \n is a valid char for a single text element
            // @performance This also means that we have one additional byte for
            // every text element even in the binary version.
            *pos++ = (char *) &data[i + 2];
        }
    }
}

int32 language_data_size(const Language* language)
{
    return (int32) (language->size
        + sizeof(language->count)
        + sizeof(language->size)
        + language->count * sizeof(uint32)
    );
}

// File layout - binary
// offsets for start of strings
// actual string data
int32 language_from_data(
    const byte* data,
    Language* language
) {
    const byte* pos = data;

    // Count
    language->count = SWAP_ENDIAN_LITTLE(*((int32 *) pos));
    pos += sizeof(language->count);

    language->size = SWAP_ENDIAN_LITTLE(*((int32 *) pos));
    pos += sizeof(language->size);

    language->lang = (char **) language->data;
    char** pos_lang = language->lang;

    byte* start = language->data;

    // Load pointers/offsets
    for (int32 i = 0; i < language->count; ++i) {
        *pos_lang++ = (char *) (start + SWAP_ENDIAN_LITTLE(*((uint32 *) pos)));
        pos += sizeof(uint32);
    }

    memcpy(
        language->data + language->count * sizeof(uint32),
        pos,
        language->size
    );

    return language_data_size(language);
}

int32 language_to_data(
    const Language* language,
    byte* data
) {
    byte* pos = data;

    // Count
    *((int32 *) pos) = SWAP_ENDIAN_LITTLE(language->count);
    pos += sizeof(language->count);

    // Count
    *((int32 *) pos) = SWAP_ENDIAN_LITTLE((int32) language->size);
    pos += sizeof(language->size);

    byte* start = pos;

    // Save pointers
    for (int32 i = 0; i < language->count; ++i) {
       *((uint32 *) pos) = SWAP_ENDIAN_LITTLE((uint32) ((uintptr_t) pos - (uintptr_t) start));
        pos += sizeof(uint32);
    }

    // Save actual strings
    memcpy(
        pos,
        language->data + language->count * sizeof(uint32),
        language->size
    );

    return language_data_size(language);
}

#endif