/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_THREADS_THREAD_POOL_H
#define COMS_THREADS_THREAD_POOL_H

#include "../stdlib/Stdlib.h"
#include "../memory/PersistentQueueT.h"
#include "../memory/BufferMemory.h"
#include "../log/DebugMemory.h"
#include "Thread.h"
#include "Atomic.h"
#include "ThreadJob.h"
#include "../log/DebugContainer.h"
#include "../utils/RandomUtils.h"

enum ThreadPoolState : int32 {
    THREAD_POOL_STATE_CANCELING = -1,
    THREAD_POOL_STATE_COMPLETED = 0,
    THREAD_POOL_STATE_WAITING = 1,
    THREAD_POOL_STATE_RUNNING = 2,

    // The job itself may pause for whatever reason (e.g. wait for elements in a threaded queue)
    THREAD_POOL_STATE_PAUSED = 3,
};

struct ThreadPool {
    // This is not a threaded queue since we want to handle the mutex in here, not in the queue for finer control
    // @question Currently we overwrite jobs in a certain amount of time, should we create a cache that remains until manually deleted
    //          Currently we might overwrite a job result before we check it
    //          We probably need to use ChunkMemory combined with a queue
    PersistentQueueT<PoolWorker> work_queue;

    // @performance Could it make more sense to use a spinlock for the thread pool?
    // Is probably overkill if we cannot really utilize it a lot -> we would have a lot of spinning
    mutex work_mutex;
    mutex_cond work_cond;
    mutex_cond working_cond;

    // By design the working_cnt is <= thread_cnt
    atomic_32 int32 working_cnt;
    atomic_32 int32 thread_cnt;

    int32 size;

    // @question Why are we using atomics if we are using mutex at the same time?
    //          I think because the mutex is actually intended for the queue.
    ThreadPoolState state;
    atomic_32 uint32 id_counter;

    DebugContainer* debug_container;
};

// @performance Can we optimize this? This is a critical function.
// If we have a small worker the "spinup"/"re-activation" time is from utmost importance
static inline
THREAD_RETURN thread_pool_worker(void* arg) NO_EXCEPT
{
    THREAD_CURRENT_ID(_thread_local_id);
    THREAD_CPU_ID(_thread_cpu_id);
    ThreadPool* const pool = (ThreadPool *) arg;

    if (pool->debug_container) {
        _log_fp = pool->debug_container->log_fp;
        _log_memory = pool->debug_container->log_memory;
        _dmc = pool->debug_container->dmc;
        _perf_stats = pool->debug_container->perf_stats;
        _perf_active = pool->debug_container->perf_active;
        _stats_counter_active = pool->debug_container->stats_counter_active;
        _stats_counter = pool->debug_container->stats_counter;

        // @question Why do we even need to to this?
        *_perf_active = *pool->debug_container->perf_active;
        *_stats_counter_active = *pool->debug_container->stats_counter_active;
    }

    // @bug Why doesn't this work? There must be some threading issue
    LOG_2("[INFO] Thread pool worker starting up");
    STATS_INCREMENT(DEBUG_COUNTER_THREAD);

    // Setting up thread local rng state
    rand_setup();

    PoolWorker* work;

    while (true) {
        THREAD_TICK(_thread_local_id);
        {
            // @performance Would a spinlock be faster
            MutexGuard _guard(&pool->work_mutex);

            while (pool->state >= THREAD_POOL_STATE_RUNNING && queue_is_empty(&pool->work_queue)) {
                coms_pthread_cond_wait(&pool->work_cond, &pool->work_mutex);
            }

            if (pool->state <= THREAD_POOL_STATE_WAITING) {
                break;
            }

            work = queue_dequeue_keep(&pool->work_queue);
        }

        // @Note Why are we using atomic operations for some of the stuff below?
        //      This only makes sense if the work/job pointer is shared across multiple threads
        //      If it is only stored in the thread itself and the calling thread we don't need atomics
        //      As a result we are now avoiding the use of atomics in some cases (see commented code)

        // When the worker functions of the thread pool get woken up it is possible that the work is already dequeued
        // by another thread -> we need to check if the work is actually valid
        if (atomic_get_release((int32 *) &work->state) <= POOL_WORKER_STATE_COMPLETED) {
            atomic_set_release((int32 *) &work->state, POOL_WORKER_STATE_COMPLETED);
            continue;
        }

        atomic_increment_release(&pool->working_cnt);
        atomic_set_release((int32 *) &work->state, POOL_WORKER_STATE_RUNNING);

        LOG_3("[INFO] ThreadPool worker started");
        {
            PROFILE(PROFILE_THREADPOOL_WORK, NULL, PROFILE_FLAG_ADD_HISTORY);
            STATS_INCREMENT(DEBUG_COUNTER_THREAD_ACTIVE);
            work->func(work);
            STATS_DECREMENT(DEBUG_COUNTER_THREAD_ACTIVE);
        }
        LOG_3("[INFO] ThreadPool worker ended");

        if (work->callback) {
            work->callback(work);
        }

        atomic_set_release((int32 *) &work->state, POOL_WORKER_STATE_COMPLETED);
        if (work->automatic_release) {
            queue_dequeue_release(&pool->work_queue, work);
        }

        atomic_decrement_release(&pool->working_cnt);

        // Signal that we ran out of work (maybe the main thread needs this info)
        // This is not required for the thread pool itself but maybe some other part of the main thread wants to know
        if (atomic_get_relaxed(&pool->working_cnt) == 0) {
            coms_pthread_cond_signal(&pool->working_cond);
        }
    }

    // We tell the thread pool that this worker thread is shutting down
    atomic_decrement_release(&pool->thread_cnt);
    coms_pthread_cond_signal(&pool->working_cond);

    LOG_2("[INFO] Thread pool worker shutting down");
    STATS_DECREMENT(DEBUG_COUNTER_THREAD);

    return (THREAD_RETURN_BODY) NULL;
}

void thread_pool_alloc(
    ThreadPool* const pool,
    int32 thread_count,
    int worker_capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE(PROFILE_THREAD_POOL_ALLOC);
    LOG_1(
        "[INFO] Allocating thread pool with %d threads and %d queue length",
        {DATA_TYPE_INT32, &thread_count},
        {DATA_TYPE_INT32, &worker_capacity}
    );

    queue_alloc(&pool->work_queue, worker_capacity, worker_capacity, alignment);
    DEBUG_MEMORY_NAME("Threadpool", pool->work_queue.memory);

    pool->thread_cnt = thread_count;

    // @todo switch from pool mutex and pool cond to threadjob mutex/cond
    //      thread_pool_wait etc. should just iterate over all mutexes
    mutex_init(&pool->work_mutex, NULL);
    coms_pthread_cond_init(&pool->work_cond, NULL);
    coms_pthread_cond_init(&pool->working_cond, NULL);

    pool->state = THREAD_POOL_STATE_RUNNING;

    coms_pthread_t thread;
    for (pool->size = 0; pool->size < thread_count; ++pool->size) {
        coms_pthread_create(&thread, NULL, thread_pool_worker, pool);
        THREAD_LOG_NAME(thread.id, "pool");
        coms_pthread_detach(thread);
    }

    LOG_2(
        "[INFO] %d threads running",
        {DATA_TYPE_INT64, (void *) &_stats_counter->stats[atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE + DEBUG_COUNTER_THREAD]}
    );
}

void thread_pool_create(
    ThreadPool* const pool,
    BufferMemory* const buf,
    int32 thread_count,
    int worker_capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE(PROFILE_THREAD_POOL_ALLOC);
    LOG_1(
        "[INFO] Creating thread pool with %d threads and %d queue length",
        {DATA_TYPE_INT32, &thread_count},
        {DATA_TYPE_INT32, &worker_capacity}
    );

    queue_init(&pool->work_queue, buf, worker_capacity, alignment);

    pool->thread_cnt = thread_count;

    // @todo switch from pool mutex and pool cond to threadjob mutex/cond
    //      thread_pool_wait etc. should just iterate over all mutexes
    mutex_init(&pool->work_mutex, NULL);
    coms_pthread_cond_init(&pool->work_cond, NULL);
    coms_pthread_cond_init(&pool->working_cond, NULL);

    // No atomic operation required here
    pool->state = THREAD_POOL_STATE_RUNNING;

    coms_pthread_t thread;
    for (pool->size = 0; pool->size < thread_count; ++pool->size) {
        coms_pthread_create(&thread, NULL, thread_pool_worker, pool);
        THREAD_LOG_NAME(thread.id, "pool");
        coms_pthread_detach(thread);
    }

    LOG_2(
        "[INFO] %d threads running",
        {DATA_TYPE_INT64,
            (void *) &_stats_counter->stats[
                atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE + DEBUG_COUNTER_THREAD
            ]
        }
    );
}

inline
void thread_pool_wait(ThreadPool* const pool) NO_EXCEPT
{
    MutexGuard _guard(&pool->work_mutex);
    while (pool->working_cnt != 0 || pool->thread_cnt != 0) {
        coms_pthread_cond_wait(&pool->working_cond, &pool->work_mutex);
    }
}

void thread_pool_destroy(ThreadPool* const pool) NO_EXCEPT
{
    // This sets the queue to empty
    atomic_set_release(&pool->work_queue.tail, pool->work_queue.head);

    // This sets the state to "shutdown"
    pool->state = THREAD_POOL_STATE_WAITING;

    coms_pthread_cond_broadcast(&pool->work_cond);
    thread_pool_wait(pool);

    mutex_destroy(&pool->work_mutex);
    coms_pthread_cond_destroy(&pool->work_cond);
    coms_pthread_cond_destroy(&pool->working_cond);

    // This sets the state to "down"
    pool->state = THREAD_POOL_STATE_COMPLETED;
}

PoolWorker* thread_pool_add_work(ThreadPool* const pool, const PoolWorker* job) NO_EXCEPT
{
    // @performance It might be better to not use mutexes but rather use atomics or spinlocks
    //              If we would change we would have to:
    //                  1. Use atomics for queue adds
    //                  2. in the queue use a second flag signaling when a insert is completed
    //                  3. use atomic increment for id_counter (how to avoid id = 0?)
    MutexGuard _guard(&pool->work_mutex);
    PoolWorker* const temp_job = (PoolWorker *) queue_enqueue_start_safe(&pool->work_queue);

    if (!temp_job) {
        ASSERT_TRUE(false);

        return NULL;
    }

    memcpy(temp_job, job, sizeof(PoolWorker));
    atomic_set_release((int32 *) &temp_job->state, POOL_WORKER_STATE_WAITING);

    queue_enqueue_end(&pool->work_queue);

    // +1 because otherwise the very first job would be id = 0 which is not a valid id
    ++pool->id_counter;
    if (!pool->id_counter) { UNLIKELY
        // ID of 0 is not allowed
        ++pool->id_counter;
    }

    temp_job->id = pool->id_counter;

    coms_pthread_cond_broadcast(&pool->work_cond);

    return temp_job;
}

// This is basically the same as thread_pool_add_work but allows us to directly write into the memory in the caller
// This makes it faster, since we can avoid a memcpy
inline
PoolWorker* thread_pool_add_work_start(ThreadPool* const pool) NO_EXCEPT
{
    // @performance It might be better to not use mutexes but rather use atomics or spinlocks
    //              If we would change we would have to:
    //                  1. Use atomics for queue adds
    //                  2. in the queue use a second flag signaling when a insert is completed
    //                  3. use atomic increment for id_counter (how to avoid id = 0?)
    mutex_lock(&pool->work_mutex);
    PoolWorker* const temp_job = (PoolWorker *) queue_enqueue_start(&pool->work_queue);

    // We need atomic here since the job state might be modified in the thread
    if (atomic_get_relaxed((int32 *) &temp_job->state) > POOL_WORKER_STATE_COMPLETED) {
        mutex_unlock(&pool->work_mutex);
        ASSERT_TRUE(temp_job->state <= POOL_WORKER_STATE_COMPLETED);

        return NULL;
    }

    memset(temp_job, 0, sizeof(PoolWorker));

    // +1 because otherwise the very first job would be id = 0 which is not a valid id
    ++pool->id_counter;
    if (!pool->id_counter) { UNLIKELY
        // ID of 0 is not allowed
        ++pool->id_counter;
    }

    temp_job->id = pool->id_counter;

    // Here we don't need atomic
    temp_job->state = POOL_WORKER_STATE_WAITING;

    return temp_job;
}

inline
void thread_pool_add_work_end(ThreadPool* const pool) NO_EXCEPT
{
    queue_enqueue_end(&pool->work_queue);
    coms_pthread_cond_broadcast(&pool->work_cond);
    mutex_unlock(&pool->work_mutex);
}

// We are not marking jobs const since it may change during the joining process (e.g. the state)
inline
bool thread_pool_join(PoolWorker* jobs, int32 count, uint64 sleep_time = 0, uint64 max_sleep = 0) NO_EXCEPT
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
                || atomic_get_relaxed((int32 *) jobs[i].state) == POOL_WORKER_STATE_COMPLETED
            ) {
                completed_mask |= bit;
            }
        }

        if (sleep_time && completed_mask != all_done_mask) {
            usleep(sleep_time);
            current_sleep += sleep_time;
        }
    }

    return completed_mask == all_done_mask;
}

inline
bool thread_pool_join(
    const PoolWorker* const* const jobs, int32 count,
    uint64 sleep_time = 0, uint64 max_sleep = 0
) NO_EXCEPT
{
    ASSERT_TRUE(count <= 64);

    // @question Maybe we should use a semaphore for this group of jobs
    //          to avoid the heavy thread lock due to looping below
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
                || atomic_get_relaxed((int32 *) jobs[i]->state) == POOL_WORKER_STATE_COMPLETED
            ) {
                completed_mask |= bit;
            }
        }

        if (completed_mask == all_done_mask) {
            return true;
        }

        if (sleep_time) {
            usleep(sleep_time);
            current_sleep += sleep_time;
        }
    }

    return false;
}

FORCE_INLINE
void thread_pool_work_release(ThreadPool* const pool, const PoolWorker* job) NO_EXCEPT
{
    MutexGuard _guard(&pool->work_mutex);
    queue_dequeue_release(&pool->work_queue, job);
}

#endif