/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_SPSCQUEUE_T_H
#define COMS_MEMORY_SPSCQUEUE_T_H

#include "../stdlib/Stdlib.h"

template <typename T>
struct SPSCQueueT {
    T* memory;
    T* head;
    T* tail;
    int capacity;
};

#endif