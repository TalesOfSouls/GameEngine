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

#if _WIN32
    #include "../platform/win32/threading/Atomic.h"
#elif __linux__
    #include "../platform/linux/threading/Atomic.h"
#endif

// The following functions are "non-standard" atomic helper functions that are based on other atomic functions

inline
int32 atomic_increment_wrap_relaxed(volatile int32* value, int32 threshold) NO_EXCEPT {
    int32 old = atomic_get_relaxed(value);
    int32 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        int32 expected = old;
        int32 prev = atomic_compare_exchange_strong_relaxed(
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
int32 atomic_increment_wrap_acquire(volatile int32* value, int32 threshold) NO_EXCEPT {
    int32 old = atomic_get_acquire(value);
    int32 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        int32 expected = old;
        int32 prev = atomic_compare_exchange_strong_acquire(
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
int32 atomic_increment_wrap_release(volatile int32* value, int32 threshold) NO_EXCEPT {
    int32 old = atomic_get_release(value);
    int32 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        int32 expected = old;
        int32 prev = atomic_compare_exchange_strong_release(
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
int32 atomic_increment_wrap_acquire_release(volatile int32* value, int32 threshold) NO_EXCEPT {
    int32 old = atomic_get_acquire_release(value);
    int32 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        int32 expected = old;
        int32 prev = atomic_compare_exchange_strong_acquire_release(
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
uint32 atomic_increment_wrap_relaxed(volatile uint32* value, uint32 threshold) NO_EXCEPT {
    uint32 old = atomic_get_relaxed(value);
    uint32 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        uint32 expected = old;
        uint32 prev = atomic_compare_exchange_strong_relaxed(
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
uint32 atomic_increment_wrap_acquire(volatile uint32* value, uint32 threshold) NO_EXCEPT {
    uint32 old = atomic_get_acquire(value);
    uint32 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        uint32 expected = old;
        uint32 prev = atomic_compare_exchange_strong_acquire(
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
uint32 atomic_increment_wrap_release(volatile uint32* value, uint32 threshold) NO_EXCEPT {
    uint32 old = atomic_get_release(value);
    uint32 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        uint32 expected = old;
        uint32 prev = atomic_compare_exchange_strong_release(
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
uint32 atomic_increment_wrap_acquire_release(volatile uint32* value, uint32 threshold) NO_EXCEPT {
    uint32 old = atomic_get_acquire_release(value);
    uint32 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        uint32 expected = old;
        uint32 prev = atomic_compare_exchange_strong_acquire_release(
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
int64 atomic_increment_wrap_relaxed(volatile int64* value, int64 threshold) NO_EXCEPT {
    int64 old = atomic_get_relaxed(value);
    int64 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        int64 expected = old;
        int64 prev = atomic_compare_exchange_strong_relaxed(
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
int64 atomic_increment_wrap_acquire(volatile int64* value, int64 threshold) NO_EXCEPT {
    int64 old = atomic_get_acquire(value);
    int64 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        int64 expected = old;
        int64 prev = atomic_compare_exchange_strong_acquire(
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
int64 atomic_increment_wrap_release(volatile int64* value, int64 threshold) NO_EXCEPT {
    int64 old = atomic_get_release(value);
    int64 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        int64 expected = old;
        int64 prev = atomic_compare_exchange_strong_release(
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
int64 atomic_increment_wrap_acquire_release(volatile int64* value, int64 threshold) NO_EXCEPT {
    int64 old = atomic_get_acquire_release(value);
    int64 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        int64 expected = old;
        int64 prev = atomic_compare_exchange_strong_acquire_release(
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
uint64 atomic_increment_wrap_relaxed(volatile uint64* value, uint64 threshold) NO_EXCEPT {
    uint64 old = atomic_get_relaxed(value);
    uint64 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        uint64 expected = old;
        uint64 prev = atomic_compare_exchange_strong_relaxed(
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
uint64 atomic_increment_wrap_acquire(volatile uint64* value, uint64 threshold) NO_EXCEPT {
    uint64 old = atomic_get_acquire(value);
    uint64 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        uint64 expected = old;
        uint64 prev = atomic_compare_exchange_strong_acquire(
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
uint64 atomic_increment_wrap_release(volatile uint64* value, uint64 threshold) NO_EXCEPT {
    uint64 old = atomic_get_release(value);
    uint64 next;

    while (true) {
        next = old + 1;
        if (next >= threshold) {
            next = 0;
        }

        uint64 expected = old;
        uint64 prev = atomic_compare_exchange_strong_release(
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
uint64 atomic_increment_wrap_acquire_release(volatile uint64* value, uint64 threshold) NO_EXCEPT {
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

#endif