/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_THREADS_ATOMIC_H
#define COMS_THREADS_ATOMIC_H

/**
 * We try to avoid the standard library atomic functionality so we can use the default datatypes and can avoid the standard library.
 * Performance wise this should be equally fast (at least on msvc and g++)
 *
 * There is also the rare situation when I want to modify an atomic variable but know that there is no race condition.
 * In such a situation I could simply use normal operations on that variable without relying on atomic instructions.
 * E.g. before or after the threaded operations start you might want to modify the variable.
 * Of course you must know that the thread isn't running that is also accessing this variable.
 * Another example could be that you have multiple atomic variables that occasionally get adjusted
 * but at one point in time you need to modify all. During that time you may suspend the thread that is also modifying them.
 * Another example can be seen in the QueueT implementation where we can create a SPSC, MPMC, SPMC, ... with a single implementation,
 * simply because the developer can choose which function to call without the necessity of different implementations,
 * even single threaded use is possible in that case with basically zero overhead since the developer can call the none-atomic functions.
 * With std::atomic this is impossible. Of course the actual performance gain is really small.
 *
 * @question What if we ever want to release on a platform where we don't have access to these intrinsics AND no assembly?
 *          It would be really painful to write a wrapper around the std::atomic implementation just so we can have parity.
 *          We really have to re-consider if we want to avoid std::atomic... I hate the std:: but we might have to use it here :(
 */

#if _WIN32
    #include "../platform/win32/threading/Atomic.h"
#elif __linux__
    #include "../platform/linux/threading/Atomic.h"
#endif

// The following functions are "non-standard" atomic helper functions that are based on other atomic functions

// @question These functions behave differently from the normal atomic increment function:
//          Normal: return old + increment
//          This:   increment + return new
template <typename T>
inline
T atomic_increment_wrap_relaxed(T* value, T threshold) NO_EXCEPT
{
    T old = atomic_get_relaxed(value);
    T next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        T expected = old;
        T prev = atomic_compare_exchange_strong_relaxed(
            value,
            expected,
            next
        );

        if (prev == old) {
            return next;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T atomic_increment_wrap_acquire(T* value, T threshold) NO_EXCEPT
{
    T old = atomic_get_acquire(value);
    T next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        T expected = old;
        T prev = atomic_compare_exchange_strong_acquire(
            value,
            expected,
            next
        );

        if (prev == old) {
            return next;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T atomic_increment_wrap_release(T* value, T threshold) NO_EXCEPT
{
    T old = atomic_get_release(value);
    T next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        T expected = old;
        T prev = atomic_compare_exchange_strong_release(
            value,
            expected,
            next
        );

        if (prev == old) {
            return next;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T atomic_increment_wrap_acquire_release(T* value, T threshold) NO_EXCEPT
{
    T old = atomic_get_acquire_release(value);
    T next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        T expected = old;
        T prev = atomic_compare_exchange_strong_acquire_release(
            value,
            expected,
            next
        );

        if (prev == old) {
            return next;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

inline
uint64 atomic_increment_wrap_acquire_release(uint64* value, uint64 threshold) NO_EXCEPT
{
    uint64 old = atomic_get_acquire_release(value);
    uint64 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        uint64 expected = old;
        uint64 prev = atomic_compare_exchange_strong_acquire_release(
            value,
            expected,
            next
        );

        if (prev == old) {
            return next;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T* atomic_fetch_increment_wrap_relaxed(T** value, T* start, T* end) NO_EXCEPT
{
    T* old = (T *) atomic_get_relaxed((void **) value);
    T* next;

    while (true) {
        next = old + 1;
        if (next >= end) {
            next = start;
        }

        T* expected = old;
        T* prev = (T *) atomic_compare_exchange_strong_relaxed(
            (uint_max *) (uintptr_t) value,
            (uint_max) (uintptr_t) expected,
            (uint_max) (uintptr_t) next
        );

        if (prev == old) {
            return prev;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T* atomic_fetch_increment_wrap_acquire(T** value, T* start, T* end) NO_EXCEPT
{
    T* old = (T *) atomic_get_acquire((void **) value);
    T* next;

    while (true) {
        next = old + 1;
        if (next >= end) {
            next = start;
        }

        T* expected = old;
        T* prev = (T *) atomic_compare_exchange_strong_acquire(
            (uint_max *) (uintptr_t) value,
            (uint_max) (uintptr_t) expected,
            (uint_max) (uintptr_t) next
        );

        if (prev == old) {
            return prev;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T* atomic_fetch_increment_wrap_release(T** value, T* start, T* end) NO_EXCEPT
{
    T* old = (T *) atomic_get_release((void **) value);
    T* next;

    while (true) {
        next = old + 1;
        if (next >= end) {
            next = start;
        }

        T* expected = old;
        T* prev = (T *) atomic_compare_exchange_strong_release(
            (uint_max *) (uintptr_t) value,
            (uint_max) (uintptr_t) expected,
            (uint_max) (uintptr_t) next
        );

        if (prev == old) {
            return prev;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T* atomic_fetch_increment_wrap_acquire_release(T** value, T* start, T* end) NO_EXCEPT
{
    T* old = (T *) atomic_get_acquire_release((void **) value);
    T* next;

    while (true) {
        next = old + 1;
        if (next >= end) {
            next = start;
        }

        T* expected = old;
        T* prev = (T *) atomic_compare_exchange_strong_acquire_release(
            (uint_max *) (uintptr_t) value,
            (uint_max) (uintptr_t) expected,
            (uint_max) (uintptr_t) next
        );

        if (prev == old) {
            return prev;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T atomic_fetch_increment_wrap_acquire(T* value, T start, T end) NO_EXCEPT
{
    T old = atomic_get_acquire(value);
    T next;

    while (true) {
        next = old + 1;
        if (next >= end) {
            next = start;
        }

        T expected = old;
        T prev = atomic_compare_exchange_strong_acquire(
            value,
            expected,
            next
        );

        if (prev == old) {
            return prev;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T atomic_fetch_increment_wrap_release(T* value, T start, T end) NO_EXCEPT
{
    T old = atomic_get_release(value);
    T next;

    while (true) {
        next = old + 1;
        if (next >= end) {
            next = start;
        }

        T expected = old;
        T prev = atomic_compare_exchange_strong_release(
            value,
            expected,
            next
        );

        if (prev == old) {
            return prev;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

template <typename T>
inline
T atomic_fetch_increment_wrap_acquire_release(T* value, T start, T end) NO_EXCEPT
{
    T old = atomic_get_acquire_release(value);
    T next;

    while (true) {
        next = old + 1;
        if (next >= end) {
            next = start;
        }

        T expected = old;
        T prev = atomic_compare_exchange_strong_acquire_release(
            value,
            expected,
            next
        );

        if (prev == old) {
            return prev;
        }

        // failed, retry with new observed value
        old = prev;
    }
}

#endif