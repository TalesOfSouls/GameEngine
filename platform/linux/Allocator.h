/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_ALLOCATOR_H
#define COMS_PLATFORM_LINUX_ALLOCATOR_H

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "../../stdlib/Stdlib.h"
#include "../../utils/Assert.h"
#include "../../log/DebugMemory.h"
#include "../../log/Stats.h"
#include "../../log/Log.h"

static int32 _page_size = 0;

struct platform_alloc_header {
    size_t reserved_size;
    size_t committed_size;
};

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
        _page_size = (int32)sysconf(_SC_PAGESIZE);
    }

    reserve_size = align_up(
        reserve_size + sizeof(platform_alloc_header) + sizeof(void*) + alignment,
        _page_size
    );

    initial_size = align_up(
        initial_size + sizeof(platform_alloc_header) + sizeof(void*) + alignment,
        _page_size
    );

    ASSERT_TRUE(initial_size <= reserve_size);

    void* base = mmap(
        NULL,
        reserve_size,
        PROT_NONE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    ASSERT_TRUE(base != MAP_FAILED);

    ASSERT_TRUE(
        mprotect(base, initial_size, PROT_READ | PROT_WRITE) == 0
    );

    platform_alloc_header* hdr = (platform_alloc_header *) base;
    hdr->reserved_size  = reserve_size;
    hdr->committed_size = initial_size;

    uintptr_t raw = (uintptr_t)base + sizeof(platform_alloc_header) + sizeof(void*);
    void* aligned = (void*)align_up(raw, alignment);

    ((void**)aligned)[-1] = base;

    DEBUG_MEMORY_INIT((uintptr_t)aligned, initial_size);
    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, initial_size);

    return aligned;
}

inline
bool platform_alloc_aligned_grow(void* aligned_ptr, size_t new_user_size) NO_EXCEPT
{
    if (!_page_size) {
        _page_size = (int32)sysconf(_SC_PAGESIZE);
    }

    void* base = ((void**)aligned_ptr)[-1];
    platform_alloc_header* hdr = (platform_alloc_header *) base;

    // Calculate new committed size including header and alignment
    const size_t new_committed = align_up(new_user_size + sizeof(platform_alloc_header) + sizeof(void*), _page_size);

    ASSERT_TRUE(new_committed > hdr->committed_size);
    ASSERT_TRUE(new_committed <= hdr->reserved_size);

    ASSERT_TRUE(
        mprotect(hdr, new_committed, PROT_READ | PROT_WRITE) == 0
    );

    DEBUG_MEMORY_INIT((uintptr_t)aligned_ptr + hdr->committed_size, new_committed - hdr->committed_size);
    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, new_committed - hdr->committed_size);

    hdr->committed_size = new_committed;
    LOG_3("[INFO] Grown aligned allocation to %n B", {DATA_TYPE_UINT64, &hdr->committed_size});

    return true;
}

inline
bool platform_alloc_aligned_shrink(void* aligned_ptr, size_t new_user_size) NO_EXCEPT
{
    if (!_page_size) {
        _page_size = (int32)sysconf(_SC_PAGESIZE);
    }

    void* base = ((void**)aligned_ptr)[-1];
    platform_alloc_header* hdr = (platform_alloc_header *) base;

    const size_t new_committed = align_up(new_user_size + sizeof(platform_alloc_header), _page_size);

    ASSERT_TRUE(new_committed < hdr->committed_size);

    const size_t delta = hdr->committed_size - new_committed;

    ASSERT_TRUE(
        mprotect(
            (uint8_t*)hdr + new_committed,
            delta,
            PROT_NONE
        ) == 0
    );

    madvise(
        (uint8_t*)hdr + new_committed,
        delta,
        MADV_DONTNEED
    );

    hdr->committed_size = new_committed;
    STATS_DECREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, delta);

    return true;
}

inline
void platform_aligned_free(void** aligned_ptr) NO_EXCEPT
{
    if (!aligned_ptr || !*aligned_ptr) {
        return;
    }

    // Base address stored just before aligned pointer
    void* base = ((void**)(*aligned_ptr))[-1];
    platform_alloc_header* hdr = (platform_alloc_header*)base;

    DEBUG_MEMORY_FREE((uintptr_t)*aligned_ptr);
    munmap(base, hdr->reserved_size);

    *aligned_ptr = NULL;
}

inline
void* platform_shared_alloc(
    int32* fd,
    const char* name,
    size_t initial_size,
    size_t reserve_size = 0
) NO_EXCEPT
{
    if (!reserve_size) {
        reserve_size = initial_size;
    }

    if (!_page_size) {
        _page_size = (int32)sysconf(_SC_PAGESIZE);
    }

    *fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ASSERT_TRUE(*fd != -1);

    reserve_size = align_up(
        reserve_size + sizeof(platform_alloc_header),
        _page_size
    );
    initial_size = align_up(
        initial_size + sizeof(platform_alloc_header),
        _page_size
    );

    ASSERT_TRUE(ftruncate(*fd, reserve_size) == 0);

    void* base = mmap(
        NULL,
        reserve_size,
        PROT_NONE,
        MAP_SHARED,
        *fd,
        0
    );
    ASSERT_TRUE(base != MAP_FAILED);

    ASSERT_TRUE(
        mprotect(base, initial_size, PROT_READ | PROT_WRITE) == 0
    );

    platform_alloc_header* hdr = (platform_alloc_header *) base;
    hdr->reserved_size  = reserve_size;
    hdr->committed_size = initial_size;

    return (void*)((uintptr_t)base + sizeof(platform_alloc_header));
}

inline
bool platform_shared_alloc_grow(
    void* ptr,
    size_t new_size
) NO_EXCEPT
{
    if (!_page_size) {
        _page_size = (int32)sysconf(_SC_PAGESIZE);
    }

    platform_alloc_header* hdr = (platform_alloc_header*)((uintptr_t)ptr - sizeof(platform_alloc_header));

    const size_t new_committed = align_up(new_size + sizeof(platform_alloc_header), _page_size);

    ASSERT_TRUE(new_committed > hdr->committed_size);
    ASSERT_TRUE(new_committed <= hdr->reserved_size);

    ASSERT_TRUE(
        mprotect(hdr, new_committed, PROT_READ | PROT_WRITE) == 0
    );

    STATS_INCREMENT_BY(
        DEBUG_COUNTER_MEM_ALLOC,
        new_committed - hdr->committed_size
    );

    hdr->committed_size = new_committed;
    return true;
}

inline
bool platform_shared_alloc_shrink(
    void* ptr,
    size_t new_size
) NO_EXCEPT
{
    if (!_page_size) {
        _page_size = (int32)sysconf(_SC_PAGESIZE);
    }

    platform_alloc_header* hdr =
        (platform_alloc_header*)((uintptr_t)ptr - sizeof(platform_alloc_header));

    const size_t new_committed =
        align_up(new_size + sizeof(platform_alloc_header), _page_size);

    ASSERT_TRUE(new_committed < hdr->committed_size);

    const size_t delta = hdr->committed_size - new_committed;

    ASSERT_TRUE(
        mprotect(
            (uint8_t*)hdr + new_committed,
            delta,
            PROT_NONE
        ) == 0
    );

    madvise(
        (uint8_t*)hdr + new_committed,
        delta,
        MADV_DONTNEED
    );

    hdr->committed_size = new_committed;
    STATS_DECREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, delta);

    return true;
}

inline
void* platform_shared_open(int32* __restrict fd, const char* __restrict name, size_t size) NO_EXCEPT
{
    *fd = shm_open(name, O_RDWR, 0666);
    ASSERT_TRUE(*fd != -1);

    size = align_up(size + sizeof(size_t), _page_size);

    void* shm_ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, *fd, 0);
    ASSERT_TRUE(shm_ptr);
    LOG_3("[INFO] Shared opened %n B", {DATA_TYPE_UINT64, &size});

    *((size_t *) shm_ptr) = size;

    return (void *) ((uintptr_t) shm_ptr + sizeof(size_t));
}

inline
void platform_shared_free(int32 fd, const char* name, void** ptr) NO_EXCEPT
{
    if (!ptr || !*ptr) {
        return;
    }

    platform_alloc_header* hdr = (platform_alloc_header*)((uintptr_t)(*ptr) - sizeof(platform_alloc_header));

    DEBUG_MEMORY_FREE((uintptr_t)hdr);

    munmap(hdr, hdr->reserved_size);

    *ptr = NULL;

    close(fd);

    shm_unlink(name);
}

FORCE_INLINE
void platform_shared_close(int32 fd) NO_EXCEPT
{
    close(fd);
}

#endif