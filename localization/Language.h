#ifndef COMS_UI_LANGUAGE_H
#define COMS_UI_LANGUAGE_H

#include "../stdlib/Stdlib.h"
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
    RingMemory* const ring
) NO_EXCEPT
{
    FileBody file = {0};
    file_read(path, &file, ring);
    ASSERT_TRUE(file.size);

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

FORCE_INLINE
int32 language_data_size(const Language* language) NO_EXCEPT
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
) NO_EXCEPT
{
    const byte* pos = data;

    pos = read_le(pos, &language->count);
    pos = read_le(pos, &language->size);

    language->lang = (char **) language->data;
    char** pos_lang = language->lang;

    const byte* const start = language->data;

    // Load pointers/offsets
    for (int32 i = 0; i < language->count; ++i) {
        uint32 offset;
        pos = read_le(pos, &offset);

        *pos_lang++ = (char *) (start + offset);
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
) NO_EXCEPT
{
    byte* pos = data;

    pos = write_le(pos, language->count);
    pos = write_le(pos, language->size);

    const byte* const start = pos;

    // Save pointers
    for (int32 i = 0; i < language->count; ++i) {
        pos = write_le(pos, (uint32) ((uintptr_t) pos - (uintptr_t) start));
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