/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_THREAD_POOL_C
#define COMS_THREADS_THREAD_POOL_C

#include "ThreadPool.h"
#include "../memory/MPMCWorkTrackingQueueT.cpp"

// Notifies the thread pool that a worker is about to become idle
static FORCE_INLINE
void thread_pool_notify_one(ThreadPool* const pool) NO_EXCEPT
{
    if (pool->sleeping_cnt.load() > 0) {
        coms_sem_post(&pool->work_sem);
    }
}

static inline
THREAD_RETURN thread_pool_worker(void* arg) NO_EXCEPT
{
    THREAD_CURRENT_ID(_thread_local_id);
    THREAD_CPU_ID(_thread_cpu_id);
    ThreadPool* const pool = (ThreadPool *) arg;

    ++pool->thread_cnt;

    if (pool->debug_container) {
        _log_fp = pool->debug_container->log_fp;
        _log_memory = pool->debug_container->log_memory;
        _dmc = pool->debug_container->dmc;
        _perf_stats = pool->debug_container->perf_stats;
        _perf_active = pool->debug_container->perf_active;
        _stats_counter_active = pool->debug_container->stats_counter_active;
        _dmc_active = pool->debug_container->dmc_active;
        _stats_counter = pool->debug_container->stats_counter;
        _stats_counter_persistent = pool->debug_container->stats_counter_persistent;
    }

    LOG_2("[INFO] Thread pool worker starting up");
    STATS_INCREMENT_DEBUG(DEBUG_COUNTER_THREAD);

    // Setting up thread local rng state
    rand_setup();

    PoolWorker* work;

    while (true) {
        THREAD_TICK(_thread_local_id);

        work = queue_dequeue_keep(&pool->work_queue);
        if (!work) {
            if (pool->state.load() == THREAD_POOL_STATE_CANCELING) {
                --pool->sleeping_cnt;
                break;
            }

            // We mark this worker as asleep
            ++pool->sleeping_cnt;

            // This avoids missing an enqueue
            work = queue_dequeue_keep(&pool->work_queue);
            if (work) {
                coms_sem_trywait(&pool->work_sem);
            } else {
                if (pool->state.load() == THREAD_POOL_STATE_CANCELING) {
                    --pool->sleeping_cnt;
                    break;
                }

                LOG_1("Worker %d: about to wait", {DATA_TYPE_INT32, &_thread_local_id});
                const int32 ret = coms_sem_wait(&pool->work_sem);
                LOG_1("Worker %d: woke up", {DATA_TYPE_INT32, &_thread_local_id});
                LOG_1("Worker %d: ret", {DATA_TYPE_INT32, &ret});
            }

            --pool->sleeping_cnt;
            if (!work) {
                continue;
            }
        }

        ++pool->working_cnt;
        work->state.store(POOL_WORKER_STATE_RUNNING);

        LOG_3("[INFO] ThreadPool worker started");
        {
            PROFILE_DEBUG(PROFILE_THREADPOOL_WORK, (char *) NULL, PROFILE_FLAG_ADD_HISTORY);
            STATS_INCREMENT_DEBUG(DEBUG_COUNTER_THREAD_ACTIVE);
            if (work->mem_size) {
                // @bug we need to wait if we don't have enough memory available
                THRD_CHUNK_STACK_MEMORY(&pool->thrd_mem, &work->mem, work->mem_size);
                work->func(work);
            } else {
                work->func(work);
            }
            STATS_DECREMENT_DEBUG(DEBUG_COUNTER_THREAD_ACTIVE);
        }
        LOG_3("[INFO] ThreadPool worker ended");

        if (work->callback) {
            work->callback(work);
        }

        work->state.store(POOL_WORKER_STATE_COMPLETED);
        if (work->automatic_release) {
            queue_dequeue_release(&pool->work_queue, work);
        }

        --pool->working_cnt;
    }

    // We tell the thread pool that this worker thread is shutting down
    --pool->thread_cnt;

    LOG_2("[INFO] Thread pool worker shutting down");
    STATS_DECREMENT_DEBUG(DEBUG_COUNTER_THREAD);

    return (THREAD_RETURN_BODY) NULL;
}

static FORCE_INLINE CONSTEXPR
size_t thread_pool_size(
    int32 thread_count,
    int32 worker_capacity
) NO_EXCEPT
{
    return queue_size(sizeof(PoolWorker), worker_capacity)
        + sizeof(coms_pthread_t) * thread_count
        + alignof(coms_pthread_t);
}

void thread_pool_alloc(
    ThreadPool* const pool,
    int32 thread_count,
    int worker_capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_THREAD_POOL_ALLOC);
    LOG_1(
        "[INFO] Allocating thread pool with %d threads and %d queue length",
        {DATA_TYPE_INT32, &thread_count},
        {DATA_TYPE_INT32, &worker_capacity}
    );

    worker_capacity = queue_next_pow2(worker_capacity);

    const size_t q_size = queue_size(sizeof(PoolWorker), worker_capacity);
    byte* buf = (byte *) platform_alloc_aligned(
        q_size + sizeof(coms_pthread_t) * thread_count + alignof(coms_pthread_t),
        q_size + sizeof(coms_pthread_t) * thread_count + alignof(coms_pthread_t),
        alignment
    );
    queue_init(&pool->work_queue, buf, worker_capacity, alignment);

    DEBUG_MEMORY_NAME("Threadpool", pool->work_queue.memory);

    if (!pool->is_detached) {
        pool->thread_handles = (coms_pthread_t *) align_up(
            (uintptr_t) (buf + q_size),
            alignof(coms_pthread_t)
        );

        memset(pool->thread_handles, 0, sizeof(coms_pthread_t) * thread_count);
    }

    coms_sem_init(&pool->work_sem, 0);
    pool->sleeping_cnt.store(0);

    coms_pthread_t thread;
    for (pool->size = 0; pool->size < thread_count; ++pool->size) {
        coms_pthread_create(&thread, NULL, thread_pool_worker, pool);
        THREAD_LOG_NAME(thread.id, "pool");

        if (pool->is_detached) {
            coms_pthread_detach(thread);
        } else {
            pool->thread_handles[pool->size] = thread;
        }
    }

    pool->state.store(THREAD_POOL_STATE_RUNNING);

    LOG_2(
        "[INFO] %d threads running",
        {DATA_TYPE_INT64, (void *) &_stats_counter->stats[
            _stats_counter->pos * DEBUG_COUNTER_SIZE + DEBUG_COUNTER_THREAD
        ]}
    );
}

void thread_pool_init(
    ThreadPool* const pool,
    BufferMemory* const buf,
    int32 thread_count,
    int worker_capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_THREAD_POOL_ALLOC);
    LOG_1(
        "[INFO] Initializing thread pool with %d threads and %d queue length",
        {DATA_TYPE_INT32, &thread_count},
        {DATA_TYPE_INT32, &worker_capacity}
    );

    worker_capacity = queue_next_pow2(worker_capacity);

    const size_t q_size = queue_size(sizeof(PoolWorker), worker_capacity);
    byte* buffer = (byte *) memory_get(buf,
        q_size + sizeof(coms_pthread_t) * thread_count + alignof(coms_pthread_t),
        alignment
    );
    queue_init(&pool->work_queue, buffer, worker_capacity, alignment);

    DEBUG_MEMORY_NAME("Threadpool", pool->work_queue.memory);

    if (!pool->is_detached) {
        pool->thread_handles = (coms_pthread_t *) align_up(
            (uintptr_t) (buffer + q_size),
            alignof(coms_pthread_t)
        );

        memset(pool->thread_handles, 0, sizeof(coms_pthread_t) * thread_count);
    }

    DEBUG_MEMORY_SUBREGION(
        (uintptr_t) pool->work_queue.memory,
        sizeof(PoolWorker) * worker_capacity + sizeof(coms_pthread_t) * thread_count
    );

    coms_sem_init(&pool->work_sem, 0);
    pool->sleeping_cnt.store(0);

    coms_pthread_t thread;
    for (pool->size = 0; pool->size < thread_count; ++pool->size) {
        coms_pthread_create(&thread, NULL, thread_pool_worker, pool);
        THREAD_LOG_NAME(thread.id, "pool");

        if (pool->is_detached) {
            coms_pthread_detach(thread);
        } else {
            pool->thread_handles[pool->size] = thread;
        }
    }

    pool->state.store(THREAD_POOL_STATE_RUNNING);

    LOG_2(
        "[INFO] %d threads running",
        {DATA_TYPE_INT64, (void *) &_stats_counter->stats[
            _stats_counter->pos * DEBUG_COUNTER_SIZE + DEBUG_COUNTER_THREAD
        ]}
    );
}

void thread_pool_destroy(ThreadPool* const pool) NO_EXCEPT
{
    pool->state.store(THREAD_POOL_STATE_CANCELING);
    for (int i = 0; i < pool->size; ++i) {
        coms_pthread_join(pool->thread_handles[i], NULL);
    }

    pool->state.store(THREAD_POOL_STATE_COMPLETED);
    coms_sem_destroy(&pool->work_sem);
}

// Checks if the thread pool is running as it is supposed to
// Currently this means we are checking if all threads are still "active"
// Active means that we can use them for work/tasks
inline
bool thread_pool_healthy(const ThreadPool* const pool) NO_EXCEPT
{
    if (pool->thread_cnt.load() != pool->size) {
        return false;
    }

    if (!pool->is_detached) {
        for (int i = 0; i < pool->size; ++i) {
            if (!coms_pthread_running(pool->thread_handles[i])) {
                return false;
            }
        }
    }

    return true;
}

// Tries to fix threads that are no longer running but should be running
inline
void thread_pool_fix(ThreadPool* const pool) NO_EXCEPT
{
    // We cannot fix a thread pool that uses detached threads
    ASSERT_TRUE(!pool->is_detached);

    for (int i = 0; i < pool->size; ++i) {
        if (!coms_pthread_running(pool->thread_handles[i])) {
            coms_pthread_create(&pool->thread_handles[i], NULL, thread_pool_worker, pool);
            THREAD_LOG_NAME(pool->thread_handles[i].id, "pool");
        }
    }
}

PoolWorker* thread_pool_add_work(ThreadPool* const pool, const PoolWorker* job) NO_EXCEPT
{
    PoolWorker* const temp_job = (PoolWorker *) queue_enqueue_start(&pool->work_queue);
    if (!temp_job) {
        ASSERT_THROW();

        return NULL;
    }

    memcpy(temp_job, job, sizeof(PoolWorker));
    temp_job->state.store(POOL_WORKER_STATE_WAITING);

    // +1 because otherwise the very first job would be id = 0 which is not a valid id
    uint32 id = ++pool->id_counter;
    if (!pool->id_counter.load()) UNLIKELY {
        // ID of 0 is not allowed
        id = ++pool->id_counter;
    }

    temp_job->id.store(id);

    queue_enqueue_end(temp_job);
    thread_pool_notify_one(pool);

    return temp_job;
}

PoolWorker* thread_pool_add_work(ThreadPool* const pool, const PoolWorker& job) NO_EXCEPT
{
    PoolWorker* const temp_job = (PoolWorker *) queue_enqueue_start(&pool->work_queue);
    if (!temp_job) {
        ASSERT_THROW();

        return NULL;
    }

    memcpy(temp_job, &job, sizeof(PoolWorker));
    temp_job->state.store(POOL_WORKER_STATE_WAITING);

    // +1 because otherwise the very first job would be id = 0 which is not a valid id
    uint32 id = ++pool->id_counter;
    if (!pool->id_counter.load()) UNLIKELY {
        // ID of 0 is not allowed
        id = ++pool->id_counter;
    }

    temp_job->id.store(id);

    queue_enqueue_end(temp_job);
    thread_pool_notify_one(pool);

    return temp_job;
}

// This is basically the same as thread_pool_add_work but allows us to directly write into the memory in the caller
// This makes it faster, since we can avoid a memcpy
inline
PoolWorker* thread_pool_add_work_start(ThreadPool* const pool) NO_EXCEPT
{
    PoolWorker* const temp_job = (PoolWorker *) queue_enqueue_start(&pool->work_queue);
    if (!temp_job) {
        return NULL;
    }

    memset(temp_job, 0, sizeof(PoolWorker));
    temp_job->state.store(POOL_WORKER_STATE_WAITING);

    // +1 because otherwise the very first job would be id = 0 which is not a valid id
    uint32 id = ++pool->id_counter;
    if (!pool->id_counter) UNLIKELY {
        // ID of 0 is not allowed
        id = ++pool->id_counter;
    }

    temp_job->id.store(id);

    return temp_job;
}

inline
void thread_pool_add_work_end(ThreadPool* const pool, PoolWorker* const job) NO_EXCEPT
{
    queue_enqueue_end(job);
    thread_pool_notify_one(pool);
}

FORCE_INLINE
void thread_pool_work_release(ThreadPool* const pool, const PoolWorker* job) NO_EXCEPT
{
    queue_dequeue_release(&pool->work_queue, job);
}

// This joins the work not the actual threads in the thread pool
// We are not marking jobs const since it may change during the joining process (e.g. the state)
// Returns the mask of the completed tasks or 0xFFFFFFFFFFFFFFFF for all completed
inline
uint64 thread_pool_join(
    ThreadPool* const pool,
    const PoolWorker* const jobs,
    int32 count,
    uint64 sleep_time = 0,
    uint64 max_sleep = 0
) NO_EXCEPT
{
    ASSERT_TRUE(count <= 64);

    uint64 completed_mask = 0;
    const uint64 all_done_mask = (count == 64 ? UINT64_MAX : ((1ULL << count) - 1));

    uint64 current_sleep = 0;

    // Loop until all jobs have been marked completed
    // We use a bitmask to avoid re-checking already validated jobs
    // If we wouldn't do that we may risk that a job got re-used for a new job giving a false negative.
    while (completed_mask != all_done_mask && max_sleep > current_sleep) {
        for (int32 i = 0; i < count; ++i) {
            const uint64 bit = 1ULL << i;

            if (completed_mask & bit) {
                continue;
            }

            if (!jobs[i].id
                || jobs[i].state.load() == POOL_WORKER_STATE_COMPLETED
            ) {
                completed_mask |= bit;

                // If we don't automatically release we have to do it now
                if (!jobs[i].automatic_release) {
                    thread_pool_work_release(pool, &jobs[i]);
                }
            }
        }

        if (sleep_time && completed_mask != all_done_mask) {
            usleep(sleep_time);
            current_sleep += sleep_time;
        }
    }

    ASSERT_TRUE(max_sleep > current_sleep);

    return completed_mask == all_done_mask ? 0xFFFFFFFFFFFFFFFF : completed_mask;
}

// This joins the work not the actual threads in the thread pool
// Returns the mask of the completed tasks or 0xFFFFFFFFFFFFFFFF for all completed
inline
uint64 thread_pool_join(
    ThreadPool* const pool,
    const PoolWorker* const* const jobs, int32 count,
    uint64 sleep_time = 0, uint64 max_sleep = 0
) NO_EXCEPT
{
    ASSERT_TRUE(count <= 64);

    uint64 completed_mask = 0;
    const uint64 all_done_mask = (count == 64 ? UINT64_MAX : ((1ULL << count) - 1));

    uint64 current_sleep = 0;

    // Loop until all jobs have been marked completed
    // We use a bitmask to avoid re-checking already validated jobs
    // If we wouldn't do that we may risk that a job got re-used for a new job giving a false negative.
    while (max_sleep >= current_sleep) {
        for (int32 i = 0; i < count; ++i) {
            const uint64 bit = 1ULL << i;

            if (completed_mask & bit) {
                continue;
            }

            if (!jobs[i] || !jobs[i]->id
                || jobs[i]->state.load() == POOL_WORKER_STATE_COMPLETED
            ) {
                completed_mask |= bit;

                // If we don't automatically release we have to do it now
                if (!jobs[i]->automatic_release) {
                    thread_pool_work_release(pool, jobs[i]);
                }
            }
        }

        if (completed_mask == all_done_mask) {
            return 0xFFFFFFFFFFFFFFFF;
        }

        if (sleep_time) {
            usleep(sleep_time);
            current_sleep += sleep_time;
        }
    }

    ASSERT_TRUE(max_sleep > current_sleep);

    return completed_mask;
}

#endif