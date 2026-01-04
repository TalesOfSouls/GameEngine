/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_THREADS_SPINLOCK_H
#define COMS_THREADS_SPINLOCK_H

#if _WIN32
    #include "../platform/win32/threading/Spinlock.h"
#elif __linux__
    #include "../platform/linux/threading/Spinlock.h"
#endif

// By using this constructor/destructor pattern you can avoid deadlocks in case of exceptions
// Why? Well because if we go out of scope the destructor is automatically called and the lock is unlocked
struct SpinlockGuard {
    spinlock32* _lock = NULL;

    inline HOT_CODE
    explicit SpinlockGuard(spinlock32* const lock, int32 delay = 10) {
        this->_lock = lock;

        spinlock_start(this->_lock, delay);
    }

    inline HOT_CODE
    void unlock() {
        if (this->_lock) {
            spinlock_end(this->_lock);
            this->_lock = NULL;
        }
    }

    inline HOT_CODE
    ~SpinlockGuard() {
        this->unlock();
    }
};

#endif