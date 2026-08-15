/**
 * Alternative to  std::atomic by using the platform/compiler/asm specific atomic versions
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_ATOMIC_NO_STBLIB_H
#define COMS_THREADS_ATOMIC_NO_STBLIB_H

#if _WIN32
    #include "../platform/win32/threading/Atomic.h"
#elif __linux__
    #include "../platform/linux/threading/Atomic.h"
#endif

enum AtomicMemoryOrder : int {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst,
};

template <typename T>
struct atomic {
    volatile alignas(alignof(uintptr_t)) T value;

    atomic() = default;
    constexpr atomic(T desired) : value(desired) {}

    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;

    T load(AtomicMemoryOrder order = memory_order_acquire) NO_EXCEPT
    {
        // We are not using atomic functions since volatile + alignas(8) results in atomic operations
        switch (order) {
            case memory_order_relaxed: return this->value;
            case memory_order_consume: return this->value;
            case memory_order_acquire: return this->value;
            case memory_order_seq_cst: return this->value;
            default:

            return this->value;
        }
    }

    void store(T val, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: atomic_set_relaxed(&this->value, val); return;
            case memory_order_release: atomic_set_release(&this->value, val); return;
            case memory_order_seq_cst: atomic_set_seq_cst(&this->value, val); return;
            default:
                atomic_set_release(&this->value, val);
                return;
        }
    }

    T exchange(T val, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: return atomic_fetch_set_relaxed(&this->value, val);
            case memory_order_consume: return atomic_fetch_set_acquire(&this->value, val);
            case memory_order_acquire: return atomic_fetch_set_acquire(&this->value, val);
            case memory_order_release: return atomic_fetch_set_release(&this->value, val);
            case memory_order_acq_rel: return atomic_fetch_set_acquire_release(&this->value, val);
            case memory_order_seq_cst: return atomic_fetch_set_seq_cst(&this->value, val);
        }

        return atomic_fetch_set_release(&this->value, val);
    }

    bool compare_exchange(T& expected, T desired, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: return atomic_compare_exchange_relaxed(&this->value, &expected, desired);
            case memory_order_consume: return atomic_compare_exchange_acquire(&this->value, &expected, desired);
            case memory_order_acquire: return atomic_compare_exchange_acquire(&this->value, &expected, desired);
            case memory_order_release: return atomic_compare_exchange_release(&this->value, &expected, desired);
            case memory_order_acq_rel: return atomic_compare_exchange_acquire_release(&this->value, &expected, desired);
            case memory_order_seq_cst: return atomic_compare_exchange_seq_cst(&this->value, &expected, desired);
        }

        return atomic_compare_exchange_release(&this->value, &expected, desired);
    }

    bool compare_exchange_weak(T& expected, T desired, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: return atomic_compare_exchange_weak_relaxed(&this->value, &expected, desired);
            case memory_order_consume: return atomic_compare_exchange_weak_acquire(&this->value, &expected, desired);
            case memory_order_acquire: return atomic_compare_exchange_weak_acquire(&this->value, &expected, desired);
            case memory_order_release: return atomic_compare_exchange_weak_release(&this->value, &expected, desired);
            case memory_order_acq_rel: return atomic_compare_exchange_weak_acquire_release(&this->value, &expected, desired);
            case memory_order_seq_cst: return atomic_compare_exchange_weak_seq_cst(&this->value, &expected, desired);
        }

        return atomic_compare_exchange_weak_release(&this->value, &expected, desired);
    }

    bool compare_exchange_strong(T& expected, T desired, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: return atomic_compare_exchange_strong_relaxed(&this->value, &expected, desired);
            case memory_order_consume: return atomic_compare_exchange_strong_acquire(&this->value, &expected, desired);
            case memory_order_acquire: return atomic_compare_exchange_strong_acquire(&this->value, &expected, desired);
            case memory_order_release: return atomic_compare_exchange_strong_release(&this->value, &expected, desired);
            case memory_order_acq_rel: return atomic_compare_exchange_strong_acquire_release(&this->value, &expected, desired);
            case memory_order_seq_cst: return atomic_compare_exchange_strong_seq_cst(&this->value, &expected, desired);
        }

        return atomic_compare_exchange_strong_release(&this->value, &expected, desired);
    }

    T fetch_add(T val, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: return atomic_add_relaxed(&this->value, val);
            case memory_order_consume: return atomic_add_acquire(&this->value, val);
            case memory_order_acquire: return atomic_add_acquire(&this->value, val);
            case memory_order_release: return atomic_add_release(&this->value, val);
            case memory_order_acq_rel: return atomic_add_acquire_release(&this->value, val);
            case memory_order_seq_cst: return atomic_add_seq_cst(&this->value, val);
        }

        return atomic_add_release(&this->value, val);
    }

    T fetch_sub(T val, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: return atomic_sub_relaxed(&this->value, val);
            case memory_order_consume: return atomic_sub_acquire(&this->value, val);
            case memory_order_acquire: return atomic_sub_acquire(&this->value, val);
            case memory_order_release: return atomic_sub_release(&this->value, val);
            case memory_order_acq_rel: return atomic_sub_acquire_release(&this->value, val);
            case memory_order_seq_cst: return atomic_sub_seq_cst(&this->value, val);
        }

        return atomic_sub_release(&this->value, val);
    }

    T fetch_or(T val, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: return atomic_or_relaxed(&this->value, val);
            case memory_order_consume: return atomic_or_acquire(&this->value, val);
            case memory_order_acquire: return atomic_or_acquire(&this->value, val);
            case memory_order_release: return atomic_or_release(&this->value, val);
            case memory_order_acq_rel: return atomic_or_acquire_release(&this->value, val);
            case memory_order_seq_cst: return atomic_or_seq_cst(&this->value, val);
        }

        return atomic_or_release(&this->value, val);
    }

    T fetch_and(T val, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: return atomic_and_relaxed(&this->value, val);
            case memory_order_consume: return atomic_and_acquire(&this->value, val);
            case memory_order_acquire: return atomic_and_acquire(&this->value, val);
            case memory_order_release: return atomic_and_release(&this->value, val);
            case memory_order_acq_rel: return atomic_and_acquire_release(&this->value, val);
            case memory_order_seq_cst: return atomic_and_seq_cst(&this->value, val);
        }

        return atomic_and_release(&this->value, val);
    }

    T fetch_xor(T val, AtomicMemoryOrder order = memory_order_release) NO_EXCEPT
    {
        switch (order) {
            case memory_order_relaxed: return atomic_xor_relaxed(&this->value, val);
            case memory_order_consume: return atomic_xor_acquire(&this->value, val);
            case memory_order_acquire: return atomic_xor_acquire(&this->value, val);
            case memory_order_release: return atomic_xor_release(&this->value, val);
            case memory_order_acq_rel: return atomic_xor_acquire_release(&this->value, val);
            case memory_order_seq_cst: return atomic_xor_seq_cst(&this->value, val);
        }

        return atomic_xor_release(&this->value, val);
    }

    operator T() const NO_EXCEPT {
        return const_cast<atomic*>(this)->load();
    }

    T operator=(T val) NO_EXCEPT {
        store(val);
        return val;
    }

    T operator++() NO_EXCEPT {
        return fetch_add(T(1)) + T(1);
    }

    T operator++(int) NO_EXCEPT {
        return fetch_add(T(1));
    }

    T operator--() NO_EXCEPT {
        return fetch_sub(T(1)) - T(1);
    }

    T operator--(int) NO_EXCEPT {
        return fetch_sub(T(1));
    }

    T operator+=(T val) NO_EXCEPT {
        return fetch_add(val) + val;
    }

    T operator-=(T val) NO_EXCEPT {
        return fetch_sub(val) - val;
    }

    T operator&=(T val) NO_EXCEPT {
        return fetch_and(val) & val;
    }

    T operator|=(T val) NO_EXCEPT {
        return fetch_or(val) | val;
    }

    T operator^=(T val) NO_EXCEPT {
        return fetch_xor(val) ^ val;
    }
};

#endif