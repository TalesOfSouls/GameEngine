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

#include "../stdlib/Types.h"
#include "../system/Allocator.h"
#include "../thread/Thread.h"

enum TaskScheduleFlag : uint32 {
    TASK_SCHEDULE_FLAG_RUNNING = 1 << 0,
    TASK_SCHEDULE_FLAG_COMPLETED = 1 << 1,
    TASK_SCHEDULE_FLAG_REPEAT = 1 << 2, // After completion, reset and run again
    TASK_SCHEDULE_FLAG_CONTINUOUS = 1 << 3, // Needs to run every iteration or only once until end time
}

struct TaskSchedule {
    uint32 flags;
    uint64 start;
    uint64 end;
    uint64 repeat_interval;
    byte repeat_count;
    ThreadJobFunc task_func;
    void* data;
};

// Multithreading: Single consumer (one thread) multiple producers (multiple threads)
struct TaskScheduler {
    ThreadPool pool;

    int32 task_count;
    TaskSchedule* tasks;

    // Array which holds the tasks array indices ordered by start time
    // This way we can quickly find all tasks that should be run without re-ordering the tasks array every time
    // To be fair re-ordering the tasks array isn't that slow but we may need a fixed memory position for pointers
    alignas(4) atomic_32 int32* priorities;

    alignas(8) atomic_64 uint64* free;

    mutex lock;
};

inline
void scheduler_alloc(TaskScheduler* scheduler, int32 count) NO_EXCEPT
{
    byte* data = (byte *) platform_alloc_aligned(
        count * sizeof(TaskSchedule)
        + count * sizeof(int32)
        + CEIL_DIV(count, 64) * sizeof(scheduler->free)
        + 128,
        64
    );

    scheduler->tasks = (TaskSchedule *) data;
    scheduler->priorities = ROUND_TO_NEAREST((uintptr_t) (data + count * sizeof(TaskSchedule)), 64);
    scheduler->free = ROUND_TO_NEAREST((uintptr_t) (scheduler->priorities + count), 64);

    mutex_init(&scheduler->lock, NULL);
}

inline
void scheduler_create(TaskScheduler* scheduler, int32 count, BufferMemory* buf) NO_EXCEPT
{
    byte* data = buffer_get_memory(
        buf,
        count * sizeof(TaskSchedule)
        + count * sizeof(int32)
        + CEIL_DIV(count, 64) * sizeof(scheduler->free)
        + 128,
        64, true
    );

    scheduler->tasks = (TaskSchedule *) data;
    scheduler->priorities = ROUND_TO_NEAREST((uintptr_t) (data + count * sizeof(TaskSchedule)), 64);
    scheduler->free = ROUND_TO_NEAREST((uintptr_t) (scheduler->priorities + count), 64);

    mutex_init(&scheduler->lock, NULL);
}

static
int32 scheduler_get_unset(uint64* state, uint32 state_count) NO_EXCEPT {
    uint32 free_index = 0;
    uint32 bit_index = 0;

    for (uint32 i = 0; i < state_count; i+= 64) {
        if (state[free_index] != 0xFFFFFFFFFFFFFFFF) {
            bit_index = compiler_find_first_bit_r2l(~state[free_index]);

            uint32 id = free_index * 64 + bit_index;
            if (id >= state_count) {
                ++free_index;
                if (free_index * 64 >= state_count) {
                    free_index = 0;
                }

                continue;
            }

            state[free_index] |= (1ULL << bit_index);

            return id;
        }

        ++free_index;
        if (free_index * 64 >= state_count) {
            free_index = 0;
        }
    }

    return -1;
}

inline
void scheduler_add(TaskScheduler* scheduler, TaskSchedule* task) NO_EXCEPT
{
    mutex_lock(&scheduler->lock);
    int32 idx = scheduler_get_unset(scheduler->free, scheduler->task_count);

    if (idx < 0) {
        return;
    }

    memcpy(scheduler->tasks[idx], task, sizeof(TaskSchedule));
    mutex_unlock(&scheduler->lock);

    for (int32 i = 0; i < scheduler->task_count; ++i) {
        if (scheduler->tasks[scheduler->priorities[i]].start > task->start
            || scheduler->priorities[i] == -1
        ) {
            // Next element starts later -> this task needs to come before

            // Move larger elements 1 over
            memmove(scheduler->priorities + i + 1, scheduler->priorities + i, scheduler->task_count - i - 1);

            // Insert new element
            scheduler->priorities[i] = idx;

            break;
        }
    }
}

FORCE_INLINE
void scheduler_remove(TaskScheduler* scheduler, uint32 element) NO_EXCEPT
{
    uint64 free_index = element / 64;
    uint32 bit_index = element & 63;

    mutex_lock(&scheduler->lock);
    scheduler->free[free_index] &= ~(1ULL << bit_index);

    for (int32 i = 0; i < scheduler->task_count; ++i) {
        scheduler->tasks[element].start = -1;
    }

    mutex_unlock(&scheduler->lock);
}

FORCE_INLINE
void scheduler_free(TaskScheduler* scheduler) NO_EXCEPT
{
    platform_aligned_free(scheduler->tasks);
    mutex_destroy(&scheduler->lock);
}

// @question do we want to use mutex? do we want to use spinlock
void scheduler_run(TaskScheduler* scheduler, uint64 current_time) NO_EXCEPT
{
    for (int32 i = 0; i < scheduler->task_count; ++i) {
        TaskSchedule* task = scheduler->priorities[i];

        // Check if task should be played
        if (scheduler->tasks[i].start <= current_time
            || (scheduler->tasks[i].end != 0 && scheduler->tasks[i].end <= current_time)
        ) {
            break;
        }

        // Already running or completed tasks are skipped
        if ((scheduler->tasks[i].flags & TASK_SCHEDULE_FLAG_RUNNING)
            || (scheduler->tasks[i].flags & TASK_SCHEDULE_FLAG_COMPLETED)
        ) {
            continue;
        }

        // @todo run in thread pool
        // @todo pass current time as well in data -> create new struct for data
        scheduler->tasks[i].task_func(&scheduler->tasks[i]);
    }
}

#endif