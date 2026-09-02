/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SCHEDULER_C
#define COMS_SCHEDULER_C

#include "TaskScheduler.h"
#include "../memory/ThrdChunkMemoryT.cpp"

FORCE_INLINE
void scheduler_alloc(TaskScheduler* const scheduler, int32 count) NO_EXCEPT
{
    chunk_alloc(&scheduler->tasks, count, count);
}

FORCE_INLINE
void scheduler_init(TaskScheduler* const scheduler, BufferMemory* const buf, int32 count) NO_EXCEPT
{
    chunk_init(&scheduler->tasks, buf, count);
    DEBUG_MEMORY_SUBREGION((uintptr_t) scheduler->tasks.memory, count * sizeof(TaskSchedule));
}

FORCE_INLINE
void scheduler_add(TaskScheduler* const scheduler, const TaskSchedule* const task) NO_EXCEPT
{
    chunk_element_insert(&scheduler->tasks, task);
}

FORCE_INLINE
void scheduler_add(TaskScheduler* const scheduler, const TaskSchedule& task) NO_EXCEPT
{
    chunk_element_insert(&scheduler->tasks, task);
}

FORCE_INLINE
void scheduler_remove(TaskScheduler* const scheduler, uint32 element) NO_EXCEPT
{
    chunk_free_element(&scheduler->tasks, element);
}

FORCE_INLINE
void scheduler_free(TaskScheduler* const scheduler) NO_EXCEPT
{
    chunk_free(&scheduler->tasks);
}

void scheduler_run(TaskScheduler* const scheduler, uint64 current_time) NO_EXCEPT
{
    int32 chunk_id = 0;
    thrd_chunk_iterate_start(&scheduler->tasks, chunk_id) {
        TaskSchedule* const task = chunk_element_get(&scheduler->tasks, chunk_id);

        const uint8 flags = task->flags.load();
        if (task->next_run <= current_time) {
            continue;
        } else if ((task->end != 0 && task->end <= current_time)
            || (task->remaining_iteration.load() == -1)
            || (flags & TASK_SCHEDULE_FLAG_CANCELLED)
        ) {
            chunk_free_element(&scheduler->tasks, chunk_id);
            continue;
        } else if ((flags & TASK_SCHEDULE_FLAG_PAUSED)
            || (flags & (TASK_SCHEDULE_FLAG_RUNNING | TASK_SCHEDULE_FLAG_MULTIPLE)) == TASK_SCHEDULE_FLAG_RUNNING
        ) {
            continue;
        }

        task->flags.fetch_or(TASK_SCHEDULE_FLAG_RUNNING);
        const PoolWorker job = {
            SMN(id) 0,
            SMN(state) POOL_WORKER_STATE_WAITING,
            SMN(automatic_release) true,
            SMN(arg_size) 0,
            SMN(arg) task,
            SMN(func) task->task_func,
            SMN(callback) NULL,
            SMN(mem_size) 0,
            SMN(mem) NULL
        };
        thread_pool_add_work(scheduler->pool, &job);
    } thrd_chunk_iterate_end;
}

// This function needs to be called at the end of a task function
inline
void scheduler_task_cleanup(TaskSchedule* task, uint64 current_time) {                                                        \
    const int32 remaining = task->remaining_iteration.fetch_sub(1);

    task->previous_run.store(current_time);
    task->next_run.store(
        task->repeat_interval > 0
            ? current_time + task->repeat_interval
            : task->next_run.load() - task->repeat_interval
    );

    if ((task->end != 0 && task->end <= current_time)
        || remaining == -1
    ) {
        chunk_free_element(&task->scheduler->tasks, task);
    } else {
        // Set as no longer running
        task->flags.fetch_and((uint8) ~TASK_SCHEDULE_FLAG_RUNNING);
    }
}

#endif