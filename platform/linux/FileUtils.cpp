/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_FILE_UTILS_C
#define COMS_PLATFORM_LINUX_FILE_UTILS_C

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>

#include "../../stdlib/Stdlib.h"
#include "../../utils/StringUtils.h"
#include "../../memory/RingMemory.cpp"
#include "../../memory/BufferMemory.cpp"
#include "../../memory/ChunkMemory.cpp"
#include "../../log/PerformanceProfiler.h"

#ifndef PATH_MAX_LENGTH
    #define PATH_MAX_LENGTH PATH_MAX_LENGTH
#endif

typedef int32 FileHandle;
typedef int MMFHandle;

FORCE_INLINE
MMFHandle file_mmf_handle(FileHandle fp) {
    return fp;
}

inline
void* mmf_region_init(MMFHandle fh, size_t offset, size_t length = 0) {
    if (length == 0) {
        struct stat st;
        if (fstat(fh, &st) != 0) {
            return NULL;
        }

        length = st.st_size - offset;
    }

    size_t page_size = sysconf(_SC_PAGESIZE);

    // Offset (must be page-aligned)
    size_t aligned_offset = offset & ~(page_size - 1);
    size_t offset_diff = offset - aligned_offset;
    size_t map_length = length + offset_diff;

    void *mapped_region = mmap(NULL, map_length, PROT_READ, MAP_PRIVATE, fh, aligned_offset);

    if (mapped_region == MAP_FAILED) {
        return NULL;
    }

    return (char *) mapped_region + offset_diff;
}

inline
void mmf_region_release(void* region, size_t length = 0) {
    size_t page_size = sysconf(_SC_PAGESIZE);

    void *aligned_region = (void *) ((uintptr_t)region & ~(page_size - 1));

    munmap(aligned_region, length);
}

FORCE_INLINE
void file_mmf_close(MMFHandle fh) {
    close(fh);
}

/**
 * Path of the executable (same as platform/win32/FileUtils.cpp)
 *
 * @return int32 Length of the path, 0 on failure
 */
inline
int32 self_file_path(char* path) NO_EXCEPT
{
    static char spath[PATH_MAX_LENGTH];
    static int32 spath_length = 0;

    if (!spath_length) {
        // readlink doesn't null terminate
        const ssize_t length = readlink("/proc/self/exe", spath, PATH_MAX_LENGTH - 1);
        if (length <= 0) {
            path[0] = '\0';

            return 0;
        }

        spath[length] = '\0';
        spath_length = (int32) length;
    }

    // +1 for '\0'
    memcpy(path, spath, spath_length + 1);

    return spath_length;
}

/**
 * Directory of the executable incl. the trailing '/' (same as platform/win32/FileUtils.cpp)
 *
 * @param const char** path Output: cached directory (valid for the lifetime of the program)
 *
 * @return int32 Length of the directory, 0 on failure
 */
inline
int32 self_directory_path(const char** path) NO_EXCEPT
{
    static char sdir[PATH_MAX_LENGTH];
    static int32 sdir_length = 0;

    if (!sdir_length) {
        int32 length = self_file_path(sdir);
        while (length > 0 && sdir[length - 1] != '/') {
            --length;
        }

        sdir[length] = '\0';
        sdir_length = length;
    }

    *path = sdir;

    return sdir_length;
}

inline
int32 self_directory_path(char* path) NO_EXCEPT
{
    const char* sdir;
    const int32 length = self_directory_path(&sdir);

    // +1 for '\0'
    memcpy(path, sdir, length + 1);

    return length;
}

/**
 * Relative paths ("./...") are relative to the executable, all other paths are copied
 * (same as platform/win32/FileUtils.cpp)
 */
inline
void relative_to_absolute(const char* __restrict rel, char* __restrict path) NO_EXCEPT
{
    if (rel[0] != '.') {
        str_copy(path, rel);

        return;
    }

    const char* spath;
    const int32 spath_length = self_directory_path(&spath);
    if (spath_length == 0) {
        str_copy(path, rel);

        return;
    }

    const char* temp = rel;
    if (rel[1] == '/') {
        temp += 2;
    }

    memcpy(path, spath, spath_length);
    str_copy(path + spath_length, temp);
}

// Overwrites the path
inline
void relative_to_absolute(char* path) NO_EXCEPT
{
    if (path[0] != '.') {
        return;
    }

    char rel[PATH_MAX_LENGTH];
    str_copy(rel, path);
    relative_to_absolute(rel, path);
}

/**
 * Linux paths are utf-8, wide paths (windows style api, e.g. AssetArchive::path) are converted
 * wchar_t is utf-32 on Linux
 *
 * @param char* buffer Output buffer (PATH_MAX_LENGTH)
 *
 * @return const char* buffer
 */
inline
const char* file_path_utf8(const wchar_t* __restrict path, char* __restrict buffer) NO_EXCEPT
{
    int32 length = 0;

    // Max. 4 bytes per code point + '\0'
    while (*path && length < PATH_MAX_LENGTH - 5) {
        length += utf8_encode((uint32) *path++, buffer + length);
    }

    buffer[length] = '\0';

    return buffer;
}

inline
void relative_to_absolute(const wchar_t* __restrict rel, wchar_t* __restrict path) NO_EXCEPT
{
    char rel_utf8[PATH_MAX_LENGTH];
    char path_utf8[PATH_MAX_LENGTH];

    relative_to_absolute(file_path_utf8(rel, rel_utf8), path_utf8);
    utf8_to_wchar(path_utf8, path, PATH_MAX_LENGTH);
}

// @todo implement relative path support, similar to UtilsWin32
inline
size_t file_size(const char* filename) {
    struct stat buffer;

    if (*filename == '.') {
        char full_path[PATH_MAX_LENGTH];
        relative_to_absolute(filename, full_path);
        if (stat(full_path, &buffer) != 0) {
            return 0;
        }
    } else {
        if (stat(filename, &buffer) != 0) {
            return 0;
        }
    }

    return buffer.st_size;
}

inline
uint64 file_last_modified(const char* filename)
{
    struct stat buffer;

    if (*filename == '.') {
        char full_path[PATH_MAX_LENGTH];
        relative_to_absolute(filename, full_path);
        stat(full_path, &buffer);
    } else {
        stat(filename, &buffer);
    }

    return (uint64) buffer.st_mtime;
}

inline
FileHandle file_append_handle(const char* path) {
    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = open(full_path, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    } else {
        fp = open(path, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    }

    return fp;
}

inline
bool file_exists(const char* path) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    struct stat buffer;
    const char* full_path = path;
    char abs_path[PATH_MAX_LENGTH];

    if (*path == '.') {
        relative_to_absolute(path, abs_path);
        full_path = abs_path;
    }

    return stat(full_path, &buffer) == 0;
}

inline
bool file_copy(const char* __restrict src, const char* __restrict dst) {
    PROFILE_DEBUG(PROFILE_FILE_UTILS, src, PROFILE_FLAG_SHOULD_LOG);

    char src_full_path[PATH_MAX_LENGTH];
    char dst_full_path[PATH_MAX_LENGTH];

    if (*src == '.') {
        relative_to_absolute(src, src_full_path);
        src = src_full_path;
    }

    if (*dst == '.') {
        relative_to_absolute(dst, dst_full_path);
        dst = dst_full_path;
    }

    int32 src_fd = open(src, O_RDONLY);
    if (src_fd < 0) {
        return false;
    }

    int32 dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        close(src_fd);

        return false;
    }

    char buffer[8192];
    ssize_t bytes_read, bytes_written;
    bool success = true;

    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            success = false;
            break;
        }
    }

    if (bytes_read < 0) {
        success = false;
    }

    close(src_fd);
    close(dst_fd);

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, bytes_written);

    return success;
}

/**
 * Reads (a part of) a file, same behavior as platform/win32/FileUtils.cpp
 *      file->size > 0: max. file->size bytes from the offset
 *      file->size = 0: the rest of the file from the offset
 * The content is null terminated -> the memory must be 1 byte larger than the data
 *
 * @param T* mem Memory for the content (NULL = file->content is already allocated)
 */
template <typename T>
inline
void file_read(
    const char* __restrict path,
    FileBody* __restrict file,
    uint64 offset,
    T* const __restrict mem
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    char full_path[PATH_MAX_LENGTH];
    relative_to_absolute(path, full_path);

    const int32 fp = open(full_path, O_RDONLY);
    if (fp < 0) {
        file->size = 0;
        file->content = NULL;

        return;
    }

    struct stat file_stat;
    if (fstat(fp, &file_stat) == -1 || (uint64) file_stat.st_size <= offset) {
        close(fp);
        file->size = 0;
        file->content = NULL;

        return;
    }

    uint64 read_length = (uint64) file_stat.st_size - offset;
    if (file->size && file->size < read_length) {
        read_length = file->size;
    }

    if (read_length > MAX_UINT32) {
        close(fp);
        file->size = 0;
        file->content = NULL;

        return;
    }

    if (mem != NULL) {
        file->content = memory_get(mem, read_length + 1);
    }

    if (!file->content) {
        close(fp);
        file->size = 0;

        return;
    }

    // pread may return less than requested
    uint64 total = 0;
    while (total < read_length) {
        const ssize_t bytes_read = pread(fp, file->content + total, read_length - total, (off_t) (offset + total));
        if (bytes_read <= 0) {
            break;
        }

        total += (uint64) bytes_read;
    }

    close(fp);

    file->content[total] = '\0';
    file->size = total;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, total);
    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_IO, total);
}

template <typename T>
inline
void file_read(const char* __restrict path, FileBody* __restrict file, T* const __restrict mem) NO_EXCEPT
{
    file_read(path, file, 0, mem);
}

// file->content must already be allocated (file->size = 0: large enough for the whole file + '\0')
inline
void file_read(const char* __restrict path, FileBody* __restrict file) NO_EXCEPT
{
    file_read(path, file, 0, (BufferMemory *) NULL);
}

template <typename T>
inline
void file_read(const wchar_t* __restrict path, FileBody* __restrict file, uint64 offset, T* const __restrict mem) NO_EXCEPT
{
    char buffer[PATH_MAX_LENGTH];
    file_read(file_path_utf8(path, buffer), file, offset, mem);
}

template <typename T>
inline
void file_read(const wchar_t* __restrict path, FileBody* __restrict file, T* const __restrict mem) NO_EXCEPT
{
    file_read(path, file, 0, mem);
}

inline
void file_read(const wchar_t* __restrict path, FileBody* __restrict file) NO_EXCEPT
{
    file_read(path, file, 0, (BufferMemory *) NULL);
}

// This function uses a couple of temporary/internal variables to keep track of state and data for consecutive calls
// The alternative would be to correct the file position after almost every call using seek which is very inefficient.
// Since the mentality of this function is to be called consecutively we do it this way.
bool file_read_line(
    FileHandle fp,
    char* __restrict line_buffer, size_t buffer_size,
    char internal_buffer[512], ssize_t* __restrict internal_buffer_size, char** internal_pos
) {
    if (!(*internal_pos)) {
        *internal_pos = internal_buffer;
    }

    size_t line_filled = 0;

    while (line_filled < buffer_size - 1) {
        // Refill the internal buffer if empty
        if (*internal_pos == internal_buffer + *internal_buffer_size) {
            *internal_buffer_size = read(fp, internal_buffer, 512);
            if (*internal_buffer_size <= 0) {
                line_buffer[line_filled] = '\0';

                return line_filled > 0;
            }

            *internal_pos = internal_buffer;
        }

        char current_char = **internal_pos;
        ++(*internal_pos);

        // Handle line endings (\n, \r, \r\n, \n\r)
        if (current_char == '\n' || current_char == '\r') {
            if ((*internal_pos < internal_buffer + *internal_buffer_size)
                && (**internal_pos == '\n' || **internal_pos == '\r')
                && **internal_pos != current_char
            ) {
                ++(*internal_pos);
            }

            line_buffer[line_filled] = '\0';

            // Successfully read a line
            return true;
        }

        line_buffer[line_filled++] = current_char;
    }

    line_buffer[line_filled] = '\0';

    return true;
}

inline
bool file_write(const char* __restrict path, const FileBody* __restrict file) {
    PROFILE_DEBUG(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    int32 fd;
    char full_path[PATH_MAX_LENGTH];

    if (*path == '.') {
        relative_to_absolute(path, full_path);
        path = full_path;
    }

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        return false;
    }

    ASSERT_TRUE(file->size < MAX_UINT32);

    ssize_t written = write(fd, file->content, file->size);
    if (written < 0 || (size_t) written != file->size) {
        close(fd);
        return false;
    }

    if (close(fd) < 0) {
        return false;
    }

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, written);

    return true;
}

FileHandle file_read_handle(const char* path) {
    FileHandle fd;

    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);
        fd = open(full_path, O_RDONLY);
    } else {
        fd = open(path, O_RDONLY);
    }

    // 0 on failure, same as win32 + file_read_handle_shared() (the callers check !fd)
    if (fd == -1) {
        return 0;
    }

    return fd;
}

FORCE_INLINE
void file_close_handle(FileHandle fp)
{
    close(fp);
}

void iterate_directory(const char* base_path, const char* file_ending, void (*handler)(const char *, va_list), ...) {
    va_list args;
    va_start(args, handler);

    char full_base_path[PATH_MAX_LENGTH];
    relative_to_absolute(base_path, full_base_path);

    DIR* dir = opendir(full_base_path);
    if (!dir) {
        return;
    }

    const dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.'
            && (entry->d_name[1] == '\0'
                || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')
            )
        ) {
            continue;
        }

        char full_path[PATH_MAX_LENGTH];
        int32 full_length = (int32) str_length(base_path);
        str_copy(full_path, base_path, full_length);
        if (!full_length || full_path[full_length - 1] != '/') {
            full_path[full_length++] = '/';
        }
        str_copy(full_path + full_length, entry->d_name);

        char full_file_path[PATH_MAX_LENGTH];
        relative_to_absolute(full_path, full_file_path);

        struct stat statbuf;
        if (stat(full_file_path, &statbuf) == -1) {
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            iterate_directory(full_path, file_ending, handler, args);
        } else if (str_ends_with(full_path, file_ending)) {
            handler(full_path, args);
        }
    }

    closedir(dir);

    va_end(args);
}

// Relative paths ("./...") are relative to the executable, same as the other functions in this file
static inline
const char* file_path_resolve(const char* path, char* buffer) NO_EXCEPT
{
    if (*path != '.') {
        return path;
    }

    relative_to_absolute(path, buffer);

    return buffer;
}

/**
 * Same as file_read_handle() but doesn't fail if the file doesn't exist yet
 * (Linux doesn't lock opened files, other processes may always write, replace or delete it)
 *
 * @return FileHandle 0 on failure
 */
inline
FileHandle file_read_handle_shared(const char* path) NO_EXCEPT
{
    char full_path[PATH_MAX_LENGTH];
    const int32 fd = open(file_path_resolve(path, full_path), O_RDONLY);

    return fd < 0 ? 0 : fd;
}

/**
 * Opens a file for reading and writing (e.g. random access with file_read_at() / file_write_at())
 *
 * @param const char*   path    File path
 * @param FileOpenMode  mode    Open mode
 *
 * @return FileHandle 0 on failure
 */
inline
FileHandle file_read_write_handle(const char* path, FileOpenMode mode) NO_EXCEPT
{
    int32 flags = O_RDWR;
    if (mode == FILE_OPEN_MODE_ALWAYS) {
        flags |= O_CREAT;
    } else if (mode == FILE_OPEN_MODE_TRUNCATE) {
        flags |= O_CREAT | O_TRUNC;
    }

    char full_path[PATH_MAX_LENGTH];
    const int32 fd = open(file_path_resolve(path, full_path), flags, 0644);

    return fd < 0 ? 0 : fd;
}

FORCE_INLINE
uint64 file_size(FileHandle fp) NO_EXCEPT
{
    struct stat st;
    if (fstat(fp, &st) != 0) {
        return 0;
    }

    return (uint64) st.st_size;
}

/**
 * Reads at a file position without using/changing a shared file position
 * (unlike file_read() no null termination)
 *
 * @return uint32 Bytes read (less than length at the end of the file or on failure)
 */
inline
uint32 file_read_at(FileHandle fp, uint64 offset, void* buffer, uint32 length) NO_EXCEPT
{
    uint32 total = 0;

    while (total < length) {
        const ssize_t bytes_read = pread(fp, (byte *) buffer + total, length - total, (off_t) (offset + total));
        if (bytes_read <= 0) {
            break;
        }

        total += (uint32) bytes_read;
    }

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, total);
    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_IO, total);

    return total;
}

/**
 * Writes at a file position, the file grows if necessary
 *
 * @return bool
 */
inline
bool file_write_at(FileHandle fp, uint64 offset, const void* buffer, uint32 length) NO_EXCEPT
{
    uint32 total = 0;

    while (total < length) {
        const ssize_t written = pwrite(fp, (const byte *) buffer + total, length - total, (off_t) (offset + total));
        if (written <= 0) {
            return false;
        }

        total += (uint32) written;
    }

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, total);
    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_IO, total);

    return true;
}

// Flushes the written data to the drive
FORCE_INLINE
void file_flush(FileHandle fp) NO_EXCEPT
{
    fsync(fp);
}

/**
 * Replaces dst with src (atomic on the same volume)
 * An existing dst is overwritten
 */
inline
bool file_replace(const char* __restrict src, const char* __restrict dst) NO_EXCEPT
{
    char src_full_path[PATH_MAX_LENGTH];
    char dst_full_path[PATH_MAX_LENGTH];

    return rename(file_path_resolve(src, src_full_path), file_path_resolve(dst, dst_full_path)) == 0;
}

inline
bool file_delete(const char* path) NO_EXCEPT
{
    char full_path[PATH_MAX_LENGTH];

    return unlink(file_path_resolve(path, full_path)) == 0;
}

inline
bool directory_exists(const char* path) NO_EXCEPT
{
    char full_path[PATH_MAX_LENGTH];

    struct stat st;
    return stat(file_path_resolve(path, full_path), &st) == 0 && S_ISDIR(st.st_mode);
}

/**
 * Creates a directory including all missing parent directories
 *
 * @return bool true if the directory exists afterwards
 */
inline
bool directory_tree_create(const char* path) NO_EXCEPT
{
    char full_path[PATH_MAX_LENGTH];
    const char* abs_path = file_path_resolve(path, full_path);

    char buffer[PATH_MAX_LENGTH];
    str_copy(buffer, abs_path);

    for (char* pos = buffer + 1; *pos; ++pos) {
        if (*pos != '/') {
            continue;
        }

        *pos = '\0';
        if (mkdir(buffer, 0755) != 0 && !directory_exists(buffer)) {
            return false;
        }

        *pos = '/';
    }

    return (mkdir(buffer, 0755) == 0) || directory_exists(buffer);
}

/**
 * Lists the direct sub directories of a directory (not recursive, hidden "." directories are skipped)
 *
 * @param const char*   path        Directory path
 * @param char*         names       Output names (count * name_size chars)
 * @param int32         name_size   Max. size of a name incl. the null terminator (longer names are skipped)
 * @param int32         count       Max. number of names
 *
 * @return int32 Number of names written
 */
inline
int32 directory_list_subdirectories(const char* path, char* names, int32 name_size, int32 count) NO_EXCEPT
{
    char full_path[PATH_MAX_LENGTH];
    const char* abs_path = file_path_resolve(path, full_path);

    DIR* dir = opendir(abs_path);
    if (!dir) {
        return 0;
    }

    int32 found = 0;
    const dirent* entry;
    while (found < count && (entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        char entry_path[PATH_MAX_LENGTH];
        sprintf_fast(entry_path, sizeof(entry_path), "%s/%s", abs_path, entry->d_name);

        struct stat st;
        if (stat(entry_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
            continue;
        }

        const size_t name_length = str_length(entry->d_name);
        if ((int32) name_length >= name_size) {
            continue;
        }

        memcpy(names + found * name_size, entry->d_name, name_length + 1);
        ++found;
    }

    closedir(dir);

    return found;
}

/////////////////////////////////////////////////////////////////////////
// Same api as platform/win32/FileUtils.cpp
/////////////////////////////////////////////////////////////////////////

FORCE_INLINE
void file_seek(FileHandle fh, uint64 pos) NO_EXCEPT
{
    lseek(fh, (off_t) pos, SEEK_SET);
}

/**
 * Reads from an open file into file->content (must be allocated, max. length + 1 bytes)
 * The content is null terminated
 */
inline
void file_read(
    FileHandle fp,
    FileBody* __restrict file,
    uint64 offset = 0,
    uint64 length = MAX_UINT64
) NO_EXCEPT
{
    const uint64 fsize = file_size(fp);
    if (offset >= fsize) {
        file->size = 0;
        file->content = NULL;
        ASSERT_THROW();

        return;
    }

    const uint64 read_length = OMS_MIN(length, fsize - offset);

    uint64 total = 0;
    while (total < read_length) {
        const ssize_t bytes_read = pread(fp, file->content + total, read_length - total, (off_t) (offset + total));
        if (bytes_read <= 0) {
            break;
        }

        total += (uint64) bytes_read;
    }

    file->content[total] = '\0';
    file->size = total;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, total);
    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_IO, total);
}

inline
uint64 file_count_lines(FileHandle fp, uint64 offset = 0, uint64 length = MAX_UINT64) NO_EXCEPT
{
    const uint64 fsize = file_size(fp);
    if (offset >= fsize) {
        return 0;
    }

    const uint64 read_length = OMS_MIN(length, fsize - offset);

    byte buffer[64 * 1024];
    uint64 total = 0;
    uint64 line_count = 0;

    while (total < read_length) {
        const uint64 to_read = OMS_MIN(read_length - total, (uint64) sizeof(buffer));
        const ssize_t bytes_read = pread(fp, buffer, to_read, (off_t) (offset + total));
        if (bytes_read <= 0) {
            break;
        }

        for (ssize_t i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                ++line_count;
            }
        }

        total += (uint64) bytes_read;
    }

    return line_count;
}

/**
 * Moves a file, the directories of dst are created
 * An existing dst is NOT overwritten (the move fails), same as win32 (@see file_replace)
 */
inline
bool file_move(const char* __restrict src, const char* __restrict dst) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_FILE_UTILS, src, PROFILE_FLAG_SHOULD_LOG);

    char src_full_path[PATH_MAX_LENGTH];
    char dst_full_path[PATH_MAX_LENGTH];
    relative_to_absolute(src, src_full_path);
    relative_to_absolute(dst, dst_full_path);

    // Directory of dst
    char directory[PATH_MAX_LENGTH];
    str_copy(directory, dst_full_path);

    char* last_slash = NULL;
    for (char* pos = directory; *pos; ++pos) {
        if (*pos == '/') {
            last_slash = pos;
        }
    }

    if (last_slash && last_slash != directory) {
        *last_slash = '\0';
        directory_tree_create(directory);
    }

    struct stat st;
    if (stat(dst_full_path, &st) == 0) {
        return false;
    }

    return rename(src_full_path, dst_full_path) == 0;
}

inline
bool file_append(FileHandle fp, const char* __restrict file, size_t length) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_FILE_UTILS, file, PROFILE_FLAG_SHOULD_LOG);

    if (fp < 0) {
        ASSERT_THROW();

        return false;
    }

    size_t total = 0;
    while (total < length) {
        const ssize_t written = write(fp, file + total, length - total);
        if (written <= 0) {
            ASSERT_THROW();

            return false;
        }

        total += (size_t) written;
    }

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, total);
    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_IO, total);

    return true;
}

FORCE_INLINE
bool file_append(FileHandle fp, const char* __restrict file) NO_EXCEPT
{
    return file_append(fp, file, str_length(file));
}

// Appends text to an existing file (fails if the file doesn't exist, same as win32)
inline
bool file_append(const char* __restrict path, const char* __restrict file) NO_EXCEPT
{
    char full_path[PATH_MAX_LENGTH];
    relative_to_absolute(path, full_path);

    const int32 fp = open(full_path, O_WRONLY | O_APPEND);
    if (fp < 0) {
        return false;
    }

    const bool result = file_append(fp, file);
    close(fp);

    return result;
}

// Async reads: Linux reads synchronously (pread), the handles are normal file descriptors
FORCE_INLINE
FileHandle file_read_async_handle(const char* path) NO_EXCEPT
{
    return file_read_handle(path);
}

FORCE_INLINE
FileHandle file_read_async_handle_shared(const char* path) NO_EXCEPT
{
    return file_read_handle_shared(path);
}

/**
 * Reads into file->content (must be allocated, max. length + 1 bytes)
 * Synchronous on Linux, file_async_wait() only returns the result
 *
 * @return bool true if something was read
 */
inline
bool file_read_async(
    FileHandle fp,
    FileBodyAsync* __restrict file,
    uint64 offset = 0,
    uint64 length = MAX_UINT64
) NO_EXCEPT
{
    file->size = 0;

    if (!file->content) {
        ASSERT_THROW();

        return false;
    }

    const uint64 fsize = file_size(fp);
    if (offset >= fsize) {
        ASSERT_THROW();

        return false;
    }

    const uint64 read_length = OMS_MIN(length, fsize - offset);

    uint64 total = 0;
    while (total < read_length) {
        const ssize_t bytes_read = pread(fp, file->content + total, read_length - total, (off_t) (offset + total));
        if (bytes_read <= 0) {
            break;
        }

        total += (uint64) bytes_read;
    }

    file->content[total] = '\0';
    file->size = total;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, total);
    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_IO, total);

    return total > 0;
}

// Same as above, the content is allocated from mem (read length + 1 bytes)
template <typename T>
inline
bool file_read_async(
    FileHandle fp,
    FileBodyAsync* __restrict file,
    uint64 offset,
    uint64 length,
    T* const __restrict mem
) NO_EXCEPT
{
    file->size = 0;

    const uint64 fsize = file_size(fp);
    if (offset >= fsize) {
        file->content = NULL;

        return false;
    }

    if (mem != NULL) {
        file->content = memory_get(mem, OMS_MIN(length, fsize - offset) + 1);
    }

    return file_read_async(fp, file, offset, length);
}

// The read already completed in file_read_async()
FORCE_INLINE
bool file_async_wait(FileHandle, FileBodyAsync* const file, bool = true) NO_EXCEPT
{
    return file->size > 0;
}

/////////////////////////////////////////////////////////////////////////
// Wide paths (windows style api, e.g. AssetArchive::path), converted to utf-8
/////////////////////////////////////////////////////////////////////////

inline
bool file_exists(const wchar_t* path) NO_EXCEPT
{
    char buffer[PATH_MAX_LENGTH];

    return file_exists(file_path_utf8(path, buffer));
}

inline
size_t file_size(const wchar_t* path) NO_EXCEPT
{
    char buffer[PATH_MAX_LENGTH];

    return file_size(file_path_utf8(path, buffer));
}

inline
uint64 file_last_modified(const wchar_t* path) NO_EXCEPT
{
    char buffer[PATH_MAX_LENGTH];

    return file_last_modified(file_path_utf8(path, buffer));
}

inline
bool file_write(const wchar_t* __restrict path, const FileBody* __restrict file) NO_EXCEPT
{
    char buffer[PATH_MAX_LENGTH];

    return file_write(file_path_utf8(path, buffer), file);
}

inline
bool file_delete(const wchar_t* path) NO_EXCEPT
{
    char buffer[PATH_MAX_LENGTH];

    return file_delete(file_path_utf8(path, buffer));
}

inline
FileHandle file_read_handle(const wchar_t* path) NO_EXCEPT
{
    char buffer[PATH_MAX_LENGTH];

    return file_read_handle(file_path_utf8(path, buffer));
}

inline
FileHandle file_read_handle_shared(const wchar_t* path) NO_EXCEPT
{
    char buffer[PATH_MAX_LENGTH];

    return file_read_handle_shared(file_path_utf8(path, buffer));
}

FORCE_INLINE
FileHandle file_read_async_handle(const wchar_t* path) NO_EXCEPT
{
    return file_read_handle(path);
}

FORCE_INLINE
FileHandle file_read_async_handle_shared(const wchar_t* path) NO_EXCEPT
{
    return file_read_handle_shared(path);
}

#endif
