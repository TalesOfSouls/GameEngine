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
#include "../../stdlib/Stdlib.h"
#include "../../utils/Assert.h"
#include "../../log/DebugMemory.h"
#include "../../log/Stats.h"
#include "../../log/Log.h"

static int32 _page_size = 0;

struct win_alloc_header {
    size_t reserved_size;
    size_t committed_size;
};

inline
void* platform_alloc(size_t initial_size, size_t reserve_size = 0) NO_EXCEPT
{
    if (!reserve_size) {
        reserve_size = initial_size;
    }

    if (!_page_size) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        _page_size = si.dwPageSize;
    }

    initial_size = align_up(initial_size, _page_size);
    reserve_size = align_up(reserve_size, _page_size);

    ASSERT_TRUE(initial_size <= reserve_size);

    // Reserve the entire range first (PROT_NONE equivalent)
    void* base = VirtualAlloc(
        NULL,
        reserve_size,
        MEM_RESERVE,
        PAGE_READWRITE
    );
    ASSERT_TRUE(base);

    // Commit only the initial portion
    void* committed = VirtualAlloc(
        base,
        initial_size,
        MEM_COMMIT,
        PAGE_READWRITE
    );
    ASSERT_TRUE(committed == base);

    win_alloc_header* hdr = (win_alloc_header *) base;
    hdr->reserved_size  = reserve_size;
    hdr->committed_size = initial_size;

    void* user_ptr = (void*)((uintptr_t)base + sizeof(win_alloc_header));

    DEBUG_MEMORY_INIT((uintptr_t)user_ptr, initial_size);
    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, initial_size);
    LOG_3("[INFO] Allocated %n B", {DATA_TYPE_UINT64, &initial_size});

    return user_ptr;
}

inline
bool platform_alloc_grow(void* ptr, size_t new_user_size) NO_EXCEPT
{
    win_alloc_header* hdr = (win_alloc_header*)((uintptr_t)ptr - sizeof(win_alloc_header));
    const size_t new_committed = align_up(new_user_size + sizeof(win_alloc_header), _page_size);

    ASSERT_TRUE(new_committed > hdr->committed_size);
    ASSERT_TRUE(new_committed <= hdr->reserved_size);

    void* result = VirtualAlloc(
        (uint8_t*)hdr + hdr->committed_size,
        new_committed - hdr->committed_size,
        MEM_COMMIT, PAGE_READWRITE
    );
    ASSERT_TRUE(result);

    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, new_committed - hdr->committed_size);
    hdr->committed_size = new_committed;

    return true;
}

inline
bool platform_alloc_shrink(void* ptr, size_t new_user_size) NO_EXCEPT
{
    win_alloc_header* hdr = (win_alloc_header*)((uintptr_t)ptr - sizeof(win_alloc_header));
    const size_t new_committed = align_up(new_user_size + sizeof(win_alloc_header), _page_size);

    ASSERT_TRUE(new_committed < hdr->committed_size);

    const size_t delta = hdr->committed_size - new_committed;
    VirtualFree((uint8_t*)hdr + new_committed, delta, MEM_DECOMMIT);

    STATS_DECREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, delta);
    hdr->committed_size = new_committed;

    return true;
}

inline
void* platform_alloc_aligned(
    size_t initial_size,
    size_t reserve_size = 0,
    int32 alignment = sizeof(void*)
) NO_EXCEPT
{
    if (!reserve_size) {
        reserve_size = initial_size;
    }

    if (!_page_size) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        _page_size = si.dwPageSize;
    }

    initial_size = align_up(initial_size + sizeof(win_alloc_header) + alignment, _page_size);
    reserve_size = align_up(reserve_size + sizeof(win_alloc_header) + alignment, _page_size);

    ASSERT_TRUE(initial_size <= reserve_size);

    void* base = VirtualAlloc(NULL, reserve_size, MEM_RESERVE, PAGE_READWRITE);
    ASSERT_TRUE(base);

    void* committed = VirtualAlloc(base, initial_size, MEM_COMMIT, PAGE_READWRITE);
    ASSERT_TRUE(committed == base);

    win_alloc_header* hdr = (win_alloc_header *) base;
    hdr->reserved_size  = reserve_size;
    hdr->committed_size = initial_size;

    uintptr_t raw = (uintptr_t)base + sizeof(win_alloc_header);
    void* aligned = (void*)align_up(raw, alignment);

    // store base for freeing
    ((void**)aligned)[-1] = base;

    DEBUG_MEMORY_INIT((uintptr_t)aligned, initial_size);
    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, initial_size);

    return aligned;
}

inline
bool platform_alloc_aligned_grow(void* aligned_ptr, size_t new_user_size) NO_EXCEPT
{
    ASSERT_TRUE(aligned_ptr);

    // Get the base header
    void* base = ((void**)aligned_ptr)[-1];
    win_alloc_header* hdr = (win_alloc_header *) base;

    // Calculate new committed size including header and alignment
    const size_t new_committed = align_up(new_user_size + sizeof(win_alloc_header), _page_size);

    ASSERT_TRUE(new_committed > hdr->committed_size);
    ASSERT_TRUE(new_committed <= hdr->reserved_size);

    // Commit the new pages
    void* result = VirtualAlloc(
        (uint8_t*)base + hdr->committed_size,
        new_committed - hdr->committed_size,
        MEM_COMMIT,
        PAGE_READWRITE
    );

    if (!result) {
        LOG_3("[ERROR] platform_alloc_aligned_grow failed");
        return false;
    }

    DEBUG_MEMORY_INIT((uintptr_t)aligned_ptr + hdr->committed_size, new_committed - hdr->committed_size);
    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, new_committed - hdr->committed_size);

    hdr->committed_size = new_committed;
    LOG_3("[INFO] Grown aligned allocation to %n B", {DATA_TYPE_UINT64, &hdr->committed_size});

    return true;
}

inline
bool platform_alloc_aligned_shrink(void* aligned_ptr, size_t new_user_size) NO_EXCEPT
{
    ASSERT_TRUE(aligned_ptr);

    // Get the base header
    void* base = ((void**)aligned_ptr)[-1];
    win_alloc_header* hdr = (win_alloc_header *) base;

    const size_t new_committed = align_up(new_user_size + sizeof(win_alloc_header), _page_size);

    ASSERT_TRUE(new_committed < hdr->committed_size);

    const size_t delta = hdr->committed_size - new_committed;

    BOOL ok = VirtualFree((uint8_t*)base + new_committed, delta, MEM_DECOMMIT);
    if (!ok) {
        LOG_3("[ERROR] platform_alloc_aligned_shrink failed");
        return false;
    }

    STATS_DECREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, delta);
    LOG_3("[INFO] Shrunk aligned allocation by %n B", {DATA_TYPE_UINT64, &delta});

    hdr->committed_size = new_committed;

    return true;
}

inline
void platform_free(void** ptr) NO_EXCEPT
{
    if (!ptr || !*ptr) {
        return;
    }

    // The base address is right before the header
    void* base = (uint8_t*)(*ptr) - sizeof(win_alloc_header);

    DEBUG_MEMORY_FREE((uintptr_t)(*ptr));

    // Free the entire reserved region
    VirtualFree(base, 0, MEM_RELEASE);

    *ptr = NULL;
}

inline
void platform_aligned_free(void** aligned_ptr) NO_EXCEPT
{
    if (!aligned_ptr || !*aligned_ptr) {
        return;
    }

    // Base address stored just before aligned pointer
    void* base = ((void**)(*aligned_ptr))[-1];

    DEBUG_MEMORY_FREE((uintptr_t)(*aligned_ptr));

    // Free the entire reserved region
    VirtualFree(base, 0, MEM_RELEASE);

    *aligned_ptr = NULL;
}

inline
void* platform_shared_alloc(
    HANDLE* fd,
    const char* name,
    size_t initial_size,
    size_t reserve_size = 0
) NO_EXCEPT
{
    if (!reserve_size) {
        reserve_size = initial_size;
    }

    if (!_page_size) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        _page_size = si.dwPageSize;
    }

    initial_size = align_up(initial_size + sizeof(win_alloc_header), _page_size);
    reserve_size = align_up(reserve_size + sizeof(win_alloc_header), _page_size);

    ASSERT_TRUE(initial_size <= reserve_size);

    *fd = CreateFileMappingA(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
        (DWORD)(reserve_size >> 32), (DWORD)(reserve_size & 0xFFFFFFFF), name
    );
    ASSERT_TRUE(*fd);

    void* base = MapViewOfFile(*fd, FILE_MAP_ALL_ACCESS, 0, 0, initial_size);
    ASSERT_TRUE(base);

    win_alloc_header* hdr = (win_alloc_header *) base;
    hdr->reserved_size  = reserve_size;
    hdr->committed_size = initial_size;

    void* user_ptr = (void*)((uintptr_t)base + sizeof(win_alloc_header));

    DEBUG_MEMORY_INIT((uintptr_t)user_ptr, initial_size);
    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, initial_size);

    return user_ptr;
}

inline
bool platform_shared_alloc_grow(void* shm_ptr, size_t new_user_size) NO_EXCEPT
{
    ASSERT_TRUE(shm_ptr);

    win_alloc_header* base_hdr = (win_alloc_header*)((uintptr_t)shm_ptr - sizeof(win_alloc_header));
    win_alloc_header* hdr = base_hdr;

    const size_t new_committed = align_up(new_user_size + sizeof(win_alloc_header), _page_size);

    ASSERT_TRUE(new_committed > hdr->committed_size);
    ASSERT_TRUE(new_committed <= hdr->reserved_size);

    void* result = VirtualAlloc(
        (uint8_t*)base_hdr + hdr->committed_size,
        new_committed - hdr->committed_size,
        MEM_COMMIT,
        PAGE_READWRITE
    );

    if (!result) {
        LOG_3("[ERROR] platform_shared_alloc_grow failed");
        return false;
    }

    DEBUG_MEMORY_INIT((uintptr_t)shm_ptr + hdr->committed_size, new_committed - hdr->committed_size);
    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, new_committed - hdr->committed_size);

    hdr->committed_size = new_committed;
    LOG_3("[INFO] Grown shared allocation to %n B", {DATA_TYPE_UINT64, &hdr->committed_size});

    return true;
}

inline
bool platform_shared_alloc_shrink(void* shm_ptr, size_t new_user_size) NO_EXCEPT
{
    ASSERT_TRUE(shm_ptr);

    win_alloc_header* base_hdr = (win_alloc_header*)((uintptr_t)shm_ptr - sizeof(win_alloc_header));
    win_alloc_header* hdr = base_hdr;

    const size_t new_committed = align_up(new_user_size + sizeof(win_alloc_header), _page_size);

    ASSERT_TRUE(new_committed < hdr->committed_size);

    const size_t delta = hdr->committed_size - new_committed;

    BOOL ok = VirtualFree((uint8_t*)base_hdr + new_committed, delta, MEM_DECOMMIT);
    if (!ok) {
        LOG_3("[ERROR] platform_shared_alloc_shrink failed");
        return false;
    }

    STATS_DECREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, delta);
    LOG_3("[INFO] Shrunk shared allocation by %n B", {DATA_TYPE_UINT64, &delta});

    hdr->committed_size = new_committed;

    return true;
}

inline
void* platform_shared_open(HANDLE* fd, const char* name, size_t reserve_size) NO_EXCEPT
{
    ASSERT_TRUE(fd);

    *fd = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name);
    ASSERT_TRUE(*fd);

    void* base = MapViewOfFile(*fd, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, reserve_size);
    ASSERT_TRUE(base);

    // Skip header to return user pointer
    void* user_ptr = (void*)((uintptr_t)base + sizeof(win_alloc_header));

    LOG_3("[INFO] Shared opened %n B", {DATA_TYPE_UINT64, &reserve_size});
    return user_ptr;
}

inline
void platform_shared_free(HANDLE fd, const char*, void** ptr) NO_EXCEPT
{
    if (!ptr || !*ptr) {
        return;
    }

    win_alloc_header* base_hdr = (win_alloc_header*)((uintptr_t)(*ptr) - sizeof(win_alloc_header));

    DEBUG_MEMORY_FREE((uintptr_t)*ptr);

    UnmapViewOfFile(base_hdr);
    CloseHandle(fd);

    *ptr = NULL;
}

FORCE_INLINE
void platform_shared_close(HANDLE fd) NO_EXCEPT
{
    CloseHandle(fd);
}

#endif