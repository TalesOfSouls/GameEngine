/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_PLATFORM_WIN32_FILE_UTILS_C
#define TOS_PLATFORM_WIN32_FILE_UTILS_C

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

typedef HANDLE FileHandler;

struct FileBodyAsync {
    // doesn't include null termination (same as strlen)
    uint64 size;
    byte* content;
    OVERLAPPED ov;
};

// @todo Consider to implement directly mapped files (CreateFileMapping) for certain files (e.g. map data or texture data, ...)

inline
void relative_to_absolute(const char* rel, char* path)
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
    strcpy(path + self_path_length, temp);
}

inline uint64
file_size(const char* path)
{
    // @performance Profile against fseek strategy
    FileHandler fp;
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
file_read(const char* path, FileBody* file, RingMemory* ring = NULL)
{
    FileHandler fp;
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

    if (ring != NULL) {
        file->content = ring_get_memory(ring, size.QuadPart);
    }

    DWORD bytes;
    ASSERT_SIMPLE(size.QuadPart < MAX_UINT32);
    if (!ReadFile(fp, file->content, (uint32) size.QuadPart, &bytes, NULL)) {
        CloseHandle(fp);
        file->content = NULL;

        return;
    }

    CloseHandle(fp);

    file->content[bytes] = '\0';
    file->size = size.QuadPart;
}

inline
void file_read(const char* path, FileBody* file, uint64 offset, uint64 length = MAX_UINT64, RingMemory* ring = NULL)
{
    FileHandler fp;
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
        file->content = ring_get_memory(ring, read_length);
    }

    // Move the file pointer to the offset position
    LARGE_INTEGER li;
    li.QuadPart = offset;
    if (SetFilePointerEx(fp, li, NULL, FILE_BEGIN) == 0) {
        CloseHandle(fp);
        file->content = NULL;

        return;
    }

    DWORD bytes;
    ASSERT_SIMPLE(read_length < MAX_UINT32);
    if (!ReadFile(fp, file->content, (uint32) read_length, &bytes, NULL)) {
        CloseHandle(fp);
        file->content = NULL;

        return;
    }

    CloseHandle(fp);

    file->content[bytes] = '\0';
    file->size = bytes;
}

inline
void file_read(FileHandler fp, FileBody* file, uint64 offset = 0, uint64 length = MAX_UINT64, RingMemory* ring = NULL)
{
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
        file->content = ring_get_memory(ring, read_length);
    }

    // Move the file pointer to the offset position
    LARGE_INTEGER li;
    li.QuadPart = offset;
    if (SetFilePointerEx(fp, li, NULL, FILE_BEGIN) == 0) {
        CloseHandle(fp);
        file->content = NULL;

        return;
    }

    DWORD bytes;
    ASSERT_SIMPLE(read_length < MAX_UINT32);
    if (!ReadFile(fp, file->content, (uint32) read_length, &bytes, NULL)) {
        CloseHandle(fp);
        file->content = NULL;

        return;
    }

    CloseHandle(fp);

    file->content[bytes] = '\0';
    file->size = bytes;
}

inline uint64
file_read_struct(const char* path, void* file, uint32 size)
{
    FileHandler fp;
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

    LARGE_INTEGER fsize;
    if (!GetFileSizeEx(fp, &fsize)) {
        CloseHandle(fp);

        return 0;
    }

    DWORD read;
    ASSERT_SIMPLE(fsize.QuadPart > size);
    if (!ReadFile(fp, file, (uint32) size, &read, NULL)) {
        CloseHandle(fp);

        return 0;
    }

    CloseHandle(fp);

    return read;
}

inline bool
file_write(const char* path, const FileBody* file)
{
    FileHandler fp;
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
    ASSERT_SIMPLE(file->size < MAX_UINT32);
    if (!WriteFile(fp, file->content, length, &written, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);

    return true;
}

inline bool
file_write_struct(const char* path, const void* file, uint32 size)
{
    FileHandler fp;
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

    DWORD written;
    ASSERT_SIMPLE(size < MAX_UINT32);
    if (!WriteFile(fp, file, size, &written, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);

    return true;
}

inline void
file_copy(const char* src, const char* dst)
{
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
void close_handle(FileHandler fp)
{
    CloseHandle(fp);
}

inline
HANDLE file_append_handle(const char* path)
{
    FileHandler fp;
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
    FileHandler fp,
    FileBodyAsync* file,
    uint64_t offset = 0,
    uint64_t length = MAXUINT64,
    RingMemory* ring = NULL
) {
    LARGE_INTEGER size;
    if (!GetFileSizeEx(fp, &size)) {
        CloseHandle(fp);
        file->content = NULL;

        return false;
    }

    // Ensure the offset and length do not exceed the file size
    uint64_t file_size = size.QuadPart;
    if (offset >= file_size) {
        file->size = 0;
        file->content = NULL;
        CloseHandle(fp);

        return false;
    }

    // Adjust the length to read so that it does not exceed the file size
    uint64 read_length = OMS_MIN(length, file_size - offset);

    // Allocate memory for the content
    if (ring != NULL) {
        file->content = ring_get_memory(ring, read_length);
    }

    if (!file->content) {
        CloseHandle(fp);

        return false;
    }

    file->ov.Offset = (DWORD)(offset & 0xFFFFFFFF);
    file->ov.OffsetHigh = (DWORD)(offset >> 32);

    // Auto-reset event
    file->ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    DWORD bytes_read = 0;
    ASSERT_SIMPLE(read_length < MAXDWORD);
    if (!ReadFile(fp, file->content, (DWORD) read_length, &bytes_read, &file->ov)) {
        DWORD error = GetLastError();
        if (error != ERROR_IO_PENDING) {
            CloseHandle(fp);
            free(file->content);
            file->content = NULL;
            CloseHandle(&file->ov.hEvent);

            return false;
        }
    }

    file->size = read_length;
    return true;
}

inline
FileHandler file_read_handle(const char* path)
{
    FileHandler fp;
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
FileHandler file_read_async_handle(const char* path)
{
    FileHandler fp;
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

bool file_append(const char* path, const char* file)
{
    FileHandler fp;
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
    DWORD length = (DWORD) strlen(file); // @question WHY is WriteFile not supporting larger data?
    ASSERT_SIMPLE(length < MAX_UINT32);
    if (!WriteFile(fp, file, length, &written, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);
    return true;
}

inline bool
file_append(FileHandler fp, const char* file)
{
    if (fp == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written;
    DWORD length = (DWORD) strlen(file); // @question WHY is WriteFile not supporting larger data?
    ASSERT_SIMPLE(length < MAX_UINT32);

    if (!WriteFile(fp, file, length, &written, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);
    return true;
}

inline bool
file_append(FileHandler fp, const char* file, size_t length)
{
    if (fp == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written;
    if (!WriteFile(fp, file, (uint32) length, &written, NULL)) {
        CloseHandle(fp);
        return false;
    }

    return true;
}

inline bool
file_append(const char* path, const FileBody* file)
{
    FileHandler fp;
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
    ASSERT_SIMPLE(file->size < MAX_UINT32);
    if (!WriteFile(fp, file->content, length, &bytes, NULL)) {
        CloseHandle(fp);
        return false;
    }

    CloseHandle(fp);
    return true;
}

inline
uint64 file_last_modified(const char* path)
{
    WIN32_FIND_DATA find_data;

    FileHandler fp;
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

#endif