/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SCHEDULER_H
#define COMS_SCHEDULER_H

#include "../stdlib/Stdlib.h"
#include "../system/Allocator.h"
#include "../thread/Thread.h"
#include "../memory/ThrdChunkMemory.h"

enum TaskScheduleFlag : uint8 {
    TASK_SCHEDULE_FLAG_NONE = 0,
    TASK_SCHEDULE_FLAG_RUNNING = 1 << 0,
    TASK_SCHEDULE_FLAG_CANCELLED = 1 << 1,
    TASK_SCHEDULE_FLAG_PAUSED = 1 << 2,

    // Allows this task to run multiple times even if it is already running
    TASK_SCHEDULE_FLAG_MULTIPLE = 1 << 3,
};

struct TaskScheduler;
struct TaskSchedule {
    atomic<uint64> next_run;
    uint64 end;
    atomic<uint64> previous_run;

    atomic<uint8> flags;
    atomic<int8> remaining_iteration; // -2 = infinite iterations, default = one run = 0

    // The sign bit allows us to define how to repeat the function
    // > 0 if next_run = current_time + repeat_interval
    // < 0 if next_run += repeat_interval;
    int32 repeat_interval;

    ThreadPoolJobFunc task_func;
    void* data;
    TaskScheduler* scheduler;
};

// Multithreading: Single consumer (one thread) multiple producers (multiple threads)
struct TaskScheduler {
    ThrdChunkMemoryT<TaskSchedule> tasks;
    ThreadPool* pool;
};

#endif