/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_FILE_UTILS_C
#define COMS_PLATFORM_WIN32_FILE_UTILS_C

#include <stdio.h>
#include <windows.h>
#include <string.h>

#ifdef _MSC_VER
    #include  <io.h>
#endif

#include "../../stdlib/Types.h"
#include "../../utils/Utils.h"
#include "../../utils/TestUtils.h"
#include "../../memory/RingMemory.h"
#include "../../log/Stats.h"
#include "../../log/PerformanceProfiler.h"

typedef HANDLE FileHandle;
typedef HANDLE MMFHandle;
typedef OVERLAPPED file_overlapped;

struct FileBodyAsync {
    // doesn't include null termination (same as str_length)
    uint64 size;
    byte* content;
    OVERLAPPED ov;
};

inline
MMFHandle file_mmf_handle(FileHandle fp)
{
    return CreateFileMappingA(fp, NULL, PAGE_READONLY, 0, 0, NULL);
}

inline
void* mmf_region_init(MMFHandle fh, size_t offset, size_t length = 0)
{
    DWORD high = (DWORD) ((offset >> 32) & 0xFFFFFFFF);
    DWORD low = (DWORD) (offset & 0xFFFFFFFF);

    return MapViewOfFile(fh, FILE_MAP_READ, high, low, length);
}

inline
void mmf_region_release(void* fh) {
    UnmapViewOfFile(fh);
}

inline
void file_mmf_close(MMFHandle fh) {
    CloseHandle(fh);
}

inline
void relative_to_absolute(const char* __restrict rel, char* __restrict path)
{
    char self_path[MAX_PATH];
    int32 self_path_length = GetModuleFileNameA(NULL, self_path, MAX_PATH);
    if (self_path_length == 0) {
        return;
    }

    const char* temp = rel;
    if (temp[0] == '.' && temp[1] == '/') {
        temp += 2;
    }

    char* last = self_path + self_path_length;
    while (*last != '\\' && self_path_length > 0) {
        --last;
        --self_path_length;
    }

    ++self_path_length;

    memcpy(path, self_path, self_path_length);
    str_copy_short(path + self_path_length, temp);
}

inline uint64
file_size(const char* path)
{
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    // @performance Profile against fseek strategy
    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = CreateFileA((LPCSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileA((LPCSTR) path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    if (fp == INVALID_HANDLE_VALUE) {
        return 0;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(fp, &size)) {
        CloseHandle(fp);
    }

    CloseHandle(fp);

    return size.QuadPart;
}

inline
bool file_exists(const char* path)
{
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    DWORD file_attr;

    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        file_attr = GetFileAttributesA(full_path);
    } else {
        file_attr = GetFileAttributesA(path);
    }

    return file_attr != INVALID_FILE_ATTRIBUTES;
}

inline void
file_read(const char* __restrict path, FileBody* __restrict file, RingMemory* __restrict ring = NULL)
{
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = CreateFileA((LPCSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileA((LPCSTR) path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    if (fp == INVALID_HANDLE_VALUE) {
        file->size = 0;
        return;
    }

    if (file->size == 0) {
        LARGE_INTEGER size;
        if (!GetFileSizeEx(fp, &size)) {
            CloseHandle(fp);
            file->content = NULL;

            return;
        }

        file->size = size.QuadPart;
    }

    if (ring != NULL) {
        file->content = ring_get_memory(ring, file->size + 1);
    }

    DWORD bytes_read;
    if (!ReadFile(fp, file->content, (uint32) file->size, &bytes_read, NULL)) {
        CloseHandle(fp);
        file->content = NULL;

        return;
    }

    CloseHandle(fp);

    file->content[bytes_read] = '\0';
    file->size = bytes_read;

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
}

// @question Do we really need length? we have file.size we could use as we do in a function above
inline
void file_read(const char* __restrict path, FileBody* __restrict file, uint64 offset, uint64 length = MAX_UINT64, RingMemory* __restrict ring = NULL)
{
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = CreateFileA((LPCSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileA((LPCSTR) path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    if (fp == INVALID_HANDLE_VALUE) {
        file->size = 0;

        return;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(fp, &size)) {
        CloseHandle(fp);
        file->content = NULL;

        return;
    }

    // Ensure the offset and length do not exceed the file size
    uint64 file_size = size.QuadPart;
    if (offset >= file_size) {
        file->size = 0;
        file->content = NULL;
        CloseHandle(fp);

        return;
    }

    // Adjust the length to read so that it does not exceed the file size
    uint64 read_length = OMS_MIN(length, file_size - offset);

    if (ring != NULL) {
        file->content = ring_get_memory(ring, read_length + 1);
    }

    // Move the file pointer to the offset position
    LARGE_INTEGER li;
    li.QuadPart = offset;
    if (SetFilePointerEx(fp, li, NULL, FILE_BEGIN) == 0) {
        CloseHandle(fp);
        file->content = NULL;

        return;
    }

    DWORD bytes_read;
    if (!ReadFile(fp, file->content, (uint32) read_length, &bytes_read, NULL)) {
        CloseHandle(fp);
        file->content = NULL;

        return;
    }

    CloseHandle(fp);

    file->content[bytes_read] = '\0';
    file->size = bytes_read;

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
}

inline
void file_read(FileHandle fp, FileBody* __restrict file, uint64 offset = 0, uint64 length = MAX_UINT64, RingMemory* __restrict ring = NULL)
{
    LARGE_INTEGER size;
    if (!GetFileSizeEx(fp, &size)) {
        file->content = NULL;
        ASSERT_SIMPLE(false);

        return;
    }

    // Ensure the offset and length do not exceed the file size
    uint64 file_size = size.QuadPart;
    if (offset >= file_size) {
        file->size = 0;
        file->content = NULL;
        ASSERT_SIMPLE(false);

        return;
    }

    // Adjust the length to read so that it does not exceed the file size
    uint64 read_length = OMS_MIN(length, file_size - offset);

    if (ring != NULL) {
        file->content = ring_get_memory(ring, read_length + 1);
    }

    // Move the file pointer to the offset position
    LARGE_INTEGER li;
    li.QuadPart = offset;
    if (SetFilePointerEx(fp, li, NULL, FILE_BEGIN) == 0) {
        file->content = NULL;
        ASSERT_SIMPLE(false);

        return;
    }

    DWORD bytes_read;
    if (!ReadFile(fp, file->content, (uint32) read_length, &bytes_read, NULL)) {
        file->content = NULL;
        ASSERT_SIMPLE(false);

        return;
    }

    file->content[bytes_read] = '\0';
    file->size = bytes_read;

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
}

inline
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
            if (!ReadFile(fp, internal_buffer, 512, (DWORD *) internal_buffer_size, NULL)
                || *internal_buffer_size == 0
            ) {
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

inline bool
file_write(const char* __restrict path, const FileBody* __restrict file)
{
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = CreateFileA((LPCSTR) full_path,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileA((LPCSTR) path,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    if (fp == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written;
    DWORD length = (DWORD) file->size;
    if (!WriteFile(fp, file->content, length, &written, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, length);

    return true;
}

inline void
file_copy(const char* __restrict src, const char* __restrict dst)
{
    PROFILE(PROFILE_FILE_UTILS, src, false, true);

    if (*src == '.') {
        char src_full_path[MAX_PATH];
        relative_to_absolute(src, src_full_path);

        if (*dst == '.') {
            char dst_full_path[MAX_PATH];
            relative_to_absolute(dst, dst_full_path);

            CopyFileA((LPCSTR) src_full_path, (LPCSTR) dst_full_path, false);
        } else {
            CopyFileA((LPCSTR) src_full_path, (LPCSTR) dst, false);
        }
    } else if (*dst == '.') {
        char dst_full_path[MAX_PATH];
        relative_to_absolute(dst, dst_full_path);

        CopyFileA((LPCSTR) src, (LPCSTR) dst_full_path, false);
    } else {
        CopyFileA((LPCSTR) src, (LPCSTR) dst, false);
    }
}

inline
void file_close_handle(FileHandle fp)
{
    CloseHandle(fp);
}

inline
HANDLE file_append_handle(const char* path)
{
    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = CreateFileA((LPCSTR) full_path,
            FILE_APPEND_DATA,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileA((LPCSTR) path,
            FILE_APPEND_DATA,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    if (fp == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    return fp;
}

inline
bool file_read_async(
    FileHandle fp,
    FileBodyAsync* __restrict file,
    uint64_t offset = 0,
    uint64_t length = MAX_UINT64,
    RingMemory* __restrict ring = NULL
) {
    LARGE_INTEGER size;
    if (!GetFileSizeEx(fp, &size)) {
        file->content = NULL;
        ASSERT_SIMPLE(false);

        return false;
    }

    // Ensure the offset and length do not exceed the file size
    uint64_t file_size = size.QuadPart;
    if (offset >= file_size) {
        file->size = 0;
        file->content = NULL;
        ASSERT_SIMPLE(false);

        return false;
    }

    // Adjust the length to read so that it does not exceed the file size
    uint64 read_length = OMS_MIN(length, file_size - offset);

    // Allocate memory for the content
    if (ring != NULL) {
        file->content = ring_get_memory(ring, read_length);
    }

    if (!file->content) {
        ASSERT_SIMPLE(false);

        return false;
    }

    file->ov.Offset = (DWORD)(offset & 0xFFFFFFFF);
    file->ov.OffsetHigh = (DWORD)(offset >> 32);

    // Auto-reset event
    file->ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    DWORD bytes_read = 0;
    if (!ReadFile(fp, file->content, (DWORD) read_length, &bytes_read, &file->ov)) {
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING) {
            free(file->content);
            file->content = NULL;
            ASSERT_SIMPLE(false);

            return false;
        }
    }

    file->size = read_length;

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, read_length);

    return true;
}

inline
void file_async_wait(FileHandle fp, file_overlapped* overlapped, bool wait)
{
    DWORD bytesTransferred;
    GetOverlappedResult(fp, overlapped, &bytesTransferred, wait);
}

inline
FileHandle file_read_handle(const char* path)
{
    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = CreateFileA((LPCSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileA((LPCSTR) path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    if (fp == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    return fp;
}

inline
FileHandle file_read_async_handle(const char* path)
{
    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = CreateFileA((LPCSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
        );
    } else {
        fp = CreateFileA((LPCSTR) path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
        );
    }

    if (fp == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    return fp;
}

bool file_append(const char* __restrict path, const char* __restrict file)
{
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = CreateFileA((LPCSTR) full_path,
            FILE_APPEND_DATA,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileA((LPCSTR) path,
            FILE_APPEND_DATA,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    if (fp == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written;
    DWORD length = (DWORD) str_length(file);
    if (!WriteFile(fp, file, length, &written, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, written);

    return true;
}

inline bool
file_append(FileHandle fp, const char* file)
{
    PROFILE(PROFILE_FILE_UTILS, file, false, true);

    if (fp == INVALID_HANDLE_VALUE) {
        ASSERT_SIMPLE(false);
        return false;
    }

    DWORD written;
    DWORD length = (DWORD) str_length(file);
    if (!WriteFile(fp, file, length, &written, NULL)) {
        ASSERT_SIMPLE(false);
        return false;
    }

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, written);

    return true;
}

inline bool
file_append(FileHandle fp, const char* file, size_t length)
{
    PROFILE(PROFILE_FILE_UTILS, file, false, true);

    if (fp == INVALID_HANDLE_VALUE) {
        ASSERT_SIMPLE(false);
        return false;
    }

    DWORD written;
    if (!WriteFile(fp, file, (uint32) length, &written, NULL)) {
        ASSERT_SIMPLE(false);
        return false;
    }

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, written);

    return true;
}

inline bool
file_append(const char* __restrict path, const FileBody* __restrict file)
{
    PROFILE(PROFILE_FILE_UTILS, path, false, true);

    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = CreateFileA((LPCSTR) full_path,
            FILE_APPEND_DATA,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileA((LPCSTR) path,
            FILE_APPEND_DATA,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    }

    if (fp == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytes;
    DWORD length = (DWORD) file->size;
    if (!WriteFile(fp, file->content, length, &bytes, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);

    LOG_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, bytes);

    return true;
}

inline
uint64 file_last_modified(const char* path)
{
    WIN32_FIND_DATA find_data;

    FileHandle fp;
    if (*path == '.') {
        char full_path[MAX_PATH];
        relative_to_absolute(path, full_path);

        fp = FindFirstFileA(full_path, (LPWIN32_FIND_DATAA) &find_data);
    } else {
        fp = FindFirstFileA(path, (LPWIN32_FIND_DATAA) &find_data);
    }

    FILETIME modified = {};
    if(fp != INVALID_HANDLE_VALUE) {
        modified = find_data.ftLastWriteTime;
        FindClose(fp);
    }

    ULARGE_INTEGER ull;
    ull.LowPart = modified.dwLowDateTime;
    ull.HighPart = modified.dwHighDateTime;

    return ull.QuadPart;
}

inline void self_path(char* path)
{
    GetModuleFileNameA(NULL, (LPSTR) path, MAX_PATH);
}

void iterate_directory(const char* base_path, const char* file_ending, void (*handler)(const char *, void *), ...) {
    va_list args;
    va_start(args, handler);

    char full_base_path[MAX_PATH];
    relative_to_absolute(base_path, full_base_path);

    WIN32_FIND_DATA find_file_data;
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", full_base_path);

    HANDLE hFind = FindFirstFile(search_path, &find_file_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (find_file_data.cFileName[0] == '.'
            && (find_file_data.cFileName[1] == '\0'
                || (find_file_data.cFileName[1] == '.' && find_file_data.cFileName[2] == '\0')
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
        str_concat_append(full_path, find_file_data.cFileName);

        if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            iterate_directory(full_path, file_ending, handler, args);
        } else if (str_ends_with(full_path, file_ending)) {
            handler(full_path, args);
        }
    } while (FindNextFile(hFind, &find_file_data) != 0);

    FindClose(hFind);

    va_end(args);
}

#endif