/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_FILE_UTILS_C
#define COMS_PLATFORM_LINUX_FILE_UTILS_C

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>

#include "../../stdlib/Types.h"
#include "../../utils/Utils.h"
#include "../../utils/TestUtils.h"
#include "../../utils/StringUtils.h"
#include "../../memory/RingMemory.h"
#include "../../log/PerformanceProfiler.h"

#ifndef MAX_PATH
    #define MAX_PATH PATH_MAX
#endif

typedef int32 FileHandle;
typedef int MMFHandle;

inline
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

inline
void file_mmf_close(MMFHandle fh) {
    close(fh);
}

inline
void relative_to_absolute(const char* __restrict rel, char* __restrict path)
{
    char self_path[MAX_PATH];
    int32 self_path_length = readlink("/proc/self/exe", self_path, MAX_PATH - 1);
    if (self_path_length == -1) {
        return;
    }

    const char* temp = rel;
    if (temp[0] == '.' && temp[1] == '/') {
        temp += 2;
    }

    char* last = self_path + self_path_length;
    while (*last != '/' && self_path_length > 0) {
        --last;
        --self_path_length;
    }

    ++self_path_length;

    memcpy(path, self_path, self_path_length);
    str_copy_short(path + self_path_length, temp);
}

// @todo implement relative path support, similar to UtilsWin32
inline
uint64 file_size(const char* filename) {
    struct stat buffer;

    if (*filename == '.') {
        char full_path[MAX_PATH];
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
        char full_path[MAX_PATH];
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
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = open(full_path, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    } else {
        fp = open(path, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    }

    return fp;
}

inline
bool file_exists(const char* path) noexcept {
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    struct stat buffer;
    const char* full_path = path;
    char abs_path[MAX_PATH];

    if (*path == '.') {
        relative_to_absolute(path, abs_path);
        full_path = abs_path;
    }

    return stat(full_path, &buffer) == 0;
}

inline
bool file_copy(const char* __restrict src, const char* __restrict dst) {
    PROFILE(PROFILE_FILE_UTILS, src, false, true);

    char src_full_path[MAX_PATH];
    char dst_full_path[MAX_PATH];

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

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, bytes_written);

    return success;
}

inline
void file_read(const char* __restrict path, FileBody* __restrict file, RingMemory* __restrict ring = NULL) {
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    char full_path[MAX_PATH];
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

    if (ring != NULL) {
        file->content = ring_get_memory(ring, file->size + 1);
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

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);

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
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    int32 fd;
    char full_path[PATH_MAX];

    if (*path == '.') {
        relative_to_absolute(path, full_path);
        path = full_path;
    }

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        return false;
    }

    ASSERT_SIMPLE(file->size < MAX_UINT32);

    ssize_t written = write(fd, file->content, file->size);
    if (written < 0 || (size_t) written != file->size) {
        close(fd);
        return false;
    }

    if (close(fd) < 0) {
        return false;
    }

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, written);

    return true;
}

FileHandle file_read_handle(const char* path) {
    FileHandle fd;
    char full_path[MAX_PATH];

    if (*path == '.') {
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
void self_path(char* path) {
    size_t len = readlink("/proc/self/exe", path, PATH_MAX);
    if (len > 0) { [[likely]]
        path[len] = '\0';
    } else {
        path[0] = '\0';
    }
}

void iterate_directory(const char* base_path, const char* file_ending, void (*handler)(const char *, va_list), ...) {
    va_list args;
    va_start(args, handler);

    char full_base_path[MAX_PATH];
    relative_to_absolute(base_path, full_base_path);

    DIR* dir = opendir(full_base_path);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.'
            && (entry->d_name[1] == '\0'
                || (entry->d_name[1] == '.' && entry->d_name[2] == '\0')
            )
        ) {
            continue;
        }

        char full_path[MAX_PATH];
        // @performance This is bad, we are internally moving two times too often to the end of full_path
        //      Maybe make str_copy_short return the length, same as append?
        str_copy_short(full_path, base_path);
        if (!str_ends_with(base_path, "/")) {
            str_concat_append(full_path, "/");
        }
        str_concat_append(full_path, entry->d_name);

        char full_file_path[MAX_PATH];
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

#endif