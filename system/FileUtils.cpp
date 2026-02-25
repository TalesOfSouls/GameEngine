/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SYSTEM_FILE_UTILS_C
#define COMS_SYSTEM_FILE_UTILS_C

#include "../stdlib/Stdlib.h"

template <typename T>
struct file_suffix_array;

template <>
struct file_suffix_array<char>
{
    static constexpr char value[] = "_temp";
};

template <>
struct file_suffix_array<wchar_t>
{
    static constexpr wchar_t value[] = L"_temp";
};

#include "FileUtils.h"

#if _WIN32
    #include "../platform/win32/FileUtils.cpp"
#elif __linux__
    #include "../platform/linux/FileUtils.cpp"
#endif

#include "../hash/Sha1.h"

template <typename T>
inline
bool file_write_secure(const T* const path, FileBody* const file, RingMemory* const ring) NO_EXCEPT
{
    T temp_path[MAX_PATH];
    const size_t path_len = str_length(path);

    memcpy(temp_path, path, path_len * sizeof(T));
    memcpy(
        temp_path + path_len,
        file_suffix_array<T>::value,
        sizeof(file_suffix_array<T>::value)
    );

    file_write(temp_path, file);

    byte mem_hash[20];
    sha1_hash(file->content, file->size, mem_hash);

    FileBody* temp_file = {0};
    file_read(temp_path, temp_file, ring);

    byte temp_hash[20];
    sha1_hash(file->content, file->size, temp_hash);

    file_delete(temp_path);

    if (memcmp(mem_hash, temp_hash, sizeof(temp_hash)) == 0) {
        return file_move(temp_path, path);
    }

    return false;
}

#endif