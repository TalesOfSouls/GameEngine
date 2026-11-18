/**
 * @copyright Jingga
 * @license   OMS License 2.0
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

/**
 * Securely overwrites a file preventing corruption
 * Only after the file is successfully written to the file system it replaces the old file
 */
template <typename C, typename T>
inline
bool file_write_secure(const C* const path, const FileBody* const file, T* const mem) NO_EXCEPT
{
    C temp_path[PATH_MAX_LENGTH];
    const size_t path_len = str_length(path);

    memcpy(temp_path, path, (path_len + 1) * sizeof(C));
    memcpy(
        temp_path + path_len,
        file_suffix_array<C>::value,
        sizeof(file_suffix_array<C>::value)
    );

    file_write(temp_path, file);

    byte mem_hash[20] = {0};
    sha1_hash(file->content, file->size, mem_hash);

    FileBody temp_file = {0};
    file_read(temp_path, &temp_file, mem);

    byte temp_hash[20]= {0};
    sha1_hash(temp_file.content, temp_file.size, temp_hash);

    if (memcmp(mem_hash, temp_hash, sizeof(temp_hash)) == 0) {
        file_delete(path);
        return file_move(temp_path, path);
    }

    file_delete(temp_path);

    return false;
}

/**
 * Writes a complete file atomically: temporary file + flush + replace
 * Unlike file_write_secure() the written data isn't read back for verification (no memory needed)
 * and the old file is replaced in one step (it never goes missing)
 *
 * @return bool
 */
template <typename C>
inline
bool file_write_atomic(const C* const path, const FileBody* const file) NO_EXCEPT
{
    C temp_path[PATH_MAX_LENGTH];
    const size_t path_len = str_length(path);
    if (path_len + ARRAY_COUNT(file_suffix_array<C>::value) > PATH_MAX_LENGTH) {
        return false;
    }

    memcpy(temp_path, path, path_len * sizeof(C));
    memcpy(
        temp_path + path_len,
        file_suffix_array<C>::value,
        sizeof(file_suffix_array<C>::value)
    );

    FileHandle fp = file_read_write_handle(temp_path, FILE_OPEN_MODE_TRUNCATE);
    if (!fp) {
        return false;
    }

    const bool ok = file_write_at(fp, 0, file->content, (uint32) file->size);
    file_flush(fp);
    file_close_handle(fp);

    if (!ok) {
        file_delete(temp_path);

        return false;
    }

    return file_replace(temp_path, path);
}

/**
 * Reads a complete file into a caller provided buffer
 * Unlike file_read() the buffer size is checked, nothing is appended (no null termination)
 * and a missing file is no error
 *
 * @param const C* path File path
 * @param void* buffer Destination
 * @param uint32 capacity Buffer size
 *
 * @return int64 File size, -1 if the file doesn't exist, can't be read or doesn't fit into the buffer
 */
template <typename C>
inline
int64 file_read_buffer(const C* const path, void* const buffer, uint32 capacity) NO_EXCEPT
{
    FileHandle fp = file_read_handle_shared(path);
    if (!fp) {
        return -1;
    }

    const uint64 size = file_size(fp);
    const uint32 bytes_read = size <= capacity ? file_read_at(fp, 0, buffer, (uint32) size) : 0;
    file_close_handle(fp);

    return size <= capacity && bytes_read == size ? (int64) size : -1;
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
    const C* start = str_right(path, (C) '/');
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
C* filename(C* path) NO_EXCEPT
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