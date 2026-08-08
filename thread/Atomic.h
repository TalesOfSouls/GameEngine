/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_ATOMIC_H
#define COMS_THREADS_ATOMIC_H

#if defined(NO_STDLIB) && NO_STDLIB
    #include "atomic_no.h"
#else
    #include <atomic>

    template <typename T>
    using atomic = std::atomic<T>;
    using std::memory_order_relaxed;
    using std::memory_order_consume;
    using std::memory_order_acquire;
    using std::memory_order_release;
    using std::memory_order_acq_rel;
    using std::memory_order_seq_cst;
#endif

// The following functions are "non-standard" atomic helper functions that are based on other atomic functions

template <typename T>
inline T atomic_increment_wrap_relaxed(atomic<T>& value, T threshold) noexcept
{
    T old = value.load(memory_order_relaxed);

    while (true) {
        T next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_relaxed,
                memory_order_relaxed))
        {
            return next;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T atomic_increment_wrap_acquire(atomic<T>& value, T threshold) noexcept
{
    T old = value.load(memory_order_acquire);

    while (true) {
        T next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_acquire,
                memory_order_acquire))
        {
            return next;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T atomic_increment_wrap_release(atomic<T>& value, T threshold) noexcept
{
    T old = value.load(memory_order_relaxed);

    while (true) {
        T next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_release,
                memory_order_relaxed))
        {
            return next;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T atomic_increment_wrap_acquire_release(atomic<T>& value, T threshold) noexcept
{
    T old = value.load(memory_order_acquire);

    while (true) {
        T next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_acq_rel,
                memory_order_acquire))
        {
            return next;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

inline uint64 atomic_increment_wrap_acquire_release(atomic<uint64>& value, uint64 threshold) noexcept
{
    uint64 old = value.load(memory_order_acquire);

    while (true) {
        uint64 next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_acq_rel,
                memory_order_acquire))
        {
            return next;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T* atomic_fetch_increment_wrap_relaxed(atomic<T*>& value, T* start, const T* end) noexcept
{
    T* old = value.load(memory_order_relaxed);

    while (true) {
        T* next = old + 1;
        if (next >= end) {
            next = start;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_relaxed,
                memory_order_relaxed))
        {
            return old;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T* atomic_fetch_increment_wrap_acquire(atomic<T*>& value, T* start, const T* end) noexcept
{
    T* old = value.load(memory_order_acquire);

    while (true) {
        T* next = old + 1;
        if (next >= end) {
            next = start;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_acquire,
                memory_order_acquire))
        {
            return old;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T* atomic_fetch_increment_wrap_release(atomic<T*>& value, T* start, const T* end) noexcept
{
    T* old = value.load(memory_order_relaxed);

    while (true) {
        T* next = old + 1;
        if (next >= end) {
            next = start;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_release,
                memory_order_relaxed))
        {
            return old;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T* atomic_fetch_increment_wrap_acquire_release(atomic<T*>& value, T* start, const T* end) noexcept
{
    T* old = value.load(memory_order_acquire);

    while (true) {
        T* next = old + 1;
        if (next >= end) {
            next = start;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_acq_rel,
                memory_order_acquire))
        {
            return old;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T atomic_fetch_increment_wrap_acquire(atomic<T>& value, T start, T end) noexcept
{
    T old = value.load(memory_order_acquire);

    while (true) {
        T next = old + 1;
        if (next >= end) {
            next = start;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_acquire,
                memory_order_acquire))
        {
            return old;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T atomic_fetch_increment_wrap_release(atomic<T>& value, T start, T end) noexcept
{
    T old = value.load(memory_order_relaxed);

    while (true) {
        T next = old + 1;
        if (next >= end) {
            next = start;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_release,
                memory_order_relaxed))
        {
            return old;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

template <typename T>
inline T atomic_fetch_increment_wrap_acquire_release(atomic<T>& value, T start, T end) noexcept
{
    T old = value.load(memory_order_acquire);

    while (true) {
        T next = old + 1;
        if (next >= end) {
            next = start;
        }

        if (value.compare_exchange_weak(
                old,
                next,
                memory_order_acq_rel,
                memory_order_acquire))
        {
            return old;
        }

        // On failure, compare_exchange_weak updates `old` with the
        // current value automatically, so we can retry immediately.
    }
}

#endif