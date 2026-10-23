/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_THREAD_POOL_C
#define COMS_THREADS_THREAD_POOL_C

#include "ThreadPool.h"
#include "../memory/MPMCQueueT.cpp"
#include "../memory/ThrdChunkMemory.cpp"

// Notifies the thread pool that a worker is about to become idle
static FORCE_INLINE
void thread_pool_notify_one(ThreadPool* const pool) NO_EXCEPT
{
    atomic_thread_fence(memory_order_seq_cst);
    if (pool->sleeping_cnt.load(memory_order_relaxed) > 0) {
        coms_sem_post(&pool->work_sem);
    }
}

// Runs a single job on the calling thread
// job points to a local copy, the queue slot is already released
static inline
void thread_pool_execute(ThreadPool* const pool, PoolWorker* const job) NO_EXCEPT
{
    ++pool->working_cnt;

    if (job->state_external) {
        job->state_external->store(POOL_WORKER_STATE_RUNNING, memory_order_relaxed);
    }

    LOG_3("[INFO] ThreadPool worker started");
    {
        PROFILE_DEBUG(PROFILE_THREADPOOL_WORK, (char *) NULL, PROFILE_FLAG_ADD_HISTORY);
        STATS_INCREMENT_DEBUG(DEBUG_COUNTER_THREAD_ACTIVE);
        if (job->mem_size && !job->mem) {
            // We try to allocate the requested memory until we can get it
            ASSERT_TRUE(job->mem_size < pool->thrd_mem.size);
            do {
                THRD_CHUNK_STACK_MEMORY(&pool->thrd_mem, (byte **) &job->mem, job->mem_size);
                if (job->mem) {
                    job->func(job);
                } else {
                    YieldProcessor();
                }
            } while (!job->mem);
        } else {
            job->func(job);
        }
        STATS_DECREMENT_DEBUG(DEBUG_COUNTER_THREAD_ACTIVE);
    }
    LOG_3("[INFO] ThreadPool worker ended");

    if (job->state_external) {
        job->state_external->store(POOL_WORKER_STATE_COMPLETED, memory_order_release);
    }

    // Must be the last access to anything the job references
    if (job->callback) {
        job->callback(job);
    }

    --pool->working_cnt;
}

static inline
THREAD_RETURN thread_pool_worker(void* arg) NO_EXCEPT
{
    THREAD_CURRENT_ID(_thread_local_id);
    THREAD_CPU_ID(_thread_cpu_id);
    ThreadPool* const pool = (ThreadPool *) arg;

    if (pool->debug_container) {
        _dmc = pool->debug_container->dmc;
        _log_memory = pool->debug_container->log_memory;
        _perf_stats = pool->debug_container->perf_stats;
        _stats_counter = pool->debug_container->stats_counter;
        _perf_current_scope = pool->debug_container->perf_current_scope;
        _perf_thread_history_count = pool->debug_container->perf_profile_history_count;
        _perf_thread_history = pool->debug_container->perf_profile_history;
    }

    LOG_2("[INFO] Thread pool worker starting up");
    STATS_INCREMENT_DEBUG(DEBUG_COUNTER_THREAD);

    // Setting up thread local rng state
    rand_setup();

    PoolWorker job;

    while (true) {
        THREAD_TICK(_thread_local_id);

        // Fast path = work available, no sleeping involved
        if (queue_dequeue(&pool->work_queue, &job)) {
            thread_pool_execute(pool, &job);
            continue;
        }

        if (pool->state.load() == THREAD_POOL_STATE_CANCELING) {
            break;
        }

        // We mark this worker as asleep
        ++pool->sleeping_cnt;
        atomic_thread_fence(memory_order_seq_cst);

        // Re-check after announcing ourselves = this avoids missing an enqueue
        if (queue_dequeue(&pool->work_queue, &job)) {
            --pool->sleeping_cnt;
            thread_pool_execute(pool, &job);

            continue;
        }

        if (pool->state.load() == THREAD_POOL_STATE_CANCELING) {
            --pool->sleeping_cnt;
            break;
        }

        LOG_4("Worker %d: about to wait", {DATA_TYPE_INT32, &_thread_local_id});
        coms_sem_wait(&pool->work_sem);
        LOG_4("Worker %d: woke up", {DATA_TYPE_INT32, &_thread_local_id});

        --pool->sleeping_cnt;
    }

    LOG_2("[INFO] Thread pool worker shutting down");
    STATS_DECREMENT_DEBUG(DEBUG_COUNTER_THREAD);

    // We tell the thread pool that this worker thread is shutting down
    --pool->thread_cnt;

    return (THREAD_RETURN_BODY) NULL;
}

static FORCE_INLINE CONSTEXPR
int32 thread_pool_next_pow2(int32 v) NO_EXCEPT
{
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    return v + 1;
}

// The job slots are cache line aligned (PoolWorker is one cache line) to avoid false sharing
static FORCE_INLINE CONSTEXPR
int32 thread_pool_alignment(int32 alignment) NO_EXCEPT
{
    return alignment < ASSUMED_CACHE_LINE_SIZE ? ASSUMED_CACHE_LINE_SIZE : alignment;
}

static FORCE_INLINE CONSTEXPR
size_t thread_pool_size(
    int32 thread_count,
    int32 worker_capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    return (size_t) thread_pool_alignment(alignment) // queue_init() aligns the slots up
        + sizeof(PoolWorker) * thread_pool_next_pow2(worker_capacity)
        + ASSUMED_CACHE_LINE_SIZE // queue_init() aligns the sequence array up
        + sizeof(atomic<size_t>) * thread_pool_next_pow2(worker_capacity)
        + alignof(coms_pthread_t)
        + sizeof(coms_pthread_t) * thread_count;
}

static
void thread_pool_setup(
    ThreadPool* const pool,
    byte* const buf,
    int32 thread_count,
    int32 worker_capacity,
    int32 alignment
) NO_EXCEPT
{
    queue_init(&pool->work_queue, buf, worker_capacity, (uint32) alignment);

    pool->thread_handles = NULL;
    if (!pool->is_detached) {
        pool->thread_handles = (coms_pthread_t *) ALIGN_UP(
            (uintptr_t) (pool->work_queue.sequence + worker_capacity),
            alignof(coms_pthread_t)
        );

        memset(pool->thread_handles, 0, sizeof(coms_pthread_t) * thread_count);
    }

    pool->working_cnt.store(0);
    pool->thread_cnt.store(0);
    pool->sleeping_cnt.store(0);
    coms_sem_init(&pool->work_sem, 0);

    pool->state.store(THREAD_POOL_STATE_RUNNING);

    coms_pthread_t thread;
    for (pool->size = 0; pool->size < thread_count; ++pool->size) {
        ++pool->thread_cnt;
        coms_pthread_create(&thread, NULL, thread_pool_worker, pool);
        THREAD_LOG_NAME(thread.id, "pool");

        if (pool->is_detached) {
            coms_pthread_detach(thread);
        } else {
            pool->thread_handles[pool->size] = thread;
        }
    }

    LOG_2(
        "[INFO] %d threads running",
        {DATA_TYPE_INT64, (void *) &_stats_counter->stats[
            _stats_counter->pos * DEBUG_COUNTER_SIZE + DEBUG_COUNTER_THREAD
        ]}
    );
}

void thread_pool_alloc(
    ThreadPool* const pool,
    int32 thread_count,
    int32 worker_capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_THREAD_POOL_ALLOC);
    LOG_1(
        "[INFO] Allocating thread pool with %d threads and %d queue length",
        {DATA_TYPE_INT32, &thread_count},
        {DATA_TYPE_INT32, &worker_capacity}
    );

    worker_capacity = thread_pool_next_pow2(worker_capacity);
    alignment = thread_pool_alignment(alignment);

    const size_t size = thread_pool_size(thread_count, worker_capacity, alignment);
    byte* const buf = (byte *) platform_alloc_aligned(size, size, alignment);
    DEBUG_MEMORY_INIT((uintptr_t) buf, size);
    DEBUG_MEMORY_NAME("ThreadPool", buf);

    thread_pool_setup(pool, buf, thread_count, worker_capacity, alignment);
}

void thread_pool_init(
    ThreadPool* const pool,
    BufferMemory* const buf,
    int32 thread_count,
    int32 worker_capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_THREAD_POOL_ALLOC);
    LOG_1(
        "[INFO] Initializing thread pool with %d threads and %d queue length",
        {DATA_TYPE_INT32, &thread_count},
        {DATA_TYPE_INT32, &worker_capacity}
    );

    worker_capacity = thread_pool_next_pow2(worker_capacity);
    alignment = thread_pool_alignment(alignment);

    const size_t size = thread_pool_size(thread_count, worker_capacity, alignment);
    byte* const buffer = (byte *) memory_get(buf, size, alignment);
    DEBUG_MEMORY_SUBREGION((uintptr_t) buffer, size);

    thread_pool_setup(pool, buffer, thread_count, worker_capacity, alignment);
}

// Finishes all queued jobs, then shuts down the workers
void thread_pool_destroy(ThreadPool* const pool) NO_EXCEPT
{
    pool->state.store(THREAD_POOL_STATE_CANCELING);

    // Wake every potential sleeper.
    for (int32 i = 0; i < pool->size; ++i) {
        coms_sem_post(&pool->work_sem);
    }

    if (pool->is_detached) {
        while (pool->thread_cnt.load() > 0) {
            YieldProcessor();
        }
    } else {
        for (int32 i = 0; i < pool->size; ++i) {
            coms_pthread_join(pool->thread_handles[i], NULL);
        }
    }

    pool->state.store(THREAD_POOL_STATE_COMPLETED);
    coms_sem_destroy(&pool->work_sem);
}

// Checks if the thread pool is running as it is supposed to
// Currently this means we are checking if all threads are still "active"
// Active means that we can use them for work/tasks
inline
bool thread_pool_healthy(const ThreadPool* const pool, BufferMemory* const mem) NO_EXCEPT
{
    // We cannot check a thread pool that uses detached threads
    ASSERT_TRUE(!pool->is_detached);

    if (pool->thread_cnt.load() != pool->size) {
        ASSERT_THROW();

        return false;
    }

    return coms_pthreads_running(pool->thread_handles, pool->size, mem);
}

// Tries to fix threads that are no longer running but should be running
inline
void thread_pool_fix(ThreadPool* const pool) NO_EXCEPT
{
    // We cannot fix a thread pool that uses detached threads
    ASSERT_TRUE(!pool->is_detached);

    for (int32 i = 0; i < pool->size; ++i) {
        if (!coms_pthread_running(pool->thread_handles[i])) {
            ++pool->thread_cnt;
            coms_pthread_create(&pool->thread_handles[i], NULL, thread_pool_worker, pool);
            THREAD_LOG_NAME(pool->thread_handles[i].id, "pool");
        }
    }
}

// Zero-copy enqueue: claim a slot, fill it in place, then publish it with thread_pool_add_work_end()
// The slot is zeroed and job->id is already assigned
// Returns NULL if the queue is full
// WARNING: Keep the time between start and end short.
//      Consumers can't see this slot, or any slot claimed after it, until it is published
inline
PoolWorker* thread_pool_add_work_start(ThreadPool* const pool) NO_EXCEPT
{
    PoolWorker* const slot = queue_enqueue_start(&pool->work_queue);
    if (!slot) {
        return NULL;
    }

    memset(slot, 0, sizeof(PoolWorker));
    slot->id = (uint64) queue_enqueue_position(&pool->work_queue, slot) + 1;

    return slot;
}

// Like thread_pool_add_work_start() but waits until a slot is free
inline
PoolWorker* thread_pool_add_work_start_wait(ThreadPool* const pool) NO_EXCEPT
{
    PoolWorker* const slot = queue_enqueue_start_wait(&pool->work_queue);

    memset(slot, 0, sizeof(PoolWorker));
    slot->id = (uint64) queue_enqueue_position(&pool->work_queue, slot) + 1;

    return slot;
}

// WARNING: job must not be accessed after this call, it may already be consumed and reused
inline
void thread_pool_add_work_end(ThreadPool* const pool, PoolWorker* const job) NO_EXCEPT
{
    if (job->state_external) {
        job->state_external->store(POOL_WORKER_STATE_WAITING, memory_order_relaxed);
    }

    queue_enqueue_end(&pool->work_queue, job);
    thread_pool_notify_one(pool);
}

// Returns the job id or 0 if the queue is full
static inline
uint64 thread_pool_add_work(ThreadPool* const pool, const PoolWorker* const job) NO_EXCEPT
{
    PoolWorker* const slot = queue_enqueue_start(&pool->work_queue);
    if (!slot) {
        return 0;
    }

    *slot = *job;
    slot->id = (uint64) queue_enqueue_position(&pool->work_queue, slot) + 1;

    // Must be read before publishing, afterwards the slot may already be reused
    const uint64 id = slot->id;
    thread_pool_add_work_end(pool, slot);

    return id;
}

inline
uint64 thread_pool_add_work(ThreadPool* const pool, const PoolWorker& job) NO_EXCEPT
{
    return thread_pool_add_work(pool, &job);
}

// Like thread_pool_add_work() but waits until a slot is free, always succeeds
static inline
uint64 thread_pool_add_work_wait(ThreadPool* const pool, const PoolWorker* const job) NO_EXCEPT
{
    PoolWorker* const slot = queue_enqueue_start_wait(&pool->work_queue);

    *slot = *job;
    slot->id = (uint64) queue_enqueue_position(&pool->work_queue, slot) + 1;

    const uint64 id = slot->id;
    thread_pool_add_work_end(pool, slot);

    return id;
}

inline
uint64 thread_pool_add_work_wait(ThreadPool* const pool, const PoolWorker& job) NO_EXCEPT
{
    return thread_pool_add_work_wait(pool, &job);
}

// sink = atomic<int32>*, the caller initializes it to the number of jobs and waits for 0
// Use with thread_pool_wait()
inline
void thread_pool_complete_counter(void* arg) NO_EXCEPT
{
    const PoolWorker* const job = (const PoolWorker *) arg;
    ((atomic<int32> *) job->sink)->fetch_sub(1, memory_order_release);
}

// sink = MPMCQueueT<PoolWorker>*, pushes a copy of the completed job (blocks while full)
// Result data should be referenced through job->arg, not job->mem (already released)
// For an MPSC result queue write the same 2 lines with your MPSC enqueue
inline
void thread_pool_complete_queue(void* arg) NO_EXCEPT
{
    const PoolWorker* const job = (const PoolWorker *) arg;
    queue_enqueue_wait((MPMCQueueT<PoolWorker> *) job->sink, job);
}

/**
 * Waits until counter reaches 0 (see thread_pool_complete_counter)
 *
 * help_pool: If not NULL, the waiting thread executes queued jobs of that pool while waiting.
 *      This is required if you wait from inside a job (otherwise all workers may end up
 *      waiting on each other), and it generally reduces latency.
 *      WARNING: The jobs then run on the caller thread (its thread locals, rng state, ...)
 *      WARNING: A helped job may itself wait and help, so nested waits nest on the stack
 *          (depth is bounded by the number of waiting jobs in the queue)
 * sleep_time: 0 = spin with YieldProcessor(), otherwise usleep(sleep_time) between checks
 * max_sleep: 0 = wait forever, otherwise give up after sleeping that long (needs sleep_time > 0)
 *
 * Returns true if the counter reached 0
 */
inline
bool thread_pool_wait(
    const atomic<int32>* const counter,
    ThreadPool* const help_pool = NULL,
    uint64 sleep_time = 0,
    uint64 max_sleep = 0
) NO_EXCEPT
{
    uint64 slept = 0;
    PoolWorker job;

    while (counter->load(memory_order_acquire) > 0) {
        if (help_pool && queue_dequeue(&help_pool->work_queue, &job)) {
            thread_pool_execute(help_pool, &job);
            continue;
        }

        if (!sleep_time) {
            YieldProcessor();
            continue;
        }

        if (max_sleep && slept >= max_sleep) {
            return false;
        }

        usleep(sleep_time);
        slept += sleep_time;
    }

    return true;
}

#endif