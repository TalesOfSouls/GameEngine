/**
 * Random access file IO
 *
 * Positional reads/writes on an open file (no shared file pointer -> a handle can be used
 * by one thread at a time without seeking), directory helpers and an atomic file replace.
 * Used by storage formats that update parts of a file in place (e.g. the voxel region files).
 *
 * The paths are char strings (ASCII/UTF-8 subset), the functions don't log or assert on
 * failure, the caller decides what an error means.
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SYSTEM_FILE_RANDOM_ACCESS_H
#define COMS_SYSTEM_FILE_RANDOM_ACCESS_H

#include "../stdlib/Stdlib.h"
#include "../log/Stats.h"

#if _WIN32
    #include <windows.h>

    typedef HANDLE FileRA;
    #define FILE_RA_INVALID INVALID_HANDLE_VALUE
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <stdio.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/types.h>

    typedef int32 FileRA;
    #define FILE_RA_INVALID (-1)
#endif

enum FileRAMode : byte {
    // Read only, the file must exist
    FILE_RA_MODE_READ,

    // Read + write, the file must exist
    FILE_RA_MODE_WRITE,

    // Read + write, the file is created if it doesn't exist
    FILE_RA_MODE_CREATE,

    // Read + write, an existing file is truncated
    FILE_RA_MODE_TRUNCATE,
};

/**
 * Checks if a handle is valid
 *
 * @param FileRA fh File handle
 *
 * @return bool
 */
FORCE_INLINE
bool file_ra_valid(FileRA fh) NO_EXCEPT
{
    return fh != FILE_RA_INVALID;
}

/**
 * Opens a file for positional reads/writes
 *
 * Other processes/handles may read the file at the same time (e.g. tools), writes are exclusive.
 *
 * @param const char* path Path
 * @param FileRAMode mode Open mode
 *
 * @return FileRA FILE_RA_INVALID on failure
 */
inline
FileRA file_ra_open(const char* path, FileRAMode mode) NO_EXCEPT
{
    #if _WIN32
        DWORD access = GENERIC_READ;
        DWORD share = FILE_SHARE_READ;
        DWORD disposition = OPEN_EXISTING;

        switch (mode) {
            case FILE_RA_MODE_READ:
                share = FILE_SHARE_READ | FILE_SHARE_WRITE;
                break;
            case FILE_RA_MODE_WRITE:
                access |= GENERIC_WRITE;
                break;
            case FILE_RA_MODE_CREATE:
                access |= GENERIC_WRITE;
                disposition = OPEN_ALWAYS;
                break;
            case FILE_RA_MODE_TRUNCATE:
                access |= GENERIC_WRITE;
                disposition = CREATE_ALWAYS;
                break;
        }

        HANDLE fh = CreateFileA(path, access, share, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
    #else
        int32 flags = O_RDONLY;
        switch (mode) {
            case FILE_RA_MODE_READ:
                break;
            case FILE_RA_MODE_WRITE:
                flags = O_RDWR;
                break;
            case FILE_RA_MODE_CREATE:
                flags = O_RDWR | O_CREAT;
                break;
            case FILE_RA_MODE_TRUNCATE:
                flags = O_RDWR | O_CREAT | O_TRUNC;
                break;
        }

        FileRA fh = open(path, flags, 0644);
    #endif

    if (fh != FILE_RA_INVALID) {
        STATS_INCREMENT_PERSISTENT_DEBUG(DEBUG_COUNTER_FILE_HANDLE_COUNT);
    }

    return fh;
}

/**
 * Closes a file handle (invalid handles are ignored)
 *
 * @param FileRA fh File handle
 *
 * @return void
 */
inline
void file_ra_close(FileRA fh) NO_EXCEPT
{
    if (fh == FILE_RA_INVALID) {
        return;
    }

    #if _WIN32
        CloseHandle(fh);
    #else
        close(fh);
    #endif

    STATS_DECREMENT_PERSISTENT_DEBUG(DEBUG_COUNTER_FILE_HANDLE_COUNT);
}

/**
 * Returns the file size
 *
 * @param FileRA fh File handle
 *
 * @return uint64
 */
inline
uint64 file_ra_size(FileRA fh) NO_EXCEPT
{
    #if _WIN32
        LARGE_INTEGER size;
        if (!GetFileSizeEx(fh, &size)) {
            return 0;
        }

        return (uint64) size.QuadPart;
    #else
        struct stat st;
        if (fstat(fh, &st) != 0) {
            return 0;
        }

        return (uint64) st.st_size;
    #endif
}

/**
 * Reads at a file position
 *
 * @param FileRA fh File handle
 * @param uint64 offset File position
 * @param void* buffer Destination
 * @param uint32 length Bytes to read
 *
 * @return uint32 Bytes read (less than length at the end of the file or on failure)
 */
inline
uint32 file_ra_read(FileRA fh, uint64 offset, void* buffer, uint32 length) NO_EXCEPT
{
    uint32 total = 0;

    while (total < length) {
        #if _WIN32
            OVERLAPPED ov = {};
            ov.Offset = (DWORD) ((offset + total) & 0xFFFFFFFF);
            ov.OffsetHigh = (DWORD) ((offset + total) >> 32);

            DWORD read = 0;
            if (!ReadFile(fh, (byte *) buffer + total, length - total, &read, &ov) || read == 0) {
                break;
            }
        #else
            const ssize_t read = pread(fh, (byte *) buffer + total, length - total, (off_t) (offset + total));
            if (read <= 0) {
                break;
            }
        #endif

        total += (uint32) read;
    }

    STATS_INCREMENT_BY_DEBUG(DEBUG_COUNTER_DRIVE_READ, total);
    STATS_INCREMENT_BY_DEBUG(DEBUG_COUNTER_DRIVE_IO, total);

    return total;
}

/**
 * Writes at a file position (the file grows if necessary)
 *
 * @param FileRA fh File handle
 * @param uint64 offset File position
 * @param const void* buffer Source
 * @param uint32 length Bytes to write
 *
 * @return bool
 */
inline
bool file_ra_write(FileRA fh, uint64 offset, const void* buffer, uint32 length) NO_EXCEPT
{
    uint32 total = 0;

    while (total < length) {
        #if _WIN32
            OVERLAPPED ov = {};
            ov.Offset = (DWORD) ((offset + total) & 0xFFFFFFFF);
            ov.OffsetHigh = (DWORD) ((offset + total) >> 32);

            DWORD written = 0;
            if (!WriteFile(fh, (const byte *) buffer + total, length - total, &written, &ov) || written == 0) {
                return false;
            }
        #else
            const ssize_t written = pwrite(fh, (const byte *) buffer + total, length - total, (off_t) (offset + total));
            if (written <= 0) {
                return false;
            }
        #endif

        total += (uint32) written;
    }

    STATS_INCREMENT_BY_DEBUG(DEBUG_COUNTER_DRIVE_WRITE, total);
    STATS_INCREMENT_BY_DEBUG(DEBUG_COUNTER_DRIVE_IO, total);

    return true;
}

/**
 * Flushes the written data to the drive
 *
 * @param FileRA fh File handle
 *
 * @return void
 */
inline
void file_ra_flush(FileRA fh) NO_EXCEPT
{
    #if _WIN32
        FlushFileBuffers(fh);
    #else
        fsync(fh);
    #endif
}

/**
 * Checks if a file or directory exists
 *
 * @param const char* path Path
 *
 * @return bool
 */
inline
bool file_ra_exists(const char* path) NO_EXCEPT
{
    #if _WIN32
        return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
    #else
        struct stat st;
        return stat(path, &st) == 0;
    #endif
}

/**
 * Checks if a path is a directory
 *
 * @param const char* path Path
 *
 * @return bool
 */
inline
bool file_ra_is_directory(const char* path) NO_EXCEPT
{
    #if _WIN32
        const DWORD attr = GetFileAttributesA(path);
        return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
    #else
        struct stat st;
        return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
    #endif
}

/**
 * Creates a directory including all missing parent directories
 *
 * @param const char* path Directory path ('/' or '\\' separated)
 *
 * @return bool true if the directory exists afterwards
 */
inline
bool file_ra_directory_create(const char* path) NO_EXCEPT
{
    char buffer[260];
    const size_t length = strlen(path);
    if (length == 0 || length >= sizeof(buffer)) {
        return false;
    }

    memcpy(buffer, path, length + 1);

    for (size_t i = 1; i <= length; ++i) {
        if (buffer[i] != '/' && buffer[i] != '\\' && buffer[i] != '\0') {
            continue;
        }

        const char old = buffer[i];
        buffer[i] = '\0';

        // Skip "." / ".." / drive letters
        if (!(i == 1 && buffer[0] == '.')
            && !(i == 2 && buffer[0] == '.' && buffer[1] == '.')
            && !(i == 2 && buffer[1] == ':')
            && !file_ra_is_directory(buffer)
        ) {
            #if _WIN32
                if (!CreateDirectoryA(buffer, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
                    return false;
                }
            #else
                if (mkdir(buffer, 0755) != 0 && !file_ra_is_directory(buffer)) {
                    return false;
                }
            #endif
        }

        buffer[i] = old;
    }

    return file_ra_is_directory(path);
}

/**
 * Replaces dst with src (atomic on the same volume)
 *
 * Used to write a complete file safely: write a temporary file, flush it, then replace the old one.
 *
 * @param const char* src Source path
 * @param const char* dst Destination path
 *
 * @return bool
 */
inline
bool file_ra_replace(const char* src, const char* dst) NO_EXCEPT
{
    #if _WIN32
        return MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    #else
        return rename(src, dst) == 0;
    #endif
}

/**
 * Deletes a file
 *
 * @param const char* path Path
 *
 * @return bool
 */
inline
bool file_ra_delete(const char* path) NO_EXCEPT
{
    #if _WIN32
        return DeleteFileA(path) != 0;
    #else
        return unlink(path) == 0;
    #endif
}

/**
 * Writes a complete file atomically (temporary file + replace)
 *
 * @param const char* path Destination path
 * @param const void* data File content
 * @param uint32 length Content length
 *
 * @return bool
 */
inline
bool file_ra_write_atomic(const char* path, const void* data, uint32 length) NO_EXCEPT
{
    char temp[260];
    const size_t path_length = strlen(path);
    if (path_length + 5 >= sizeof(temp)) {
        return false;
    }

    memcpy(temp, path, path_length);
    memcpy(temp + path_length, ".tmp", 5);

    FileRA fh = file_ra_open(temp, FILE_RA_MODE_TRUNCATE);
    if (!file_ra_valid(fh)) {
        return false;
    }

    const bool ok = file_ra_write(fh, 0, data, length);
    file_ra_flush(fh);
    file_ra_close(fh);

    if (!ok) {
        file_ra_delete(temp);

        return false;
    }

    return file_ra_replace(temp, path);
}

/**
 * Reads a complete file into a caller provided buffer
 *
 * @param const char* path Path
 * @param void* buffer Destination
 * @param uint32 capacity Buffer size
 *
 * @return int64 File size, -1 if the file doesn't exist or doesn't fit into the buffer
 */
inline
int64 file_ra_read_all(const char* path, void* buffer, uint32 capacity) NO_EXCEPT
{
    FileRA fh = file_ra_open(path, FILE_RA_MODE_READ);
    if (!file_ra_valid(fh)) {
        return -1;
    }

    const uint64 size = file_ra_size(fh);
    if (size > capacity) {
        file_ra_close(fh);

        return -1;
    }

    const uint32 read = file_ra_read(fh, 0, buffer, (uint32) size);
    file_ra_close(fh);

    return read == size ? (int64) size : -1;
}

/**
 * Lists the sub directories of a directory
 *
 * @param const char* path Directory path
 * @param char* names Output names (count * name_size chars)
 * @param int32 name_size Max. size of a name incl. the null terminator (longer names are skipped)
 * @param int32 count Max. number of names
 *
 * @return int32 Number of names written
 */
inline
int32 file_ra_directory_list(const char* path, char* names, int32 name_size, int32 count) NO_EXCEPT
{
    int32 found = 0;

    #if _WIN32
        char pattern[260];
        const size_t length = strlen(path);
        if (length + 3 >= sizeof(pattern)) {
            return 0;
        }

        memcpy(pattern, path, length);
        memcpy(pattern + length, "/*", 3);

        WIN32_FIND_DATAA data;
        HANDLE find = FindFirstFileA(pattern, &data);
        if (find == INVALID_HANDLE_VALUE) {
            return 0;
        }

        do {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || data.cFileName[0] == '.') {
                continue;
            }

            const size_t name_length = strlen(data.cFileName);
            if ((int32) name_length >= name_size) {
                continue;
            }

            memcpy(names + found * name_size, data.cFileName, name_length + 1);
            ++found;
        } while (found < count && FindNextFileA(find, &data));

        FindClose(find);
    #else
        DIR* dir = opendir(path);
        if (!dir) {
            return 0;
        }

        struct dirent* entry;
        while (found < count && (entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') {
                continue;
            }

            char full[512];
            snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
            if (!file_ra_is_directory(full)) {
                continue;
            }

            const size_t name_length = strlen(entry->d_name);
            if ((int32) name_length >= name_size) {
                continue;
            }

            memcpy(names + found * name_size, entry->d_name, name_length + 1);
            ++found;
        }

        closedir(dir);
    #endif

    return found;
}

#endif
