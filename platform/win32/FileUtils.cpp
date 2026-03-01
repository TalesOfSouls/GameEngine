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

#include <windows.h>

#ifdef _MSC_VER
    #include  <io.h>
#endif

#include "../../stdlib/Stdlib.h"
#include "../../utils/Assert.h"
#include "../../memory/RingMemory.cpp"
#include "../../log/Stats.h"
#include "../../log/PerformanceProfiler.h"

#include "FileUtils.h"

// @todo create some simple wrapper functions that create file pointers for:
//  1. game data location
//  2. save data location
//  3. download location

FORCE_INLINE
MMFHandle file_mmf_handle(FileHandle fp) NO_EXCEPT
{
    STATS_INCREMENT_PERSISTENT(DEBUG_COUNTER_FILE_HANDLE_COUNT);
    return CreateFileMappingA(fp, NULL, PAGE_READONLY, 0, 0, NULL);
}

FORCE_INLINE
void* mmf_region_init(MMFHandle fh, size_t offset, size_t length = 0) NO_EXCEPT
{
    const DWORD high = (DWORD) ((offset >> 32) & 0xFFFFFFFF);
    const DWORD low = (DWORD) (offset & 0xFFFFFFFF);

    return MapViewOfFile(fh, FILE_MAP_READ, high, low, length);
}

FORCE_INLINE
void mmf_region_release(void* fh) NO_EXCEPT
{
    UnmapViewOfFile(fh);
}

FORCE_INLINE
void file_mmf_close(MMFHandle fh) NO_EXCEPT
{
    CloseHandle(fh);
    STATS_DECREMENT_PERSISTENT(DEBUG_COUNTER_FILE_HANDLE_COUNT);
}

inline
void relative_to_absolute(const char* __restrict rel, char* __restrict path) NO_EXCEPT
{
    if (rel[0] != '.') {
        strcpy(path, rel);
        return;
    }

    char spath[PATH_MAX_LENGTH];
    int32 spath_length = GetModuleFileNameA(NULL, spath, PATH_MAX_LENGTH);
    if (spath_length == 0) {
        return;
    }

    const char* temp = rel;
    if (temp[0] == '.' && temp[1] == '/') {
        temp += 2;
    }

    char* last = spath + spath_length;
    while (*last != '\\' && *last != '/' && spath_length > 0) {
        --last;
        --spath_length;
    }

    ++spath_length;

    memcpy(path, spath, spath_length);
    strcpy(path + spath_length, temp);
}

inline
int32 self_file_path(wchar_t* path)
{
    return (int32) GetModuleFileNameW(NULL, path, PATH_MAX_LENGTH);
}

inline
void relative_to_absolute(
    const wchar_t* __restrict rel,
    wchar_t* __restrict path
) NO_EXCEPT
{
    if (rel[0] != L'.') {
        wcscpy(path, rel);
        return;
    }

    wchar_t spath[PATH_MAX_LENGTH];
    int32 spath_length = GetModuleFileNameW(NULL, spath, PATH_MAX_LENGTH);
    if (spath_length == 0) {
        return;
    }

    const wchar_t* temp = rel;
    if (temp[0] == L'.' && temp[1] == L'/') {
        temp += 2;
    }

    wchar_t* last = spath + spath_length;
    while (*last != L'\\' && *last != L'/' && spath_length > 0) {
        --last;
        --spath_length;
    }

    ++spath_length;

    memcpy(path, spath, spath_length * sizeof(wchar_t));

    wcscpy(path + spath_length, temp);
}

inline
bool directory_tree_create(const wchar_t* path) NO_EXCEPT
{
    wchar_t buffer[PATH_MAX_LENGTH];
    relative_to_absolute(path, buffer);

    // If it already exists and is a directory = success
    DWORD attr = GetFileAttributesW(buffer);
    if (attr != INVALID_FILE_ATTRIBUTES) {
        return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    wchar_t* p = buffer;

    // Skip drive letter (e.g. "C:\")
    if (p[0] && p[1] == L':') {
        p += 2;
    }

    // Skip UNC prefix (\\server\share\)
    if (p[0] == L'\\' && p[1] == L'\\') {
        p += 2;
        while (*p && *p != L'\\') {
            ++p; // server
        }

        if (*p) {
            ++p;
        }

        while (*p && *p != L'\\') {
            ++p; // share
        }
    }

    for (; *p; ++p) {
        if (*p == L'\\' || *p == L'/') {
            wchar_t old = *p;
            *p = 0;

            if (*buffer) {
                DWORD a = GetFileAttributesW(buffer);
                if (a == INVALID_FILE_ATTRIBUTES) {
                    if (!CreateDirectoryW(buffer, nullptr)) {
                        if (GetLastError() != ERROR_ALREADY_EXISTS) {
                            return false;
                        }
                    }
                } else if (!(a & FILE_ATTRIBUTE_DIRECTORY)) {
                    // exists but not a directory
                    return false;
                }
            }

            *p = old;
        }
    }

    // Create final directory
    DWORD a = GetFileAttributesW(buffer);
    if (a == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryW(buffer, nullptr)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                return false;
            }
        }
    } else if (!(a & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    return true;
}

inline
bool directory_tree_create(const char* path) NO_EXCEPT
{
    char buffer[PATH_MAX_LENGTH];
    relative_to_absolute(path, buffer);

    // If it already exists and is a directory = success
    DWORD attr = GetFileAttributesA(path);
    if (attr != INVALID_FILE_ATTRIBUTES) {
        return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    char* p = buffer;

    // Skip drive letter (e.g. "C:\")
    if (p[0] && p[1] == ':') {
        p += 2;
    }

    // Skip UNC prefix (\\server\share\)
    if (p[0] == '\\' && p[1] == '\\') {
        p += 2;
        while (*p && *p != '\\') {
            ++p; // server
        }

        if (*p) {
            ++p;
        }

        while (*p && *p != '\\') {
            ++p; // share
        }
    }

    for (; *p; ++p) {
        if (*p == '\\' || *p == '/') {
            char old = *p;
            *p = 0;

            if (*buffer) {
                DWORD a = GetFileAttributesA(buffer);
                if (a == INVALID_FILE_ATTRIBUTES) {
                    if (!CreateDirectoryA(buffer, nullptr)) {
                        if (GetLastError() != ERROR_ALREADY_EXISTS) {
                            return false;
                        }
                    }
                } else if (!(a & FILE_ATTRIBUTE_DIRECTORY)) {
                    // exists but not a directory
                    return false;
                }
            }

            *p = old;
        }
    }

    // Create final directory
    DWORD a = GetFileAttributesA(buffer);
    if (a == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryA(buffer, nullptr)) {
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                return false;
            }
        }
    } else if (!(a & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    return true;
}

FORCE_INLINE
void file_seek(FileHandle fh, uint64 pos) NO_EXCEPT
{
    LARGE_INTEGER li;
    li.QuadPart = pos;

    SetFilePointer(fh, li.LowPart, &li.HighPart, FILE_BEGIN);
}

inline size_t
file_size(const char* path) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    // @performance Profile against fseek strategy
    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
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

inline size_t
file_size(const wchar_t* path) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, NULL, PROFILE_FLAG_SHOULD_LOG);

    // @performance Profile against fseek strategy
    FileHandle fp;
    if (*path == '.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = CreateFileW((LPCWSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileW((LPCWSTR) path,
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
bool file_exists(const char* path) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    DWORD file_attr;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        file_attr = GetFileAttributesA(full_path);
    } else {
        file_attr = GetFileAttributesA(path);
    }

    return file_attr != INVALID_FILE_ATTRIBUTES;
}

inline
bool file_exists(const wchar_t* path) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, NULL, PROFILE_FLAG_SHOULD_LOG);

    DWORD file_attr;
    if (*path == L'.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        file_attr = GetFileAttributesW(full_path);
    } else {
        file_attr = GetFileAttributesW(path);
    }

    return file_attr != INVALID_FILE_ATTRIBUTES;
}

inline void
file_read(
    const char* __restrict path,
    FileBody* __restrict file,
    RingMemory* const __restrict ring = NULL
) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
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

    ASSERT_TRUE(bytes_read <= 2147483648);

    file->content[bytes_read] = '\0';
    file->size = bytes_read;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
}

inline void
file_read(
    const wchar_t* __restrict path,
    FileBody* __restrict file,
    RingMemory* const __restrict ring = NULL
) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, NULL, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == L'.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = CreateFileW((LPCWSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileW((LPCWSTR) path,
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

    ASSERT_TRUE(bytes_read <= 2147483648);

    file->content[bytes_read] = '\0';
    file->size = bytes_read;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
}

// @question Do we really need length? we have file.size we could use as we do in a function above
inline
void file_read(
    const char* __restrict path,
    FileBody* __restrict file,
    uint64 offset,
    uint64 length = MAX_UINT64,
    RingMemory* const __restrict ring = NULL
) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
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
    const uint64 fsize = size.QuadPart;
    if (offset >= fsize) {
        file->size = 0;
        file->content = NULL;
        CloseHandle(fp);

        return;
    }

    // Adjust the length to read so that it does not exceed the file size
    const uint64 read_length = OMS_MIN(length, fsize - offset);

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

    ASSERT_TRUE(bytes_read <= 2147483648);

    file->content[bytes_read] = '\0';
    file->size = bytes_read;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
}

inline
void file_read(
    const wchar_t* __restrict path,
    FileBody* __restrict file,
    uint64 offset,
    uint64 length = MAX_UINT64,
    RingMemory* const __restrict ring = NULL
) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, NULL, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == L'.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = CreateFileW((LPCWSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileW((LPCWSTR) path,
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
    const uint64 fsize = size.QuadPart;
    if (offset >= fsize) {
        file->size = 0;
        file->content = NULL;
        CloseHandle(fp);

        return;
    }

    // Adjust the length to read so that it does not exceed the file size
    const uint64 read_length = OMS_MIN(length, fsize - offset);

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

    ASSERT_TRUE(bytes_read <= 2147483648);

    file->content[bytes_read] = '\0';
    file->size = bytes_read;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
}

inline
void file_read(
    FileHandle fp,
    FileBody* __restrict file,
    uint64 offset = 0,
    uint64 length = MAX_UINT64,
    RingMemory* const __restrict ring = NULL
) NO_EXCEPT
{
    LARGE_INTEGER size;
    if (!GetFileSizeEx(fp, &size)) {
        file->content = NULL;
        ASSERT_TRUE(false);

        return;
    }

    // Ensure the offset and length do not exceed the file size
    const uint64 fsize = size.QuadPart;
    if (offset >= fsize) {
        file->size = 0;
        file->content = NULL;
        ASSERT_TRUE(false);

        return;
    }

    // Adjust the length to read so that it does not exceed the file size
    const uint64 read_length = OMS_MIN(length, fsize - offset);

    if (ring != NULL) {
        file->content = ring_get_memory(ring, read_length + 1);
    }

    // Move the file pointer to the offset position
    if (offset) {
        LARGE_INTEGER li;
        li.QuadPart = offset;
        if (SetFilePointerEx(fp, li, NULL, FILE_BEGIN) == 0) {
            file->content = NULL;
            ASSERT_TRUE(false);

            return;
        }
    }

    DWORD bytes_read;
    if (!ReadFile(fp, file->content, (uint32) read_length, &bytes_read, NULL)) {
        file->content = NULL;
        ASSERT_TRUE(false);

        return;
    }

    ASSERT_TRUE(bytes_read <= 2147483648);

    file->content[bytes_read] = '\0';
    file->size = bytes_read;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, bytes_read);
}

uint64 file_count_lines(FileHandle fp, uint64 offset = 0, uint64 length = MAX_UINT64) NO_EXCEPT
{
    LARGE_INTEGER size;
    if (!GetFileSizeEx(fp, &size)) {
        return 0;
    }

    const uint64 fsize = size.QuadPart;
    if (offset >= fsize) {
        return 0;
    }

    // Adjust the length to read so that it does not exceed the file size
    const uint64 read_length = OMS_MIN(length, fsize - offset);

    // Move file pointer
    if (offset) {
        LARGE_INTEGER li;
        li.QuadPart = offset;
        if (!SetFilePointerEx(fp, li, NULL, FILE_BEGIN)) {
            return 0;
        }
    }

    const DWORD chunk_size = 64 * 1024;
    BYTE buffer[chunk_size];
    DWORD bytes_read = 0;
    uint64 total_read = 0;
    uint64 line_count = 0;

    while (total_read < read_length) {
        DWORD to_read = (DWORD)((read_length - total_read) > chunk_size
            ? chunk_size
            : (read_length - total_read)
        );

        if (!ReadFile(fp, buffer, to_read, &bytes_read, NULL) || bytes_read == 0) {
            break;
        }

        for (DWORD i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                ++line_count;
            }
        }

        total_read += bytes_read;
    }

    return line_count;
}

inline
bool file_read_line(
    FileHandle fp,
    char* __restrict line_buffer, size_t buffer_size,
    char internal_buffer[512], ssize_t* __restrict internal_buffer_size, char** internal_pos
) NO_EXCEPT
{
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
file_write(const char* __restrict path, const FileBody* __restrict file) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
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

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, length);

    return true;
}

inline bool
file_write(const wchar_t* __restrict path, const FileBody* __restrict file) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, NULL, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == L'.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = CreateFileW((LPCWSTR) full_path,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileW((LPCWSTR) path,
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

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, length);

    return true;
}

inline bool
file_copy(const char* __restrict src, const char* __restrict dst) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, src, PROFILE_FLAG_SHOULD_LOG);

    char dst_full_path[PATH_MAX_LENGTH];
    relative_to_absolute(dst, dst_full_path);

    // Find last slash and check if the last path element is a file
    // We don't want to create the file name as a directory by accident
    char* last_slash = NULL;
    for (char* pos = dst_full_path; *pos; ++pos) {
        if (*pos == '\\' || *pos == '/') {
            last_slash = pos;
        }
    }

    // Remove . from path since the directory tree creation will otherwise create the file name as directory
    for (char* pos = last_slash; *pos; ++pos) {
        if (*pos == '.') {
            *last_slash = '\0';
            break;
        }
    }
    directory_tree_create(dst_full_path);

    // We re-add the previously removed .
    *last_slash = '\\';

    if (*src == '.') {
        char src_full_path[PATH_MAX_LENGTH];
        relative_to_absolute(src, src_full_path);

        return (bool) CopyFileA((LPCSTR) src_full_path, (LPCSTR) dst_full_path, false);
    } else {
        return (bool) CopyFileA((LPCSTR) src, (LPCSTR) dst_full_path, false);
    }
}

inline bool
file_copy(const wchar_t* __restrict src, const wchar_t* __restrict dst) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, NULL, PROFILE_FLAG_SHOULD_LOG);

    wchar_t dst_full_path[PATH_MAX_LENGTH];
    relative_to_absolute(dst, dst_full_path);

    // Find last slash and check if the last path element is a file
    // We don't want to create the file name as a directory by accident
    wchar_t* last_slash = NULL;
    for (wchar_t* pos = dst_full_path; *pos; ++pos) {
        if (*pos == L'\\' || *pos == L'/') {
            last_slash = pos;
        }
    }

    // Remove . from path since the directory tree creation will otherwise create the file name as directory
    for (wchar_t* pos = last_slash; *pos; ++pos) {
        if (*pos == L'.') {
            *last_slash = L'\0';
            break;
        }
    }
    directory_tree_create(dst_full_path);
    // We re-add the previously removed .
    *last_slash = L'\\';

    if (*src == L'.') {
        wchar_t src_full_path[PATH_MAX_LENGTH];
        relative_to_absolute(src, src_full_path);

        return (bool) CopyFileW(src_full_path, dst_full_path, FALSE);
    } else {
        return (bool) CopyFileW(src, dst_full_path, FALSE);
    }
}

inline bool directory_copy(
    const wchar_t* src,
    const wchar_t* dst
) NO_EXCEPT
{
    wchar_t abs_dst[PATH_MAX_LENGTH];
    relative_to_absolute(dst, abs_dst);

    if (!directory_tree_create(abs_dst)) {
        return false;
    }

    wchar_t abs_src[PATH_MAX_LENGTH];
    relative_to_absolute(src, abs_src);

    wchar_t search_path[MAX_PATH];
    sprintf_fast(search_path, L"%s\\*", abs_src);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search_path, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (wcscmp(ffd.cFileName, L".") == 0
            || wcscmp(ffd.cFileName, L"..") == 0
        ) {
            continue;
        }

        wchar_t src_item[MAX_PATH];
        wchar_t dst_item[MAX_PATH];

        sprintf_fast(src_item, L"%s/%s", abs_src, ffd.cFileName);
        sprintf_fast(dst_item, L"%s/%s", abs_dst, ffd.cFileName);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!directory_copy(src_item, dst_item)) {
                FindClose(hFind);
                return false;
            }
        } else {
            if (!file_copy(src_item, dst_item)) {
                FindClose(hFind);
                return false;
            }
        }

    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);

    return true;
}

inline bool
file_move(const char* __restrict src, const char* __restrict dst) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, src, PROFILE_FLAG_SHOULD_LOG);

    // @performance we are creating an absolute path for dst potentially twice
    directory_tree_create(dst);

    if (*src == '.') {
        char src_full_path[PATH_MAX_LENGTH];
        relative_to_absolute(src, src_full_path);

        if (*dst == '.') {
            char dst_full_path[PATH_MAX_LENGTH];
            relative_to_absolute(dst, dst_full_path);

            return (bool) MoveFileA((LPCSTR) src_full_path, (LPCSTR) dst_full_path);
        } else {
            return (bool) MoveFileA((LPCSTR) src_full_path, (LPCSTR) dst);
        }
    } else if (*dst == '.') {
        char dst_full_path[PATH_MAX_LENGTH];
        relative_to_absolute(dst, dst_full_path);

        return (bool) MoveFileA((LPCSTR) src, (LPCSTR) dst_full_path);
    } else {
        return (bool) MoveFileA((LPCSTR) src, (LPCSTR) dst);
    }
}

inline bool
file_move(const wchar_t* __restrict src, const wchar_t* __restrict dst) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, NULL, PROFILE_FLAG_SHOULD_LOG);

    // @performance we are creating an absolute path for dst potentially twice
    directory_tree_create(dst);

    if (*src == L'.') {
        wchar_t src_full_path[PATH_MAX_LENGTH];
        relative_to_absolute(src, src_full_path);

        if (*dst == L'.') {
            wchar_t dst_full_path[PATH_MAX_LENGTH];
            relative_to_absolute(dst, dst_full_path);

            return (bool) MoveFileW(src_full_path, dst_full_path);
        } else {
            return (bool) MoveFileW(src_full_path, dst);
        }
    } else if (*dst == L'.') {
        wchar_t dst_full_path[PATH_MAX_LENGTH];
        relative_to_absolute(dst, dst_full_path);

        return (bool) MoveFileW(src, dst_full_path);
    } else {
        return (bool) MoveFileW(src, dst);
    }
}

FORCE_INLINE
void file_close_handle(FileHandle fp) NO_EXCEPT
{
    CloseHandle(fp);
    STATS_DECREMENT_PERSISTENT(DEBUG_COUNTER_FILE_HANDLE_COUNT);
}

inline
FileHandle file_append_handle(const char* path) NO_EXCEPT
{
    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
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

    STATS_INCREMENT_PERSISTENT(DEBUG_COUNTER_FILE_HANDLE_COUNT);

    return fp;
}

inline
FileHandle file_append_handle(const wchar_t* path) NO_EXCEPT
{
    FileHandle fp;
    if (*path == L'.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = CreateFileW((LPCWSTR) full_path,
            FILE_APPEND_DATA,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileW((LPCWSTR) path,
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

    STATS_INCREMENT_PERSISTENT(DEBUG_COUNTER_FILE_HANDLE_COUNT);

    return fp;
}

inline
bool file_read_async(
    FileHandle fp,
    FileBodyAsync* __restrict file,
    uint64 offset = 0,
    uint64 length = MAX_UINT64,
    RingMemory* const __restrict ring = NULL
) NO_EXCEPT
{
    LARGE_INTEGER size;
    if (!GetFileSizeEx(fp, &size)) {
        file->content = NULL;
        ASSERT_TRUE(false);

        return false;
    }

    // Ensure the offset and length do not exceed the file size
    uint64 fsize = size.QuadPart;
    if (offset >= fsize) {
        file->size = 0;
        file->content = NULL;
        ASSERT_TRUE(false);

        return false;
    }

    // Adjust the length to read so that it does not exceed the file size
    uint64 read_length = OMS_MIN(length, fsize - offset);

    // Allocate memory for the content
    if (ring != NULL) {
        file->content = ring_get_memory(ring, read_length);
    }

    if (!file->content) {
        ASSERT_TRUE(false);

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
            ASSERT_TRUE(false);

            return false;
        }
    }

    file->size = read_length;

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_READ, read_length);

    return true;
}

inline
void file_async_wait(FileHandle fp, file_overlapped* overlapped, bool wait) NO_EXCEPT
{
    DWORD bytesTransferred;
    GetOverlappedResult(fp, overlapped, &bytesTransferred, wait);
}

inline
FileHandle file_read_handle(const char* path) NO_EXCEPT
{
    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
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

    STATS_INCREMENT_PERSISTENT(DEBUG_COUNTER_FILE_HANDLE_COUNT);

    return fp;
}

inline
FileHandle file_read_handle(const wchar_t* path) NO_EXCEPT
{
    FileHandle fp;
    if (*path == '.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = CreateFileW((LPCWSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileW((LPCWSTR) path,
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

    STATS_INCREMENT_PERSISTENT(DEBUG_COUNTER_FILE_HANDLE_COUNT);

    return fp;
}

inline
FileHandle file_read_async_handle(const char* path) NO_EXCEPT
{
    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
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

    STATS_INCREMENT_PERSISTENT(DEBUG_COUNTER_FILE_HANDLE_COUNT);

    return fp;
}

inline
FileHandle file_read_async_handle(const wchar_t* path) NO_EXCEPT
{
    FileHandle fp;
    if (*path == '.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = CreateFileW((LPCWSTR) full_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
        );
    } else {
        fp = CreateFileW((LPCWSTR) path,
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

    STATS_INCREMENT_PERSISTENT(DEBUG_COUNTER_FILE_HANDLE_COUNT);

    return fp;
}

bool file_append(const char* __restrict path, const char* __restrict file) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
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
    DWORD length = (DWORD) strlen(file);
    if (!WriteFile(fp, file, length, &written, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, written);

    return true;
}

inline bool
file_append(FileHandle fp, const char* file) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, file, PROFILE_FLAG_SHOULD_LOG);

    if (fp == INVALID_HANDLE_VALUE) {
        ASSERT_TRUE(false);
        return false;
    }

    DWORD written;
    const DWORD length = (DWORD) strlen(file);
    if (!WriteFile(fp, file, length, &written, NULL)) {
        ASSERT_TRUE(false);
        return false;
    }

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, written);

    return true;
}

inline bool
file_append(FileHandle fp, const char* file, size_t length) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, file, PROFILE_FLAG_SHOULD_LOG);

    if (fp == INVALID_HANDLE_VALUE) {
        ASSERT_TRUE(false);
        return false;
    }

    DWORD written;
    if (!WriteFile(fp, file, (uint32) length, &written, NULL)) {
        ASSERT_TRUE(false);
        return false;
    }

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, written);

    return true;
}

inline bool
file_append(const char* __restrict path, const FileBody* __restrict file) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, path, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
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
    const DWORD length = (DWORD) file->size;
    if (!WriteFile(fp, file->content, length, &bytes, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, bytes);

    return true;
}

bool file_append(const wchar_t* __restrict path, const wchar_t* __restrict file) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, NULL, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == '.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = CreateFileW((LPCWSTR) full_path,
            FILE_APPEND_DATA,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileW((LPCWSTR) path,
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
    DWORD length = (DWORD) wcslen(file);
    if (!WriteFile(fp, file, length, &written, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, written);

    return true;
}

inline bool
file_append(const wchar_t* __restrict path, const FileBody* __restrict file) NO_EXCEPT
{
    PROFILE(PROFILE_FILE_UTILS, NULL, PROFILE_FLAG_SHOULD_LOG);

    FileHandle fp;
    if (*path == '.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = CreateFileW((LPCWSTR) full_path,
            FILE_APPEND_DATA,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
    } else {
        fp = CreateFileW((LPCWSTR) path,
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
    const DWORD length = (DWORD) file->size;
    if (!WriteFile(fp, file->content, length, &bytes, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);

    STATS_INCREMENT_BY(DEBUG_COUNTER_DRIVE_WRITE, bytes);

    return true;
}

inline
uint64 file_last_modified(const char* path) NO_EXCEPT
{
    WIN32_FIND_DATAA find_data;

    FileHandle fp;
    if (*path == '.') {
        char full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = FindFirstFileA(full_path, (LPWIN32_FIND_DATAA) &find_data);
    } else {
        fp = FindFirstFileA(path, (LPWIN32_FIND_DATAA) &find_data);
    }

    FILETIME modified = {0};
    if(fp != INVALID_HANDLE_VALUE) {
        modified = find_data.ftLastWriteTime;
        FindClose(fp);
    }

    ULARGE_INTEGER ull;
    ull.LowPart = modified.dwLowDateTime;
    ull.HighPart = modified.dwHighDateTime;

    return ull.QuadPart;
}

inline
uint64 file_last_modified(const wchar_t* path) NO_EXCEPT
{
    WIN32_FIND_DATAW find_data;

    FileHandle fp;
    if (*path == L'.') {
        wchar_t full_path[PATH_MAX_LENGTH];
        relative_to_absolute(path, full_path);

        fp = FindFirstFileW(full_path, &find_data);
    } else {
        fp = FindFirstFileW(path, &find_data);
    }

    FILETIME modified = {0};
    if (fp != INVALID_HANDLE_VALUE) {
        modified = find_data.ftLastWriteTime;
        FindClose(fp);
    }

    ULARGE_INTEGER ull;
    ull.LowPart  = modified.dwLowDateTime;
    ull.HighPart = modified.dwHighDateTime;

    return ull.QuadPart;
}

FORCE_INLINE
int32 self_path(char* path) NO_EXCEPT
{
    int32 path_length = GetModuleFileNameA(NULL, path, PATH_MAX_LENGTH);
    if (path_length == 0) {
        return 0;
    }

    char* last = path + path_length;
    while (*last != '\\' && path_length > 0) {
        --last;
        --path_length;
    }

    ++path_length;

    ++last;
    *last = '\0';

    return path_length;
}

FORCE_INLINE
int32 self_path(wchar_t* path) NO_EXCEPT
{
    int32 path_length = GetModuleFileNameW(NULL, path, PATH_MAX_LENGTH);
    if (path_length == 0) {
        return 0;
    }

    wchar_t* last = path + path_length;
    while (*last != L'\\' && path_length > 0) {
        --last;
        --path_length;
    }

    ++path_length;

    ++last;
    *last = L'\0';

    return path_length;
}

void iterate_directory(
    const char* base_path,
    const char* file_ending,
    void (*handler)(const char *, void *),
    ...
) NO_EXCEPT
{
    va_list args;
    va_start(args, handler);

    char full_base_path[PATH_MAX_LENGTH];
    relative_to_absolute(base_path, full_base_path);

    WIN32_FIND_DATAA find_file_data;
    char search_path[PATH_MAX_LENGTH];
    snprintf(search_path, PATH_MAX_LENGTH, "%s/*", full_base_path);

    HANDLE hFind = FindFirstFileA((LPCSTR) search_path, &find_file_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        va_end(args);

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

        char full_path[PATH_MAX_LENGTH];
        // @performance This is bad, we are internally moving two times too often to the end of full_path
        //      Maybe make str_copy return the length, same as append?
        strcpy(full_path, base_path);
        if (!str_ends_with(base_path, "/")) {
            strcat(full_path, "/");
        }
        strcat(full_path, (const char *) find_file_data.cFileName);

        if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            iterate_directory(full_path, file_ending, handler, args);
        } else if (str_ends_with(full_path, file_ending)) {
            handler(full_path, args);
        }
    } while (FindNextFileA(hFind, &find_file_data) != 0);

    FindClose(hFind);

    va_end(args);
}

FORCE_INLINE
void file_delete(const char* path) {
    DeleteFileA(path);
}

FORCE_INLINE
void file_delete(const wchar_t* path) {
    DeleteFileW(path);
}

#endif