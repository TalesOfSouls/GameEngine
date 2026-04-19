/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_SCHEDULER_H
#define COMS_SCHEDULER_H

#include "../stdlib/Stdlib.h"
#include "../system/Allocator.h"
#include "../thread/Thread.h"
#include "../memory/PersistentQueueT.h"

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

#endif