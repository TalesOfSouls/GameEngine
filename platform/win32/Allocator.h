/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_ALLOCATOR_H
#define COMS_PLATFORM_WIN32_ALLOCATOR_H

#include <malloc.h>
#include <windows.h>
#include "../../stdlib/Types.h"
#include "../../utils/Assert.h"
#include "../../log/DebugMemory.h"
#include "../../log/Stats.h"
#include "../../log/Log.h"

// @todo Currently alignment only effects the starting position, but it should also effect the ending/size

static int32 _page_size = 0;

inline
void* platform_alloc(size_t size)
{
    if (!_page_size) {
        // @todo Fix and get system page size
        _page_size = 1;
    }

    size = OMS_ALIGN_UP(size, _page_size);

    void* ptr = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    DEBUG_MEMORY_INIT((uintptr_t) ptr, size);
    LOG_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, size);
    LOG_3("[INFO] Allocated %n B", {LOG_DATA_UINT64, &size});

    return ptr;
}

inline
void* platform_alloc_aligned(size_t size, int32 alignment)
{
    if (!_page_size) {
        _page_size = 1;
    }

    size = OMS_ALIGN_UP(size + sizeof(void*) + alignment - 1, alignment);
    size = OMS_ALIGN_UP(size, _page_size);

    void* ptr = VirtualAlloc(NULL, size + alignment + sizeof(void*), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    ASSERT_TRUE(ptr);

    // We want an aligned memory area but mmap doesn't really support that.
    // That's why we have to manually offset our memory area.
    // However, when freeing the pointer later on we need the actual start of the memory area, not the manually offset one.
    void* aligned_ptr = (void *) OMS_ALIGN_UP((uintptr_t) ptr + sizeof(void*), alignment);
    ((void**) aligned_ptr)[-1] = ptr;

    DEBUG_MEMORY_INIT((uintptr_t) aligned_ptr, size);
    LOG_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, size);
    LOG_3("[INFO] Aligned allocated %n B", {LOG_DATA_UINT64, &size});

    return aligned_ptr;
}

inline
void platform_free(void** ptr) {
    DEBUG_MEMORY_FREE((uintptr_t) *ptr);
    VirtualFree(*ptr, 0, MEM_RELEASE);
    *ptr = NULL;
}

inline
void platform_aligned_free(void** aligned_ptr) {
    void* ptr = ((void**) *aligned_ptr)[-1];
    DEBUG_MEMORY_FREE((uintptr_t) ptr);

    VirtualFree(ptr, 0, MEM_RELEASE);
    *aligned_ptr = NULL;
}

inline
void* platform_shared_alloc(HANDLE* __restrict fd, const char* __restrict name, size_t size)
{
    if (!_page_size) {
        _page_size = 1;
    }

    size = OMS_ALIGN_UP(size, _page_size);

    *fd = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD) size, name);
    ASSERT_TRUE(*fd);

    void* shm_ptr = MapViewOfFile(*fd, FILE_MAP_ALL_ACCESS, 0, 0, size);
    ASSERT_TRUE(shm_ptr);

    DEBUG_MEMORY_INIT((uintptr_t) shm_ptr, size);
    LOG_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, size);
    LOG_3("[INFO] Shared allocated %n B", {LOG_DATA_UINT64, &size});

    return shm_ptr;
}

inline
void* platform_shared_open(HANDLE* __restrict fd, const char* __restrict name, size_t size)
{
    *fd = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name);
    ASSERT_TRUE(*fd);

    void* shm_ptr = MapViewOfFile(*fd, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, (DWORD) size);
    ASSERT_TRUE(shm_ptr);
    LOG_3("[INFO] Shared opened %n B", {LOG_DATA_UINT64, &size});

    return shm_ptr;
}

inline
void platform_shared_free(HANDLE __restrict fd, const char*, void** __restrict ptr)
{
    DEBUG_MEMORY_FREE((uintptr_t) *ptr);
    UnmapViewOfFile(*ptr);
    CloseHandle(fd);
    *ptr = NULL;
}

inline
void platform_shared_close(HANDLE fd)
{
    CloseHandle(fd);
}

#endif