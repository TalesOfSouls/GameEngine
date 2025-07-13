/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPILER_GCC_ATOMIC_H
#define COMS_COMPILER_GCC_ATOMIC_H

#include "../../stdlib/Types.h"
#include "CompilerUtils.h"

typedef union { f32 f; int32 l; } _atomic_32;
typedef union { f64 f; int64 l; } _atomic_64;

FORCE_INLINE void atomic_set_relaxed(void** target, void* value) noexcept { __atomic_store_n(target, value, __ATOMIC_RELAXED); }
FORCE_INLINE void* atomic_get_relaxed(void** target) noexcept { return __atomic_load_n(target, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(volatile int8* value, int8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_fetch_set_relaxed(volatile int8* value, int8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_fetch_set_relaxed(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_fetch_set_relaxed(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_fetch_set_relaxed(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_get_relaxed(volatile int8* value) noexcept { return __atomic_load_n((int8 *) value, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_get_relaxed(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n((int16 *) value, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_get_relaxed(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n((int32 *) value, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_get_relaxed(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n((int64 *) value, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_increment_relaxed(volatile int8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_decrement_relaxed(volatile int8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_increment_relaxed(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_decrement_relaxed(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_increment_relaxed(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_decrement_relaxed(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_increment_relaxed(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_decrement_relaxed(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(volatile int8* value, int8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(volatile int8* value, int8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(volatile int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(volatile int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(volatile int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(volatile int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(volatile int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(volatile int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE f32 atomic_compare_exchange_strong_relaxed(volatile f32* value, f32* expected, f32 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 4) == 0);

    volatile _atomic_32* value_as_union = (volatile _atomic_32*)value;
    _atomic_32* expected_as_union = (_atomic_32*)expected;
    _atomic_32 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE f64 atomic_compare_exchange_strong_relaxed(volatile f64* value, f64* expected, f64 desired) noexcept {

    volatile _atomic_64* value_as_union = (volatile _atomic_64*)value;
    _atomic_64* expected_as_union = (_atomic_64*)expected;
    _atomic_64 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_RELAXED, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE int32 atomic_compare_exchange_strong_relaxed(volatile int32* value, int32* expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE int64 atomic_compare_exchange_strong_relaxed(volatile int64* value, int64* expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE int8 atomic_fetch_add_relaxed(volatile int8* value, int8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int8 atomic_fetch_sub_relaxed(volatile int8* value, int8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_fetch_add_relaxed(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int16 atomic_fetch_sub_relaxed(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_fetch_add_relaxed(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int32 atomic_fetch_sub_relaxed(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_fetch_add_relaxed(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE int64 atomic_fetch_sub_relaxed(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(volatile uint8* value, uint8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_set_relaxed(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_fetch_set_relaxed(volatile uint8* value, uint8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_fetch_set_relaxed(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_fetch_set_relaxed(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_fetch_set_relaxed(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_get_relaxed(volatile uint8* value) noexcept { return __atomic_load_n(value, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_get_relaxed(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n(value, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_get_relaxed(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n(value, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_get_relaxed(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n(value, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_increment_relaxed(volatile uint8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_decrement_relaxed(volatile uint8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_increment_relaxed(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_decrement_relaxed(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_increment_relaxed(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_decrement_relaxed(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_increment_relaxed(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_decrement_relaxed(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(volatile uint8* value, uint8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(volatile uint8* value, uint8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(volatile uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(volatile uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(volatile uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(volatile uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_add_relaxed(volatile uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_sub_relaxed(volatile uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_relaxed(volatile uint32* value, uint32* expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE uint64 atomic_compare_exchange_strong_relaxed(volatile uint64* value, uint64* expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE uint8 atomic_fetch_add_relaxed(volatile uint8* value, uint8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint8 atomic_fetch_sub_relaxed(volatile uint8* value, uint8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_fetch_add_relaxed(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint16 atomic_fetch_sub_relaxed(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_fetch_add_relaxed(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint32 atomic_fetch_sub_relaxed(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_fetch_add_relaxed(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE uint64 atomic_fetch_sub_relaxed(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(volatile uint8* value, uint8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(volatile int8* value, int8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_and_relaxed(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(volatile uint8* value, uint8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(volatile int8* value, int8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }
FORCE_INLINE void atomic_or_relaxed(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELAXED); }

FORCE_INLINE void* atomic_get_acquire(void** target) noexcept { return __atomic_load_n(target, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_fetch_set_acquire(volatile int8* value, int8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_fetch_set_acquire(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_fetch_set_acquire(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_fetch_set_acquire(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_get_acquire(volatile int8* value) noexcept { return __atomic_load_n((int8 *) value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_get_acquire(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n((int16 *) value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_get_acquire(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n((int32 *) value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_get_acquire(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n((int64 *) value, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_increment_acquire(volatile int8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_decrement_acquire(volatile int8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_increment_acquire(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_decrement_acquire(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_increment_acquire(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_decrement_acquire(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_increment_acquire(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_decrement_acquire(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(volatile int8* value, int8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(volatile int8* value, int8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(volatile int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(volatile int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(volatile int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(volatile int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(volatile int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(volatile int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE f32 atomic_compare_exchange_strong_acquire(volatile f32* value, f32* expected, f32 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 4) == 0);

    volatile _atomic_32* value_as_union = (volatile _atomic_32*)value;
    _atomic_32* expected_as_union = (_atomic_32*)expected;
    _atomic_32 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE f64 atomic_compare_exchange_strong_acquire(volatile f64* value, f64* expected, f64 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 8) == 0);

    volatile _atomic_64* value_as_union = (volatile _atomic_64*)value;
    _atomic_64* expected_as_union = (_atomic_64*)expected;
    _atomic_64 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_ACQUIRE, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE int32 atomic_compare_exchange_strong_acquire(volatile int32* value, int32* expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); return *expected; }
FORCE_INLINE int64 atomic_compare_exchange_strong_acquire(volatile int64* value, int64* expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); return *expected; }
FORCE_INLINE int8 atomic_fetch_add_acquire(volatile int8* value, int8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int8 atomic_fetch_sub_acquire(volatile int8* value, int8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_fetch_add_acquire(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int16 atomic_fetch_sub_acquire(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_fetch_add_acquire(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int32 atomic_fetch_sub_acquire(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_fetch_add_acquire(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE int64 atomic_fetch_sub_acquire(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_set_acquire(volatile uint8* value, uint8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_set_acquire(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_set_acquire(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_set_acquire(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_fetch_set_acquire(volatile uint8* value, uint8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_fetch_set_acquire(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_fetch_set_acquire(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_fetch_set_acquire(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_get_acquire(volatile uint8* value) noexcept { return __atomic_load_n(value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_get_acquire(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n(value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_get_acquire(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n(value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_get_acquire(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n(value, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_increment_acquire(volatile uint8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_decrement_acquire(volatile uint8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_increment_acquire(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_decrement_acquire(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_increment_acquire(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_decrement_acquire(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_increment_acquire(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_decrement_acquire(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(volatile uint8* value, uint8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(volatile uint8* value, uint8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(volatile uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(volatile uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(volatile uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(volatile uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_add_acquire(volatile uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_sub_acquire(volatile uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_acquire(volatile uint32* value, uint32* expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); return *expected; }
FORCE_INLINE uint64 atomic_compare_exchange_strong_acquire(volatile uint64* value, uint64* expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE); return *expected; }
FORCE_INLINE uint8 atomic_fetch_add_acquire(volatile uint8* value, uint8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint8 atomic_fetch_sub_acquire(volatile uint8* value, uint8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_fetch_add_acquire(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint16 atomic_fetch_sub_acquire(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_fetch_add_acquire(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint32 atomic_fetch_sub_acquire(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_fetch_add_acquire(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE uint64 atomic_fetch_sub_acquire(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(volatile uint8* value, uint8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(volatile int8* value, int8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_and_acquire(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(volatile uint8* value, uint8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(volatile int8* value, int8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }
FORCE_INLINE void atomic_or_acquire(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_ACQUIRE); }

FORCE_INLINE void atomic_set_release(void** target, void* value) noexcept { __atomic_store_n(target, value, __ATOMIC_RELEASE); }
FORCE_INLINE void* atomic_get_release(void** target) noexcept { return __atomic_load_n(target, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(volatile int8* value, int8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_fetch_set_release(volatile int8* value, int8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_fetch_set_release(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_fetch_set_release(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_set_release(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_get_release(volatile int8* value) noexcept { return __atomic_load_n((int8 *) value, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_get_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n((int16 *) value, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_get_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n((int32 *) value, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_get_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n((int64 *) value, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_increment_release(volatile int8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_decrement_release(volatile int8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_increment_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_decrement_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_increment_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_decrement_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_increment_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_decrement_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(volatile int8* value, int8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(volatile int8* value, int8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(volatile int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(volatile int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(volatile int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(volatile int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(volatile int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(volatile int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE f32 atomic_compare_exchange_strong_release(volatile f32* value, f32* expected, f32 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 4) == 0);

    volatile _atomic_32* value_as_union = (volatile _atomic_32*)value;
    _atomic_32* expected_as_union = (_atomic_32*)expected;
    _atomic_32 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_RELEASE, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE f64 atomic_compare_exchange_strong_release(volatile f64* value, f64* expected, f64 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 8) == 0);

    volatile _atomic_64* value_as_union = (volatile _atomic_64*)value;
    _atomic_64* expected_as_union = (_atomic_64*)expected;
    _atomic_64 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union->l, desired_as_union.l, 0,
        __ATOMIC_RELEASE, __ATOMIC_RELAXED
    );

    return expected_as_union->f;
}
FORCE_INLINE int32 atomic_compare_exchange_strong_release(volatile int32* value, int32* expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_RELEASE, __ATOMIC_RELEASE); return *expected; }
FORCE_INLINE int64 atomic_compare_exchange_strong_release(volatile int64* value, int64* expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_RELEASE, __ATOMIC_RELEASE); return *expected; }
FORCE_INLINE int8 atomic_fetch_add_release(volatile int8* value, int8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int8 atomic_fetch_sub_release(volatile int8* value, int8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_fetch_add_release(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int16 atomic_fetch_sub_release(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_fetch_add_release(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int32 atomic_fetch_sub_release(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_add_release(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_sub_release(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_and_release(volatile int64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_and_fetch(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE int64 atomic_fetch_or_release(volatile int64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_or_fetch(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_and_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_and_fetch(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_or_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_or_fetch(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(volatile uint8* value, uint8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_set_release(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_fetch_set_release(volatile uint8* value, uint8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_fetch_set_release(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_fetch_set_release(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_set_release(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_get_release(volatile uint8* value) noexcept { return __atomic_load_n(value, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_get_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n(value, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_get_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n(value, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_get_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n(value, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_increment_release(volatile uint8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_decrement_release(volatile uint8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_increment_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_decrement_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_increment_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_decrement_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_increment_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_decrement_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(volatile uint8* value, uint8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(volatile uint8* value, uint8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(volatile uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(volatile uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(volatile uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(volatile uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_add_release(volatile uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_sub_release(volatile uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_RELEASE); }
// @bug Wrong implementation, see strong_acquire_release
FORCE_INLINE uint32 atomic_compare_exchange_strong_release(volatile uint32* value, uint32* expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE uint64 atomic_compare_exchange_strong_release(volatile uint64* value, uint64* expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, expected, desired, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED); return *expected; }
FORCE_INLINE uint8 atomic_fetch_add_release(volatile uint8* value, uint8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint8 atomic_fetch_sub_release(volatile uint8* value, uint8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_fetch_add_release(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint16 atomic_fetch_sub_release(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_fetch_add_release(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint32 atomic_fetch_sub_release(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_add_release(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE uint64 atomic_fetch_sub_release(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(volatile uint8* value, uint8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(volatile int8* value, int8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_and_release(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(volatile uint8* value, uint8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(volatile int8* value, int8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }
FORCE_INLINE void atomic_or_release(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_RELEASE); }

FORCE_INLINE void atomic_set_acquire_release(void** target, void* value) noexcept { __atomic_store_n(target, value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void* atomic_get_acquire_release(void** target) noexcept { return __atomic_load_n(target, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(volatile int8* value, int8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_fetch_set_acquire_release(volatile int8* value, int8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_fetch_set_acquire_release(volatile int16* value, int16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_fetch_set_acquire_release(volatile int32* value, int32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_fetch_set_acquire_release(volatile int64* value, int64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_get_acquire_release(volatile int8* value) noexcept { return __atomic_load_n((int8 *) value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_get_acquire_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n((int16 *) value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_get_acquire_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n((int32 *) value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_get_acquire_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n((int64 *) value, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_increment_acquire_release(volatile int8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_decrement_acquire_release(volatile int8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_increment_acquire_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_decrement_acquire_release(volatile int16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_increment_acquire_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_decrement_acquire_release(volatile int32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_increment_acquire_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_decrement_acquire_release(volatile int64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(volatile int8* value, int8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(volatile int8* value, int8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(volatile int16* value, int16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(volatile int16* value, int16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(volatile int32* value, int32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(volatile int32* value, int32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(volatile int64* value, int64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(volatile int64* value, int64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE f32 atomic_compare_exchange_strong_acquire_release(volatile f32* value, f32 expected, f32 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 4) == 0);

    volatile _atomic_32* value_as_union = (volatile _atomic_32*)value;
    _atomic_32 expected_as_union = (_atomic_32)expected;
    _atomic_32 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union.l, desired_as_union.l, 0,
        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
    );

    return expected_as_union.f;
}
FORCE_INLINE f64 atomic_compare_exchange_strong_acquire_release(volatile f64* value, f64 expected, f64 desired) noexcept {
    ASSERT_STRICT(((uintptr_t) value % 8) == 0);

    volatile _atomic_64* value_as_union = (volatile _atomic_64*)value;
    _atomic_64 expected_as_union = (_atomic_64)expected;
    _atomic_64 desired_as_union;
    desired_as_union.f = desired;

    __atomic_compare_exchange_n(
        &value_as_union->l, &expected_as_union.l, desired_as_union.l, 0,
        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
    );

    return expected_as_union.f;
}
FORCE_INLINE int32 atomic_compare_exchange_strong_acquire_release(volatile int32* value, int32 expected, int32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, &expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); return expected; }
FORCE_INLINE int64 atomic_compare_exchange_strong_acquire_release(volatile int64* value, int64 expected, int64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, &expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); return expected; }
FORCE_INLINE int8 atomic_fetch_add_acquire_release(volatile int8* value, int8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int8 atomic_fetch_sub_acquire_release(volatile int8* value, int8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_fetch_add_acquire_release(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int16 atomic_fetch_sub_acquire_release(volatile int16* value, int16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_fetch_add_acquire_release(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int32 atomic_fetch_sub_acquire_release(volatile int32* value, int32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_fetch_add_acquire_release(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE int64 atomic_fetch_sub_acquire_release(volatile int64* value, int64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(volatile uint8* value, uint8 new_value) noexcept { __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_set_acquire_release(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_store_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_fetch_set_acquire_release(volatile uint8* value, uint8 new_value) noexcept { return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_fetch_set_acquire_release(volatile uint16* value, uint16 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_fetch_set_acquire_release(volatile uint32* value, uint32 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_fetch_set_acquire_release(volatile uint64* value, uint64 new_value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_exchange_n(value, new_value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_get_acquire_release(volatile uint8* value) noexcept { return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_get_acquire_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_get_acquire_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_get_acquire_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_load_n(value, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_increment_acquire_release(volatile uint8* value) noexcept { return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_decrement_acquire_release(volatile uint8* value) noexcept { return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_increment_acquire_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_decrement_acquire_release(volatile uint16* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_increment_acquire_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_decrement_acquire_release(volatile uint32* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_increment_acquire_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_decrement_acquire_release(volatile uint64* value) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(volatile uint8* value, uint8 increment) noexcept { __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(volatile uint8* value, uint8 decrement) noexcept { __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(volatile uint16* value, uint16 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(volatile uint16* value, uint16 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(volatile uint32* value, uint32 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(volatile uint32* value, uint32 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_add_acquire_release(volatile uint64* value, uint64 increment) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_add_fetch(value, increment, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_sub_acquire_release(volatile uint64* value, uint64 decrement) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_sub_fetch(value, decrement, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_compare_exchange_strong_acquire_release(volatile uint32* value, uint32 expected, uint32 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_compare_exchange_n(value, &expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); return expected; }
FORCE_INLINE uint64 atomic_compare_exchange_strong_acquire_release(volatile uint64* value, uint64 expected, uint64 desired) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_compare_exchange_n(value, &expected, desired, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); return expected; }
FORCE_INLINE uint8 atomic_fetch_add_acquire_release(volatile uint8* value, uint8 operand) noexcept { return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint8 atomic_fetch_sub_acquire_release(volatile uint8* value, uint8 operand) noexcept { return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_fetch_add_acquire_release(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint16 atomic_fetch_sub_acquire_release(volatile uint16* value, uint16 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_fetch_add_acquire_release(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint32 atomic_fetch_sub_acquire_release(volatile uint32* value, uint32 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_fetch_add_acquire_release(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_add_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE uint64 atomic_fetch_sub_acquire_release(volatile uint64* value, uint64 operand) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); return __atomic_sub_fetch(value, operand, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(volatile uint8* value, uint8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(volatile int8* value, int8 mask) noexcept { __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_and_acquire_release(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_and(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(volatile uint8* value, uint8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(volatile int8* value, int8 mask) noexcept { __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(volatile uint16* value, uint16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(volatile int16* value, int16 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 2) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(volatile uint32* value, uint32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(volatile int32* value, int32 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 4) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(volatile uint64* value, uint64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }
FORCE_INLINE void atomic_or_acquire_release(volatile int64* value, int64 mask) noexcept { ASSERT_STRICT(((uintptr_t) value % 8) == 0); __atomic_fetch_or(value, mask, __ATOMIC_SEQ_CST); }

// Check out the intrinsic functions fence_memory and fence_write
// These are much faster and could accomplish what you are doing
#define atomic_fence_acquire() __atomic_thread_fence(__ATOMIC_ACQUIRE)

// Check out the intrinsic functions fence_memory and fence_write
// These are much faster and could accomplish what you are doing
#define atomic_fence_release() __atomic_thread_fence(__ATOMIC_RELEASE)

#endif