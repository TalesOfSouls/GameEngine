/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SCHEDULER_C
#define COMS_SCHEDULER_C

#include "TaskScheduler.h"
#include "../memory/PersistentQueueT.cpp"

FORCE_INLINE
void scheduler_alloc(TaskScheduler* const scheduler, int32 count) NO_EXCEPT
{
    queue_alloc(&scheduler->tasks, count, count);
}

FORCE_INLINE
void thrd_scheduler_alloc(TaskScheduler* const scheduler, int32 count) NO_EXCEPT
{
    thrd_queue_alloc(&scheduler->tasks, count, count, ASSUMED_CACHE_LINE_SIZE);
}

FORCE_INLINE
void scheduler_init(TaskScheduler* const scheduler, BufferMemory* const buf, int32 count) NO_EXCEPT
{
    queue_init(&scheduler->tasks, buf, count);
}

FORCE_INLINE
void thrd_scheduler_init(TaskScheduler* const scheduler, BufferMemory* const buf, int32 count) NO_EXCEPT
{
    thrd_queue_init(&scheduler->tasks, buf, count, ASSUMED_CACHE_LINE_SIZE);
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
            0, // .id =
            POOL_WORKER_STATE_WAITING, // .state =
            true, // .atomic_release =
            0, // .arg_size =
            task, // .arg =
            task->task_func, // .func =
            scheduler_cleanup_callback, // .callback =
            NULL // .mem =
        };
        thread_pool_add_work(scheduler->pool, &job);
    }
}

#endif