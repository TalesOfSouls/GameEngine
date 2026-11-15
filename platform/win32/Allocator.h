/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_ALLOCATOR_H
#define COMS_PLATFORM_WIN32_ALLOCATOR_H

#include <malloc.h>
#include <windows.h>
#include "../../stdlib/Stdlib.h"
#include "../../log/DebugMemory.h"
#include "../../log/Stats.h"
#include "../../log/Log.h"

static int32 _page_size = 0;

/**
 * Large pages (usually 2 MB instead of 4 KB pages -> far fewer TLB misses for the big memory pools)
 *
 * Used by platform_alloc_aligned() for allocations that are
 *      - fixed in size (initial_size == reserve_size): large pages can only be reserved + committed at once,
 *        they can't be committed/decommitted later (no grow/shrink)
 *      - at least one large page in size (smaller allocations would waste most of the page)
 *
 * Requirements (otherwise the normal pages are used, no error):
 *      - the user account has the right "Lock pages in memory" (SeLockMemoryPrivilege, secpol.msc ->
 *        Local Policies -> User Rights Assignment, takes effect after a new log on)
 *      - the privilege is enabled in the process token
 *      - the OS finds enough contiguous physical memory (can fail on a fragmented system, also after a while)
 *
 * WARNING: Large pages are never paged out (locked in physical memory)
 *
 * Set COMS_ALLOC_LARGE_PAGES to 0 to disable them
 */
#ifndef COMS_ALLOC_LARGE_PAGES
    #define COMS_ALLOC_LARGE_PAGES 1
#endif

// Large page size of the system, 0 = not checked yet, -1 = the system doesn't support large pages (per module)
static int64 _large_page_size = 0;

enum PlatformAllocFlag : uint32 {
    PLATFORM_ALLOC_FLAG_NONE = 0,

    // The allocation uses large pages (can't grow/shrink)
    PLATFORM_ALLOC_FLAG_LARGE_PAGES = 1 << 0,
};

struct platform_alloc_header {
    size_t reserved_size;
    size_t committed_size;

    // @type PlatformAllocFlag
    uint32 flags;
};

/**
 * Tries to reserve + commit memory with large pages
 *
 * @param size_t size Size in bytes (incl. header + alignment)
 * @param size_t* allocated_size Size of the allocation, a multiple of the large page size (output)
 *
 * @return void* NULL if large pages are not available (the caller falls back to normal pages)
 */
static inline
void* platform_alloc_large_pages(size_t size, size_t* const allocated_size) NO_EXCEPT
{
    #if COMS_ALLOC_LARGE_PAGES
        if (!_large_page_size) {
            const size_t large_page_size = GetLargePageMinimum();
            _large_page_size = large_page_size ? (int64) large_page_size : -1;
        }

        if (_large_page_size <= 0 || size < (size_t) _large_page_size) {
            return NULL;
        }

        const size_t large_size = ALIGN_UP(size, (size_t) _large_page_size);
        void* const base = VirtualAlloc(NULL, large_size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
        if (!base) {
            const DWORD error = GetLastError();
            if (error == ERROR_PRIVILEGE_NOT_HELD) {
                // The privilege is missing or not enabled yet (allocations before setup_large_pages(), e.g. debug memory)
                // Not cached: the privilege is enabled later during the setup (setup_large_pages() already logs a warning)
                LOG_3("[INFO] Large pages not used: privilege SeLockMemoryPrivilege not held");
            } else {
                // E.g. not enough contiguous physical memory, a smaller allocation may still work
                LOG_1(
                    "[WARNING] Large page allocation of %n B failed (error %d), using normal pages",
                    {DATA_TYPE_UINT64, (void *) &large_size}, {DATA_TYPE_UINT32, (void *) &error}
                );
            }

            return NULL;
        }

        *allocated_size = large_size;
        LOG_1("[INFO] Allocated %n B with large pages", {DATA_TYPE_UINT64, (void *) &large_size});

        return base;
    #else
        (void) size;
        (void) allocated_size;

        return NULL;
    #endif
}

// Returns zeroed memory
inline
void* platform_alloc_aligned(
    size_t initial_size,
    size_t reserve_size = 0,
    int32 alignment = sizeof(void*)
) NO_EXCEPT
{
    ASSERT_TRUE(alignment >= sizeof(void*));

    if (!reserve_size) {
        reserve_size = initial_size;
    }

    if (!_page_size) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        _page_size = si.dwPageSize;
    }

    initial_size = ALIGN_UP(
        initial_size + sizeof(platform_alloc_header) + sizeof(void*) + alignment,
        _page_size
    );
    reserve_size = ALIGN_UP(
        reserve_size + sizeof(platform_alloc_header) + sizeof(void*) + alignment,
        _page_size
    );

    ASSERT_TRUE(initial_size <= reserve_size);

    // Fixed size allocations: large pages if possible
    uint32 flags = PLATFORM_ALLOC_FLAG_NONE;
    void* base = NULL;
    if (initial_size == reserve_size) {
        size_t large_size = 0;
        base = platform_alloc_large_pages(reserve_size, &large_size);
        if (base) {
            initial_size = large_size;
            reserve_size = large_size;
            flags |= PLATFORM_ALLOC_FLAG_LARGE_PAGES;
        }
    }

    if (!base) {
        base = VirtualAlloc(NULL, reserve_size, MEM_RESERVE, PAGE_READWRITE);
        ASSERT_TRUE(base);

        const void* const committed = VirtualAlloc(base, initial_size, MEM_COMMIT, PAGE_READWRITE);
        ASSERT_TRUE(committed == base);
        PSEUDO_USE(committed);
    }

    platform_alloc_header* hdr = (platform_alloc_header *) base;
    hdr->reserved_size  = reserve_size;
    hdr->committed_size = initial_size;
    hdr->flags = flags;

    uintptr_t raw = (uintptr_t)base + sizeof(platform_alloc_header) + sizeof(void*);
    void* aligned = (void*) ALIGN_UP(raw, alignment);

    // store base for freeing
    ((void**)aligned)[-1] = base;

    STATS_INCREMENT_BY_DEBUG(DEBUG_COUNTER_MEM_ALLOC, initial_size);

    return aligned;
}

inline
bool platform_alloc_aligned_grow(void* aligned_ptr, size_t new_user_size) NO_EXCEPT
{
    ASSERT_TRUE(aligned_ptr);

    // Get the base header
    void* base = ((void**)aligned_ptr)[-1];
    platform_alloc_header* hdr = (platform_alloc_header *) base;

    // Large pages are committed at once and can't grow
    if (hdr->flags & PLATFORM_ALLOC_FLAG_LARGE_PAGES) {
        LOG_1("[ERROR] platform_alloc_aligned_grow: large page allocations can't grow");
        return false;
    }

    // Calculate new committed size including header and alignment
    const size_t new_committed = ALIGN_UP(new_user_size + sizeof(platform_alloc_header) + sizeof(void*), _page_size);

    ASSERT_TRUE(new_committed > hdr->committed_size);
    ASSERT_TRUE(new_committed <= hdr->reserved_size);

    // Commit the new pages
    const void* const result = VirtualAlloc(
        (uint8_t*)base + hdr->committed_size,
        new_committed - hdr->committed_size,
        MEM_COMMIT,
        PAGE_READWRITE
    );

    if (!result) {
        LOG_3("[ERROR] platform_alloc_aligned_grow failed");
        return false;
    }

    STATS_INCREMENT_BY_DEBUG(DEBUG_COUNTER_MEM_ALLOC, new_committed - hdr->committed_size);

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
    platform_alloc_header* hdr = (platform_alloc_header *) base;

    // Large pages can't be decommitted (the memory stays committed, not an error for the caller)
    if (hdr->flags & PLATFORM_ALLOC_FLAG_LARGE_PAGES) {
        return true;
    }

    const size_t new_committed = ALIGN_UP(new_user_size + sizeof(platform_alloc_header) + sizeof(void*), _page_size);

    ASSERT_TRUE(new_committed < hdr->committed_size);

    const size_t delta = hdr->committed_size - new_committed;

    BOOL ok = VirtualFree((uint8_t*)base + new_committed, delta, MEM_DECOMMIT);
    if (!ok) {
        LOG_3("[ERROR] platform_alloc_aligned_shrink failed");
        return false;
    }

    STATS_DECREMENT_BY_DEBUG(DEBUG_COUNTER_MEM_ALLOC, delta);
    LOG_3("[INFO] Shrunk aligned allocation by %n B", {DATA_TYPE_UINT64, &delta});

    hdr->committed_size = new_committed;

    return true;
}

inline
bool platform_alloc_aligned_resize(void* aligned_ptr, size_t new_user_size) NO_EXCEPT
{
    ASSERT_TRUE(aligned_ptr);

    // Get the base header
    void* base = ((void**)aligned_ptr)[-1];
    platform_alloc_header* hdr = (platform_alloc_header *) base;

    // Calculate new committed size including header and alignment
    const size_t new_committed = ALIGN_UP(new_user_size + sizeof(platform_alloc_header) + sizeof(void*), _page_size);

    if (new_committed == hdr->committed_size) {
        return true;
    } else if (new_committed > hdr->committed_size) {
        return platform_alloc_aligned_grow(aligned_ptr, new_user_size);
    } else {
        return platform_alloc_aligned_shrink(aligned_ptr, new_user_size);
    }
}

inline
void platform_aligned_free(void** aligned_ptr) NO_EXCEPT
{
    if (!aligned_ptr || !*aligned_ptr) {
        return;
    }

    // Base address stored just before aligned pointer
    void* base = ((void**)(*aligned_ptr))[-1];

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

    initial_size = ALIGN_UP(initial_size + sizeof(platform_alloc_header), _page_size);
    reserve_size = ALIGN_UP(reserve_size + sizeof(platform_alloc_header), _page_size);

    ASSERT_TRUE(initial_size <= reserve_size);

    // Shared memory doesn't use large pages (would need SEC_LARGE_PAGES + a size that is a multiple of the large page size)
    *fd = CreateFileMappingA(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
        (DWORD)(reserve_size >> 32), (DWORD)(reserve_size & 0xFFFFFFFF), name
    );
    ASSERT_TRUE(*fd);

    void* base = MapViewOfFile(*fd, FILE_MAP_ALL_ACCESS, 0, 0, initial_size);
    ASSERT_TRUE(base);

    platform_alloc_header* hdr = (platform_alloc_header *) base;
    hdr->reserved_size  = reserve_size;
    hdr->committed_size = initial_size;
    hdr->flags = PLATFORM_ALLOC_FLAG_NONE;

    void* user_ptr = (void*)((uintptr_t)base + sizeof(platform_alloc_header));

    STATS_INCREMENT_BY_DEBUG(DEBUG_COUNTER_MEM_ALLOC, initial_size);

    return user_ptr;
}

inline
bool platform_shared_alloc_grow(void* shm_ptr, size_t new_user_size) NO_EXCEPT
{
    ASSERT_TRUE(shm_ptr);

    platform_alloc_header* base_hdr = (platform_alloc_header*)((uintptr_t)shm_ptr - sizeof(platform_alloc_header));
    platform_alloc_header* hdr = base_hdr;

    const size_t new_committed = ALIGN_UP(new_user_size + sizeof(platform_alloc_header), _page_size);

    ASSERT_TRUE(new_committed > hdr->committed_size);
    ASSERT_TRUE(new_committed <= hdr->reserved_size);

    const void* const result = VirtualAlloc(
        (uint8_t*)base_hdr + hdr->committed_size,
        new_committed - hdr->committed_size,
        MEM_COMMIT,
        PAGE_READWRITE
    );

    if (!result) {
        LOG_3("[ERROR] platform_shared_alloc_grow failed");
        return false;
    }

    STATS_INCREMENT_BY_DEBUG(DEBUG_COUNTER_MEM_ALLOC, new_committed - hdr->committed_size);

    hdr->committed_size = new_committed;
    LOG_3("[INFO] Grown shared allocation to %n B", {DATA_TYPE_UINT64, &hdr->committed_size});

    return true;
}

inline
bool platform_shared_alloc_shrink(void* shm_ptr, size_t new_user_size) NO_EXCEPT
{
    ASSERT_TRUE(shm_ptr);

    platform_alloc_header* base_hdr = (platform_alloc_header*)((uintptr_t)shm_ptr - sizeof(platform_alloc_header));
    platform_alloc_header* hdr = base_hdr;

    const size_t new_committed = ALIGN_UP(new_user_size + sizeof(platform_alloc_header), _page_size);

    ASSERT_TRUE(new_committed < hdr->committed_size);

    const size_t delta = hdr->committed_size - new_committed;

    BOOL ok = VirtualFree((uint8_t*)base_hdr + new_committed, delta, MEM_DECOMMIT);
    if (!ok) {
        LOG_3("[ERROR] platform_shared_alloc_shrink failed");
        return false;
    }

    STATS_DECREMENT_BY_DEBUG(DEBUG_COUNTER_MEM_ALLOC, delta);
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
    void* user_ptr = (void*)((uintptr_t)base + sizeof(platform_alloc_header));

    LOG_3("[INFO] Shared opened %n B", {DATA_TYPE_UINT64, &reserve_size});
    return user_ptr;
}

inline
void platform_shared_free(HANDLE fd, const char*, void** ptr) NO_EXCEPT
{
    if (!ptr || !*ptr) {
        return;
    }

    platform_alloc_header* base_hdr = (platform_alloc_header*)((uintptr_t)(*ptr) - sizeof(platform_alloc_header));

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