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
    ThreadJobFunc task_func;
    void* data;
};

// Multithreading: Single consumer (one thread) multiple producers (multiple threads)
struct TaskScheduler {
    int32 task_count;
    TaskSchedule* tasks;
    ThreadPool* pool;

    // Array which holds the tasks array indices ordered by start time
    // This way we can quickly find all tasks that should be run without re-ordering the tasks array every time
    // To be fair re-ordering the tasks array isn't that slow but we may need a fixed memory position for pointers
    atomic_32 int32* priorities;
    atomic_64 uint64* free;

    mutex lock;
};

inline
void scheduler_alloc(TaskScheduler* scheduler, int32 count) NO_EXCEPT
{
    byte* data = (byte *) platform_alloc_aligned(
        count * sizeof(TaskSchedule)
        + count * sizeof(int32)
        + CEIL_DIV(count, 64) * sizeof(*scheduler->free)
        + 128,
        64
    );

    scheduler->tasks = (TaskSchedule *) data;
    scheduler->priorities = (volatile int32 *) OMS_ALIGN_UP((uintptr_t) (data + count * sizeof(TaskSchedule)), 64);
    scheduler->free = (volatile uint64 *) OMS_ALIGN_UP((uintptr_t) (scheduler->priorities + count), 64);

    mutex_init(&scheduler->lock, NULL);
}

inline
void scheduler_create(TaskScheduler* scheduler, int32 count, BufferMemory* buf) NO_EXCEPT
{
    byte* data = buffer_get_memory(
        buf,
        count * sizeof(TaskSchedule)
        + count * sizeof(int32)
        + CEIL_DIV(count, 64) * sizeof(*scheduler->free)
        + 128,
        64
    );

    scheduler->tasks = (TaskSchedule *) data;
    scheduler->priorities = (volatile int32 *) OMS_ALIGN_UP((uintptr_t) (data + count * sizeof(TaskSchedule)), 64);
    scheduler->free = (volatile uint64 *) OMS_ALIGN_UP((uintptr_t) (scheduler->priorities + count), 64);

    mutex_init(&scheduler->lock, NULL);
}

static
int32 scheduler_get_unset(volatile uint64* state, uint32 state_count) NO_EXCEPT {
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

static inline
void scheduler_add(TaskScheduler* scheduler, TaskSchedule* task) NO_EXCEPT
{
    mutex_lock(&scheduler->lock);
    int32 idx = scheduler_get_unset(scheduler->free, scheduler->task_count);
    if (idx < 0) {
        mutex_unlock(&scheduler->lock);
        return;
    }

    memcpy(&scheduler->tasks[idx], task, sizeof(TaskSchedule));

    for (int32 i = 0; i < scheduler->task_count; ++i) {
        if (scheduler->tasks[scheduler->priorities[i]].start > task->start
            || scheduler->priorities[i] == -1
        ) {
            // Next element starts later -> this task needs to come before

            // Move larger elements 1 over
            memmove(
                (void *) (scheduler->priorities + i + 1),
                (void *) (scheduler->priorities + i),
                scheduler->task_count - i - 1
            );

            // Insert new element
            scheduler->priorities[i] = idx;

            break;
        }
    }

    mutex_unlock(&scheduler->lock);
}

FORCE_INLINE
void scheduler_remove(TaskScheduler* scheduler, uint32 element) NO_EXCEPT
{
    uint64 free_index = element / 64;
    uint32 bit_index = MODULO_2(element, 64);

    mutex_lock(&scheduler->lock);
    scheduler->free[free_index] &= ~(1ULL << bit_index);

    for (int32 i = 0; i < scheduler->task_count; ++i) {
        scheduler->tasks[element].start = 0;
    }

    mutex_unlock(&scheduler->lock);
}

FORCE_INLINE
void scheduler_free(TaskScheduler* scheduler) NO_EXCEPT
{
    platform_aligned_free((void **) &scheduler->tasks);
    mutex_destroy(&scheduler->lock);
}

// @question do we want to use mutex? do we want to use spinlock
void scheduler_run(TaskScheduler* scheduler, uint64 current_time) NO_EXCEPT
{
    for (int32 i = 0; i < scheduler->task_count; ++i) {
        TaskSchedule* task = &scheduler->tasks[scheduler->priorities[i]];

        // Check if task should be played
        if (task->start <= current_time
            || (task->end != 0 && task->end <= current_time)
        ) {
            break;
        }

        // Already running or completed tasks are skipped
        if ((task->flags & TASK_SCHEDULE_FLAG_RUNNING)
            || (task->flags & TASK_SCHEDULE_FLAG_COMPLETED)
        ) {
            continue;
        }

        // @todo run in thread pool
        // @todo pass current time as well in data -> create new struct for data
        // @question Do we even want this to call the pool in here? Maybe we want an iterator where we can decide per task if it should run in a pool or something else.
        task->task_func(&scheduler->tasks[i]);
    }
}

#endif