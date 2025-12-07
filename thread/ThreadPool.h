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

#include "../stdlib/Types.h"
#include "../memory/Queue.h"
#include "../memory/BufferMemory.h"
#include "../log/DebugMemory.h"
#include "Thread.h"
#include "Atomic.h"
#include "ThreadJob.h"
#include "../log/DebugContainer.h"
#include "../utils/RandomUtils.h"

struct ThreadPool {
    // This is not a threaded queue since we want to handle the mutex in here, not in the queue for finer control
    Queue work_queue;

    // @performance Could it make more sense to use a spinlock for the thread pool?
    // Is probably overkill if we cannot really utilize it a lot -> we would have a lot of spinning
    mutex work_mutex;
    mutex_cond work_cond;
    mutex_cond working_cond;

    // By design the working_cnt is <= thread_cnt
    atomic_32 int32 working_cnt;
    atomic_32 int32 thread_cnt;

    int32 size;
    int32 element_size;

    // @question Why are we using atomics if we are using mutex at the same time?
    //  I think because the mutex is actually intended for the queue.
    // 1 = waiting for run, 2 = running, 0 = completed, -1 = canceling
    atomic_32 int32 state;
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
    ThreadPool* pool = (ThreadPool *) arg;

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
            MutexGuard _guard(&pool->work_mutex);

            while (pool->state > 1 && queue_is_empty(&pool->work_queue)) {
                coms_pthread_cond_wait(&pool->work_cond, &pool->work_mutex);
            }

            if (pool->state < 2) {
                break;
            }

            // We define a queue element as free based on it's id
            // So even if we "keep" it in the queue the pool will not overwrite it as long as the id > 0 (see pool_add)
            // This is only a ThreadPool specific queue behavior to avoid additional memory copy
            work = (PoolWorker *) queue_dequeue_keep(&pool->work_queue);
        }

        // When the worker functions of the thread pool get woken up it is possible that the work is already dequeued
        // by another thread -> we need to check if the work is actually valid
        if (work->state <= POOL_WORKER_STATE_COMPLETED) {
            atomic_set_release((volatile int32*) &work->state, POOL_WORKER_STATE_COMPLETED);
            continue;
        }

        atomic_increment_release(&pool->working_cnt);
        atomic_set_release((volatile int32*) &work->state, POOL_WORKER_STATE_RUNNING);
        LOG_3("ThreadPool worker started");
        {
            PROFILE(PROFILE_THREADPOOL_WORK, NULL, PROFILE_FLAG_ADD_HISTORY);
            STATS_INCREMENT(DEBUG_COUNTER_THREAD_ACTIVE);
            work->func(work);
            STATS_DECREMENT(DEBUG_COUNTER_THREAD_ACTIVE);
        }
        LOG_3("ThreadPool worker ended");

        // @question Do I really need state and id both? seems like setting one should be sufficient
        // Obviously we would also have to change thread_pool_add_work to check for state instead of id
        atomic_set_release((volatile int32*) &work->state, POOL_WORKER_STATE_COMPLETED);

        if (work->callback) {
            work->callback(work);
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
    ThreadPool* pool,
    int32 element_size,
    int32 thread_count,
    int32 worker_count,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE(PROFILE_THREAD_POOL_ALLOC);
    LOG_1(
        "[INFO] Allocating thread pool with %d threads and %d queue length",
        {DATA_TYPE_INT32, &thread_count},
        {DATA_TYPE_INT32, &worker_count}
    );

    queue_alloc(&pool->work_queue, worker_count, element_size, alignment);

    pool->element_size = element_size;
    pool->thread_cnt = thread_count;

    // @todo switch from pool mutex and pool cond to threadjob mutex/cond
    //      thread_pool_wait etc. should just iterate over all mutexes
    mutex_init(&pool->work_mutex, NULL);
    coms_pthread_cond_init(&pool->work_cond, NULL);
    coms_pthread_cond_init(&pool->working_cond, NULL);

    pool->state = 2;

    coms_pthread_t thread;
    for (pool->size = 0; pool->size < thread_count; ++pool->size) {
        MAYBE_UNUSED int32 id = coms_pthread_create(&thread, NULL, thread_pool_worker, pool);
        THREAD_LOG_NAME(id, "pool");
        PSEUDO_USE(id);
        coms_pthread_detach(thread);
    }

    LOG_2("[INFO] %d threads running", {DATA_TYPE_INT64, (void *) &_stats_counter->stats[atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE + DEBUG_COUNTER_THREAD]});
}

void thread_pool_create(
    ThreadPool* pool,
    BufferMemory* const buf,
    int32 element_size,
    int32 thread_count,
    int32 worker_count,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE(PROFILE_THREAD_POOL_ALLOC);
    LOG_1(
        "Creating thread pool with %d threads and %d queue length",
        {DATA_TYPE_INT32, &thread_count},
        {DATA_TYPE_INT32, &worker_count}
    );

    queue_init(&pool->work_queue, buf, worker_count, element_size, alignment);

    pool->element_size = element_size;
    pool->thread_cnt = thread_count;

    // @todo switch from pool mutex and pool cond to threadjob mutex/cond
    //      thread_pool_wait etc. should just iterate over all mutexes
    mutex_init(&pool->work_mutex, NULL);
    coms_pthread_cond_init(&pool->work_cond, NULL);
    coms_pthread_cond_init(&pool->working_cond, NULL);

    pool->state = 2;

    coms_pthread_t thread;
    for (pool->size = 0; pool->size < thread_count; ++pool->size) {
        MAYBE_UNUSED int32 id = coms_pthread_create(&thread, NULL, thread_pool_worker, pool);
        THREAD_LOG_NAME(id, "pool");
        PSEUDO_USE(id);
        coms_pthread_detach(thread);
    }

    LOG_2("[INFO] %d threads running", {DATA_TYPE_INT64, (void *) &_stats_counter->stats[atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE + DEBUG_COUNTER_THREAD]});
}

inline
void thread_pool_wait(ThreadPool* pool) NO_EXCEPT
{
    MutexGuard _guard(&pool->work_mutex);
    // @question We removed some state checks here, not sure if they were really necessary
    // remove this comment once we are sure everything works as expected
    while (pool->working_cnt != 0 || pool->thread_cnt != 0) {
        coms_pthread_cond_wait(&pool->working_cond, &pool->work_mutex);
    }
}

void thread_pool_destroy(ThreadPool* pool) NO_EXCEPT
{
    // This sets the queue to empty
    atomic_set_release((void **) &pool->work_queue.tail, pool->work_queue.head);

    // This sets the state to "shutdown"
    atomic_set_release(&pool->state, 1);

    coms_pthread_cond_broadcast(&pool->work_cond);
    thread_pool_wait(pool);

    mutex_destroy(&pool->work_mutex);
    coms_pthread_cond_destroy(&pool->work_cond);
    coms_pthread_cond_destroy(&pool->working_cond);

    // This sets the state to "down"
    atomic_set_release(&pool->state, 0);
}

PoolWorker* thread_pool_add_work(ThreadPool* pool, const PoolWorker* job) NO_EXCEPT
{
    MutexGuard _guard(&pool->work_mutex);
    PoolWorker* const temp_job = (PoolWorker *) ring_get_memory_nomove((RingMemory *) &pool->work_queue, pool->element_size, 8);

    if (atomic_get_relaxed((volatile int32*) &temp_job->state) > POOL_WORKER_STATE_COMPLETED) {
        mutex_unlock(&pool->work_mutex);
        ASSERT_TRUE(temp_job->state <= POOL_WORKER_STATE_COMPLETED);

        return NULL;
    }

    memcpy(temp_job, job, pool->element_size);
    temp_job->state = POOL_WORKER_STATE_WAITING;

    ring_move_pointer((RingMemory *) &pool->work_queue, &pool->work_queue.head, pool->element_size, 8);

    // @performance Do we really want to do this under all circumstances?
    //  There are many situations where we don't need an id
    // +1 because otherwise the very first job would be id = 0 which is not a valid id
    // @question Why do I even need an atomic if we are in a mutex locked region?
    temp_job->id = atomic_fetch_add_acquire(&pool->id_counter, 1) + 1;

    coms_pthread_cond_broadcast(&pool->work_cond);

    // @bug Do we really want to return the pointer? it might be overwritten at some point
    // How do we know it's the original job or a new/overwritten one?
    return temp_job;
}

// This is basically the same as thread_pool_add_work but allows us to directly write into the memory in the caller
// This makes it faster, since we can avoid a memcpy
inline
PoolWorker* thread_pool_add_work_start(ThreadPool* pool) NO_EXCEPT
{
    mutex_lock(&pool->work_mutex);

    PoolWorker* const temp_job = (PoolWorker *) queue_enqueue_start(&pool->work_queue);
    if (atomic_get_relaxed((volatile int32*) &temp_job->state) > POOL_WORKER_STATE_COMPLETED) {
        mutex_unlock(&pool->work_mutex);
        ASSERT_TRUE(temp_job->state <= POOL_WORKER_STATE_COMPLETED);

        return NULL;
    }

    memset(temp_job, 0, sizeof(PoolWorker));

    // @performance Do we really want to do this under all circumstances?
    //  There are many situations where we don't need an id
    // +1 because otherwise the very first job would be id = 0 which is not a valid id
    temp_job->id = atomic_fetch_add_acquire(&pool->id_counter, 1) + 1;

    temp_job->state = POOL_WORKER_STATE_WAITING;

    return temp_job;
}

inline
void thread_pool_add_work_end(ThreadPool* pool) NO_EXCEPT
{
    queue_enqueue_end(&pool->work_queue);
    coms_pthread_cond_broadcast(&pool->work_cond);
    mutex_unlock(&pool->work_mutex);
}

inline
void thread_pool_join(PoolWorker* jobs, int32 count) NO_EXCEPT
{
    uint64 completed_mask = 0;
    const uint64 all_done_mask = (count == 64 ? UINT64_MAX : ((1ULL << count) - 1));

    // Loop until all jobs have been marked completed
    // We use a bitmask to avoid re-checking already validated jobs
    // If we wouldn't do that we may risk that a job got re-used for a new job giving a false negative.
    while (completed_mask != all_done_mask) {
        for (int32 i = 0; i < count; ++i) {
            const uint64 bit = 1ULL << i;

            if (completed_mask & bit) {
                continue;
            }

            if (jobs[i].state == POOL_WORKER_STATE_COMPLETED) {
                completed_mask |= bit;
            }
        }
    }
}

inline
void thread_pool_join(const PoolWorker* const* const jobs, int32 count) NO_EXCEPT
{
    uint64 completed_mask = 0;
    const uint64 all_done_mask = (count == 64 ? UINT64_MAX : ((1ULL << count) - 1));

    // Loop until all jobs have been marked completed
    // We use a bitmask to avoid re-checking already validated jobs
    // If we wouldn't do that we may risk that a job got re-used for a new job giving a false negative.
    while (completed_mask != all_done_mask) {
        for (int32 i = 0; i < count; ++i) {
            const uint64 bit = 1ULL << i;

            if (completed_mask & bit) {
                continue;
            }

            if (jobs[i]->state == POOL_WORKER_STATE_COMPLETED) {
                completed_mask |= bit;
            }
        }
    }
}

#endif