/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_SCHEDULER_H
#define COMS_SCHEDULER_H

#include "../stdlib/Stdlib.h"
#include "../system/Allocator.h"
#include "../thread/Thread.h"

enum TaskScheduleFlag : uint8 {
    TASK_SCHEDULE_FLAG_RUNNING = 1 << 0,
    TASK_SCHEDULE_FLAG_COMPLETED = 1 << 1,
    TASK_SCHEDULE_FLAG_REPEAT = 1 << 2, // After completion, reset and run again
    TASK_SCHEDULE_FLAG_CONTINUOUS = 1 << 3, // Needs to run every iteration or only once until end time
};

struct TaskSchedule {
    uint64 start;
    uint64 end;
    uint8 flags;
    int8 repeat_count; // -1 = infinite
    uint16 repeat_interval; // 1 = 100 ms, smaller intervals are not required?!
    uint32 priority;
    ThreadPoolJobFunc task_func;
    uint64 time;
    void* data;
    void* scheduler;
};

// Multithreading: Single consumer (one thread) multiple producers (multiple threads)
struct TaskScheduler {
    PersistentQueueT<TaskSchedule> tasks;
    ThreadPool* pool;
};

FORCE_INLINE
void scheduler_alloc(TaskScheduler* const scheduler, int32 count) NO_EXCEPT
{
    queue_alloc(&scheduler->tasks, count, count);
}

FORCE_INLINE
void thrd_scheduler_alloc(TaskScheduler* const scheduler, int32 count) NO_EXCEPT
{
    thrd_queue_alloc(&scheduler->tasks, count, count);
}

FORCE_INLINE
void scheduler_create(TaskScheduler* const scheduler, int32 count, BufferMemory* const buf) NO_EXCEPT
{
    queue_init(&scheduler->tasks, buf, count);
}

FORCE_INLINE
void thrd_scheduler_create(TaskScheduler* const scheduler, int32 count, BufferMemory* const buf) NO_EXCEPT
{
    thrd_queue_init(&scheduler->tasks, buf, count);
}

FORCE_INLINE
void scheduler_add(TaskScheduler* const scheduler, const TaskSchedule* const task) NO_EXCEPT
{
    queue_enqueue(&scheduler->tasks, task);
}

FORCE_INLINE
void thrd_scheduler_add(TaskScheduler* const scheduler, const TaskSchedule* const task) NO_EXCEPT
{
    thrd_queue_enqueue(&scheduler->tasks, task);
}

FORCE_INLINE
void scheduler_remove(TaskScheduler* const scheduler, uint32 element) NO_EXCEPT
{
    queue_dequeue_release(&scheduler->tasks, element);
}

FORCE_INLINE
void thrd_scheduler_remove(TaskScheduler* const scheduler, uint32 element) NO_EXCEPT
{
    thrd_queue_dequeue_release(&scheduler->tasks, element);
}

FORCE_INLINE
void scheduler_free(TaskScheduler* const scheduler) NO_EXCEPT
{
    queue_free(&scheduler->tasks);
}

FORCE_INLINE
void thrd_scheduler_free(TaskScheduler* const scheduler) NO_EXCEPT
{
    thrd_queue_free(&scheduler->tasks);
}

// This removes the task from the task queue after it is completed
static inline
void scheduler_cleanup_callback(void* data) {
    TaskSchedule* const task = (TaskSchedule *) data;
    TaskScheduler* const scheduler = (TaskScheduler *) task->scheduler;

    queue_dequeue_release(&scheduler->tasks, task);
}

void scheduler_run(TaskScheduler* const scheduler, uint64 current_time) NO_EXCEPT
{
    TaskSchedule* task;
    while(task = queue_dequeue_keep(&scheduler->tasks)) {
        if (task->start <= current_time
            || (task->end != 0 && task->end <= current_time)
        ) {
            queue_uncomplete(&scheduler->tasks, task);
            break;
        }

        // Already running or completed tasks are skipped
        if ((task->flags & TASK_SCHEDULE_FLAG_RUNNING)
            || (task->flags & TASK_SCHEDULE_FLAG_COMPLETED)
        ) {
            continue;
        }

        task->time = current_time;
        task->task_func(&task);
    }
}

void thrd_scheduler_run(TaskScheduler* const scheduler, uint64 current_time) NO_EXCEPT
{
    TaskSchedule* task;
    while (task = thrd_queue_dequeue_keep(&scheduler->tasks)) {
        if (task->start <= current_time
            || (task->end != 0 && task->end <= current_time)
        ) {
            thrd_queue_uncomplete(&scheduler->tasks, task);
            break;
        }

        // Already running or completed tasks are skipped
        if ((task->flags & TASK_SCHEDULE_FLAG_RUNNING)
            || (task->flags & TASK_SCHEDULE_FLAG_COMPLETED)
        ) {
            continue;
        }

        task->time = current_time;

        const PoolWorker job = {
            0,
            POOL_WORKER_STATE_WAITING,
            true,
            task->task_func,
            scheduler_cleanup_callback,
            task
        };
        thread_pool_add_work(scheduler->pool, &job);
    }
}

#endif