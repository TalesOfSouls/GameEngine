/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
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

template <typename C, typename T>
inline
bool file_write_secure(const C* const path, FileBody* const file, T* const mem) NO_EXCEPT
{
    C temp_path[PATH_MAX_LENGTH];
    const size_t path_len = str_length(path);

    memcpy(temp_path, path, path_len * sizeof(T));
    memcpy(
        temp_path + path_len,
        file_suffix_array<C>::value,
        sizeof(file_suffix_array<C>::value)
    );

    file_write(temp_path, file);

    byte mem_hash[20];
    sha1_hash(file->content, file->size, mem_hash);

    FileBody* temp_file = {0};
    file_read(temp_path, temp_file, mem);

    byte temp_hash[20];
    sha1_hash(file->content, file->size, temp_hash);

    file_delete(temp_path);

    if (memcmp(mem_hash, temp_hash, sizeof(temp_hash)) == 0) {
        return file_move(temp_path, path);
    }

    return false;
}

/**
 * @param const C* base Base path could even be the path of the exe.
 *                      This will be used to create the base path
 * @param const C* dest Destination path which is either an absolute path or relative path, relative to base
 * @param C* out Where do we want to store the final path
 */
template <typename C>
inline
void build_base_path(const C* base, C* out) NO_EXCEPT
{
    if (*base == (C) '.') {
        relative_to_absolute(base, out);
    } else {
        str_copy(out, base);
    }

    C* dir = str_right(out, (C) '/');
    if (!dir) {
        dir = str_right(out, (C) '\\');
    }

    if (dir) {
        *dir = (C) '\0';
    }
}

/**
 * Often you want to combine two paths to a new path.
 * One path is the base path and the other one is relative to the base bath.
 * This function also handles the special case where the other path is already an absolute path
 *
 * @param const C* base Base path could even be the path of the exe.
 *                      This will be used to create the base path
 * @param const C* dest Destination path which is either an absolute path or relative path, relative to base
 * @param C* out Where do we want to store the final path
 */
template <typename C>
inline
void build_path(const C* base, const C* dest, C* out) NO_EXCEPT
{
    // The base path could be to a .exe for example, we need to get the directory
    if (*dest == (C) '.') {
        build_base_path(base, out);
        str_copy(out + str_length(out), dest + 1);
    } else {
        str_copy(out, dest);
    }
}

template <typename C>
inline
void filename(const C* path, C* out) NO_EXCEPT
{
    C* start = str_right(path, (C) '/');
    if (!start) {
        start = str_right(path, (C) '\\');
    }

    if (start) {
        ++start;
    } else {
        start = path;
    }

    memcpy(out, start, (str_length(start) + 1) * sizeof(C));
}

template <typename C>
inline
C* filename(const C* path) NO_EXCEPT
{
    C* start = str_right(path, (C) '/');
    if (!start) {
        start = str_right(path, (C) '\\');
    }

    if (start) {
        ++start;

        return start;
    }

    return path;
}

#endif