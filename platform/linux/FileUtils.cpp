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

inline
void relative_to_absolute(const char* __restrict rel, char* __restrict path)
{
    char self_path[PATH_MAX_LENGTH];
    int32 self_path_length = readlink("/proc/self/exe", self_path, PATH_MAX_LENGTH - 1);
    if (self_path_length == -1) {
        return;
    }

    const char* temp = rel;
    if (temp[0] == '.' && temp[1] == '/') {
        temp += 2;
    }

    // readlink doesn't null terminate, the last character is self_path[self_path_length - 1]
    char* last = self_path + self_path_length - 1;
    --self_path_length;
    while (*last != '/' && self_path_length > 0) {
        --last;
        --self_path_length;
    }

    ++self_path_length;

    memcpy(path, self_path, self_path_length);
    str_copy(path + self_path_length, temp);
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
    PROFILE_DEBUG(PROFILE_FILE_UTILS, path, false, true);

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
    PROFILE_DEBUG(PROFILE_FILE_UTILS, src, false, true);

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

template <typename T>
inline
void file_read(
    const char* __restrict path,
    FileBody* __restrict file,
    T* const __restrict mem = NULL
) {
    PROFILE_DEBUG(PROFILE_FILE_UTILS, path, false, true);

    char full_path[PATH_MAX_LENGTH];
    const char* abs_path = path;

    if (*path == '.') {
        relative_to_absolute(path, full_path);
        abs_path = full_path;
    }

    int32 fp = open(abs_path, O_RDONLY);
    if (fp < 0) {
        file->size = 0;
        file->content = NULL;

        return;
    }

    if (file->size == 0) {
        struct stat file_stat;
        if (fstat(fp, &file_stat) == -1) {
            close(fp);
            file->size = 0;
            file->content = NULL;

            return;
        }

        if (file_stat.st_size > MAX_UINT32) {
            close(fp);
            file->size = 0;
            file->content = NULL;

            return;
        }

        file->size = file_stat.st_size;
    }

    if (mem != NULL) {
        file->content = memory_get(mem, file->size + 1);
    }

    ssize_t bytes_read = read(fp, file->content, file->size);
    if (bytes_read <= 0) {
        close(fp);
        file->content = NULL;
        file->size = 0;

        return;
    }

    file->content[bytes_read] = '\0';
    file->size = bytes_read;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);

    close(fp);
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
    PROFILE_DEBUG(PROFILE_FILE_UTILS, path, false, true);

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

    if (fd == -1) {
        return -1;
    }

    return fd;
}

FORCE_INLINE
void file_close_handle(FileHandle fp)
{
    close(fp);
}

inline
void self_directory_path(char* path) {
    size_t len = readlink("/proc/self/exe", path, PATH_MAX_LENGTH);
    if (len > 0) LIKELY {
        path[len] = '\0';
    } else {
        path[0] = '\0';
    }
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
        // @performance This is bad, we are internally moving two times too often to the end of full_path
        //      Maybe make str_copy return the length, same as append?
        str_copy(full_path, base_path);
        if (!str_ends_with(base_path, "/")) {
            strcat(full_path, "/");
        }
        strcat(full_path, entry->d_name);

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
 * @param const char* path File path
 * @param FileOpenMode mode Open mode
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
 * @param const char* path Directory path
 * @param char* names Output names (count * name_size chars)
 * @param int32 name_size Max. size of a name incl. the null terminator (longer names are skipped)
 * @param int32 count Max. number of names
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
        snprintf(entry_path, sizeof(entry_path), "%s/%s", abs_path, entry->d_name);

        struct stat st;
        if (stat(entry_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
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

    return found;
}

#endif
